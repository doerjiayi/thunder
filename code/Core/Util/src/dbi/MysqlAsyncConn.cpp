#include "MysqlAsyncConn.h"

namespace util
{

const char* state_descs[] = {
"NO_CONNECTED",
"CONNECT_WAITING",
"WAIT_OPERATE",
"EXECSQL_WAITING",
"QUERY_WAITING",
"STORE_WAITING"
};
//#define _USE_MYSQLCONN_LOG
#ifdef _USE_MYSQLCONN_LOG
#define _LOG(stage) {printf("\n%s:%d stage:%d,%s\n",__FILE__, __LINE__,stage,state_descs[stage]);}
#define _LOGSTR(str) {printf("\n%s:%d %s",__FILE__, __LINE__,str);}
#define _LOGSTR_ERROR(str,nError,sError) {printf("\n%s:%d %s error(%d:%s)",__FILE__, __LINE__,str,nError,sError);}
#else
#define _LOG(stage)
#define _LOGSTR(str)
#define _LOGSTR_ERROR(str,nError,sError)
#endif

SqlTask::SqlTask(const std::string& s,eSqlTaskOper o,MysqlHandler *h){sql = s;oper = o;handler=h;iErrno =0;}

MysqlAsyncConn::MysqlAsyncConn()
{
	m_port = 0;
	m_queryres = NULL;
	m_loop = NULL;
	m_curSqlTask = NULL;
	row = NULL;
	m_state = NO_CONNECTED;
	m_taskCounter = 0;
	m_connIndex = 0;
	m_boInit = false;
}

MysqlAsyncConn::~MysqlAsyncConn()
{
	std::list<SqlTask *>::const_iterator it = m_SqlTaskList.begin();
	std::list<SqlTask *>::const_iterator itEnd = m_SqlTaskList.end();
	for(;it != itEnd;++it)
	{
		delete (*it);
	}
	m_SqlTaskList.clear();
	if (m_curSqlTask)
	{
		delete m_curSqlTask;
		m_curSqlTask = NULL;
	}
}

int MysqlAsyncConn::init(const char *ip, int port,const char *user, const char *passwd,
		const char *dbname, const char *dbcharacterset,struct ev_loop *loop)
{
    strncpy(m_ip, ip, sizeof(m_ip));
    strncpy(m_user, user, sizeof(m_user));
    strncpy(m_passwd, passwd, sizeof(m_passwd));
    strncpy(m_dbname, dbname, sizeof(m_dbname));
    strncpy(m_dbcharacterset, dbcharacterset, sizeof(m_dbcharacterset));
    m_port = port;
    mysql_init(&m_mysql);
    mysql_options(&m_mysql, MYSQL_OPT_NONBLOCK, 0);
    unsigned int uiTimeOut = 3;
    mysql_options(&m_mysql, MYSQL_OPT_CONNECT_TIMEOUT, reinterpret_cast<char *>(&uiTimeOut));
    mysql_options(&m_mysql, MYSQL_OPT_COMPRESS, NULL);           //设置传输数据压缩
	mysql_options(&m_mysql, MYSQL_OPT_LOCAL_INFILE, NULL);       //设置允许使用LOAD FILE
	bool bBool = 1;//设置自动重连
	mysql_options(&m_mysql, MYSQL_OPT_RECONNECT, reinterpret_cast<char *>(&bBool));
    m_loop = loop;
    connect_start();
    m_boInit = true;
    return 0;
}

int MysqlAsyncConn::init(const tagDbConnInfo &stDbConnInfo,struct ev_loop *loop)
{
	return init(stDbConnInfo.m_szDbHost,stDbConnInfo.m_uiDbPort,stDbConnInfo.m_szDbUser,
			stDbConnInfo.m_szDbPwd,stDbConnInfo.m_szDbName,stDbConnInfo.m_szDbCharSet,loop);
}

void MysqlAsyncConn::libev_io_cb(struct ev_loop *loop, ev_io *watcher, int event)
{
    MysqlAsyncConn *conn = MysqlAsyncConn::get_self_by_watcher(watcher);
    conn->state_handle(loop, watcher, event);
}

int MysqlAsyncConn::event_to_mysql_status(int event)
{
    int status= 0;
    if (event & EV_READ)status|= MYSQL_WAIT_READ;
    if (event & EV_WRITE)status|= MYSQL_WAIT_WRITE;
    return status;
}

int MysqlAsyncConn::mysql_status_to_event(int status)
{
    int waitevent = 0;
    if (status & MYSQL_WAIT_READ)waitevent |= EV_READ;
    if (status & MYSQL_WAIT_WRITE)waitevent |= EV_WRITE;
    return waitevent;
}


int MysqlAsyncConn::state_handle(struct ev_loop *loop, ev_io *watcher, int event)
{
//    printf("\n%s,state:%d\n", __func__, m_state);
    if (m_state == CONNECT_WAITING)
    {
    	_LOG(CONNECT_WAITING);
        connect_wait(loop, watcher, event);
    }
    else if (m_state == EXECSQL_WAITING)
    {
    	_LOG(EXECSQL_WAITING);
        execsql_wait(loop, watcher, event);
    }
    else if (m_state == QUERY_WAITING)
    {
    	_LOG(QUERY_WAITING);
        query_wait(loop, watcher, event);
    }
    else if (m_state == STORE_WAITING)
    {
    	_LOG(STORE_WAITING);
        store_result_wait(loop, watcher, event);
    }
    else if (m_state == WAIT_OPERATE)
    {
    	_LOG(WAIT_OPERATE);
    	if (m_curSqlTask)
		{
			delete m_curSqlTask;
			m_curSqlTask = NULL;
		}
        if ((m_curSqlTask = fetch_next_task()) != NULL)
        {
            if (eSqlTaskOper_select == m_curSqlTask->oper)
            {
                query_start();
            }
            else
            {
                execsql_start();
            }
        }
        else
        {
        	stop_task();
        }
    }
    return 0;
}

int MysqlAsyncConn::wait_next_task(int libev_event)
{
	if (all_task_size() > 0)
	{
		ev_io_stop(m_loop, &m_watcher);
		ev_io_set(&m_watcher,m_watcher.fd, m_watcher.events|libev_event);//继续监听,等待下次循环
		ev_io_start (m_loop, &m_watcher);
	}
    return 0;
}

void MysqlAsyncConn::stop_task()
{
	ev_io_stop(m_loop, &m_watcher);
}

int MysqlAsyncConn::check_error_reconnect(SqlTask *task)
{
	/*
	 * 2006 (CR_SERVER_GONE_ERROR) : MySQL服务器不可用
	 * 2013 (CR_SERVER_LOST) : 查询过程中丢失了与MySQL服务器的连接
	 * mysql_ping 这个函数的返回值和文档说明有出入，返回0并不能保证连接是可用的
	 */
	_LOGSTR("check_error_reconnect\n");
	if (task->iErrno == 2006 || task->iErrno == 2013)
	{
		connect_start();
		return 0;
	}
	return 1;
}

int MysqlAsyncConn::connect_start()
{
    MYSQL *ret(NULL);
    int status = mysql_real_connect_start(&ret, &m_mysql, m_ip, m_user, m_passwd, m_dbname, m_port, NULL, 0);
    if (0 != status)
    {
    	_LOG(CONNECT_WAITING);
    	_LOGSTR("connecting\n");
        m_state = CONNECT_WAITING;
        int waitevent = mysql_status_to_event(status);
        if (!m_boInit)
        {
        	int socket = mysql_get_socket(&m_mysql);
        	ev_io_init (&m_watcher, libev_io_cb, socket, waitevent);
			ev_io_start(m_loop, &m_watcher);
        }
        else
        {
        	ev_io_stop(m_loop, &m_watcher);
			ev_io_set(&m_watcher, m_watcher.fd, m_watcher.events | waitevent);
			ev_io_start (m_loop, &m_watcher);
        }
    }
    else
    {
    	mysql_set_character_set(&m_mysql,m_dbcharacterset); //mysql 5.0 lib
        m_state = WAIT_OPERATE;
        wait_next_task();
        _LOGSTR("connected, no need wait\n");
    }
    return 0;
}

int MysqlAsyncConn::connect_wait(struct ev_loop *loop, ev_io *watcher, int event)
{
    MYSQL *ret;
    int status = event_to_mysql_status(event);
    status= mysql_real_connect_cont(&ret, &m_mysql, status);
    if (0 == status)
    {
    	mysql_set_character_set(&m_mysql,m_dbcharacterset); //mysql 5.0 lib
    	_LOGSTR("connected\n");
    	m_state = WAIT_OPERATE;
		wait_next_task();
    }
    else //LT模式会一直触发，等待下次被回调
    {
    	if (mysql_errno(&m_mysql) > 0)
		{
			_LOGSTR_ERROR("\nerror connect_wait ",mysql_errno(&m_mysql),mysql_error(&m_mysql));
			if (NULL == m_curSqlTask) m_curSqlTask = fetch_next_task();
			if (m_curSqlTask)
			{
				m_curSqlTask->iErrno = mysql_errno(&m_mysql);
				m_curSqlTask->errmsg = mysql_error(&m_mysql);
				if (0 == check_error_reconnect(m_curSqlTask)) return 0;//重新连接
				m_curSqlTask->handler->on_execsql(this, m_curSqlTask);
				delete m_curSqlTask;
				m_curSqlTask = NULL;
				wait_next_task();
			}//连接失败
			return 1;
		}
    }
	return 0;
}

int MysqlAsyncConn::execsql_start()
{
	_LOGSTR("execsql_start\n");
	if (m_curSqlTask)
	{
		int iRet(0);
		int status = mysql_real_query_start(&iRet, &m_mysql, m_curSqlTask->sql.c_str(), m_curSqlTask->sql.size());
		if (0 != iRet)
		{
			m_curSqlTask->iErrno = mysql_errno(&m_mysql);
			m_curSqlTask->errmsg = mysql_error(&m_mysql);
			if (0 == check_error_reconnect(m_curSqlTask)) return 0;//重新连接
			m_curSqlTask->handler->on_execsql(this, m_curSqlTask);
			m_state = WAIT_OPERATE;
			wait_next_task();
			return 1;
		}
		if (status)
		{
			m_state = EXECSQL_WAITING;
			int waitevent = mysql_status_to_event(status);
			ev_io_stop(m_loop, &m_watcher);
			ev_io_set (&m_watcher, m_watcher.fd, m_watcher.events|waitevent);
			ev_io_start(m_loop, &m_watcher);
		}
		else
		{
			_LOGSTR("execlsql done, no need wait\n");
			m_curSqlTask->handler->on_execsql(this, m_curSqlTask);
			m_state = WAIT_OPERATE;
			wait_next_task();
		}
	}
    return 0;
}


int MysqlAsyncConn::execsql_wait(struct ev_loop *loop, ev_io *watcher, int event)
{
	_LOGSTR("execsql_wait\n");
	if(m_curSqlTask)
	{
		int iRet(0);
		int status = event_to_mysql_status(event);
		status= mysql_real_query_cont(&iRet, &m_mysql, status);
		if (0 != iRet)
		{
			m_curSqlTask->iErrno = mysql_errno(&m_mysql);
			m_curSqlTask->errmsg = mysql_error(&m_mysql);
			if (0 == check_error_reconnect(m_curSqlTask)) return 0;//重新连接
			m_curSqlTask->handler->on_execsql(this, m_curSqlTask);
			m_state = WAIT_OPERATE;
			wait_next_task();
			return 1;
		}
		if (status != 0)
		{
			//LT模式会一直触发，等待下次被回调
			m_state = EXECSQL_WAITING;
		}
		else
		{
			m_curSqlTask->handler->on_execsql(this, m_curSqlTask);
			m_state = WAIT_OPERATE;
			wait_next_task();
		}
	}
    return 0;
}

int MysqlAsyncConn::query_start()
{
	_LOGSTR("query_start\n");
	if (m_curSqlTask)
	{
		int iRet(0);
		int status = mysql_real_query_start(&iRet, &m_mysql, m_curSqlTask->sql.c_str(), m_curSqlTask->sql.size());
		if (0 != iRet)
		{
			m_curSqlTask->iErrno = mysql_errno(&m_mysql);
			m_curSqlTask->errmsg = mysql_error(&m_mysql);
			if (0 == check_error_reconnect(m_curSqlTask)) return 0;//重新连接
			m_curSqlTask->handler->on_query(this, m_curSqlTask, m_queryres);
			m_state = WAIT_OPERATE;
			wait_next_task();
			return 1;
		}
		if (status)
		{
			m_state = QUERY_WAITING;
			int waitevent = mysql_status_to_event(status);
			ev_io_stop(m_loop, &m_watcher);
			ev_io_set(&m_watcher, m_watcher.fd, m_watcher.events|waitevent);
			ev_io_start(m_loop, &m_watcher);
		}
		else
		{
			_LOGSTR("query done, no need wait\n");
			store_result_start();
		}
	}
    return 0;
}

int MysqlAsyncConn::query_wait(struct ev_loop *loop, ev_io *watcher, int event)
{
	_LOGSTR("query_wait\n");
	if(m_curSqlTask)
	{
		int iRet(0);
		int status = event_to_mysql_status(event);
		status= mysql_real_query_cont(&iRet, &m_mysql, status);
		if (0 != iRet)
		{
			m_curSqlTask->iErrno = mysql_errno(&m_mysql);
			m_curSqlTask->errmsg = mysql_error(&m_mysql);
			if (0 == check_error_reconnect(m_curSqlTask)) return 0;//重新连接
			m_curSqlTask->handler->on_query(this, m_curSqlTask, m_queryres);
			m_state = WAIT_OPERATE;
			wait_next_task();
			return 1;
		}
		if (status != 0)
		{
			//LT模式会一直触发，等待下次被回调
			m_state = QUERY_WAITING;
		}
		else
		{
			_LOGSTR("query done\n");
			store_result_start();
		}
	}
    return 0;
}

int MysqlAsyncConn::store_result_start()
{
	_LOGSTR("store_result_start\n");
	if (m_curSqlTask)
	{
		int status = mysql_store_result_start(&m_queryres, &m_mysql);
		if (status)
		{
			m_state = STORE_WAITING;
			int waitevent = mysql_status_to_event(status);
			ev_io_stop(m_loop, &m_watcher);
			ev_io_set(&m_watcher, m_watcher.fd, m_watcher.events|waitevent);
			ev_io_start(m_loop, &m_watcher);
		}
		else
		{
			//printf("store_result done, no need wait\n");
			m_curSqlTask->handler->on_query(this, m_curSqlTask, m_queryres);
			m_state = WAIT_OPERATE;
			wait_next_task();
		}
	}
    return 0;
}

int MysqlAsyncConn::store_result_wait(struct ev_loop *loop, ev_io *watcher, int event)
{
	_LOGSTR("store_result_wait\n");
	if (m_curSqlTask)
	{
		int status = event_to_mysql_status(event);
		status= mysql_store_result_cont(&m_queryres, &m_mysql, status);
		if (status != 0)
		{
			//LT模式会一直触发，等待下次被回调
			m_state = STORE_WAITING;
		}
		else
		{
			//printf("store_result done\n");
			if (m_queryres && m_curSqlTask)
			{
				m_curSqlTask->handler->on_query(this, m_curSqlTask, m_queryres);
			}
			m_state = WAIT_OPERATE;
			wait_next_task();
		}
	}
    return 0;
}

int MysqlAsyncConn::close()
{
	_LOGSTR("close");
    mysql_close(&m_mysql);//同步关闭
    return 0;
}

MysqlResSet::MysqlResSet()
{
	m_pMysql = NULL;
	m_pResultSet = NULL;
	m_stCurrRow = NULL;
	m_dwRowCount = m_dwFieldCount = 0;
	m_iErrno = 0;
}

MysqlResSet::MysqlResSet(MYSQL_RES *pResultSet,MYSQL *mysql)
{
	Init(pResultSet,mysql);
}

MysqlResSet::~MysqlResSet()
{
	Clear();
}

void MysqlResSet::Init(MYSQL_RES *pResultSet,MYSQL *mysql)
{
    m_pMysql = mysql;//mysql对象由连接对象管理
    m_stCurrRow = NULL;
    if (pResultSet != m_pResultSet)
    {
        if (m_pResultSet)
        {
            delete m_pResultSet;
        }
        m_pResultSet = pResultSet;
    }
    if (m_pResultSet)
    {
        m_dwFieldCount = mysql_num_fields(m_pResultSet);
        m_dwRowCount   = mysql_num_rows(m_pResultSet);
    }
    else
    {
        m_dwRowCount = m_dwFieldCount = 0;
    }
    if (m_pMysql)
    {
        m_iErrno = mysql_errno(m_pMysql);
        m_strError = mysql_error(m_pMysql);
    }
    else
    {
        m_iErrno = 0;
        m_strError.clear();
    }
}

int MysqlResSet::GetResultSet(T_vecResultSet& vecResultSet)
{
	if (!m_pResultSet)return 0;
	MYSQL_FIELD* fields = mysql_fetch_fields(m_pResultSet);
	while ((m_stCurrRow = mysql_fetch_row(m_pResultSet)) != NULL)
	{
		T_mapRow mapRow;
		for (int i = 0; i < m_dwFieldCount; i++)
		{
			if (m_stCurrRow[i] != NULL)
			{
				mapRow[fields[i].name] = m_stCurrRow[i];
			}
			else
			{
				mapRow[fields[i].name] = "";
			}
		}
		vecResultSet.push_back(mapRow);
	}
	return vecResultSet.size();
}

void MysqlResSet::Clear()
{
	if (m_pResultSet)
	{
		mysql_free_result(m_pResultSet);
		m_pResultSet = NULL;
	}
}

unsigned long* MysqlResSet::FetchLengths()
{
	if (!m_pResultSet)return 0;
    return mysql_fetch_lengths(m_pResultSet);//Get column lengths of the current row
}

unsigned int MysqlResSet::FetchFieldNum()
{
	if (!m_pResultSet)return 0;
   	return mysql_num_fields(m_pResultSet);
}

unsigned int MysqlResSet::GetRowsNum()
{
	if (!m_pResultSet)return 0;
	return mysql_num_rows(m_pResultSet);
}

MYSQL_ROW MysqlResSet::GetRow()
{
	if (!m_pResultSet)return NULL;
    m_stCurrRow = mysql_fetch_row(m_pResultSet);
    return m_stCurrRow;
}

const MYSQL_RES* MysqlResSet::UseResult()
{
	return m_pResultSet;
}

MysqlAsyncConn *mysqlAsyncConnect(const char *ip, int port,const char *user,const char *passwd,const char *dbname,
		const char *dbcharacterset,struct ev_loop *loop)
{
	MysqlAsyncConn *pMysqlConn = new MysqlAsyncConn();
	int nRet = pMysqlConn->init(ip, port, user, passwd, dbname,dbcharacterset,loop);
	if (0 != nRet)
	{
		delete pMysqlConn;
		return NULL;
	}
	return pMysqlConn;
}

}
