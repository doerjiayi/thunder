#ifndef __MYSQL_CONN__
#define __MYSQL_CONN__

#include <mysql.h>
#include <stdint.h>
#include <assert.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <map>
#include <list>
#include <string>
#include <stdio.h>
#include <libev/ev.h>
#include "Dbi.hpp"

namespace util
{
class MysqlAsyncConn;
class MysqlResSet;
struct SqlTask;

enum eSqlTaskOper
{
	eSqlTaskOper_select,
	eSqlTaskOper_exec,
};

class MysqlHandler {
public:
	MysqlHandler(){}
	virtual ~MysqlHandler(){}
	virtual int on_execsql(MysqlAsyncConn *c, SqlTask *task) = 0;
	virtual int on_query(MysqlAsyncConn *c, SqlTask *task, MYSQL_RES *pResultSet) = 0;//pResultSet对象由回调者管理
};

struct SqlTask {
	SqlTask(const std::string& s,eSqlTaskOper o,MysqlHandler *h);
	virtual ~SqlTask(){if (handler){delete handler;handler=NULL;}}
	std::string sql;
	eSqlTaskOper oper;
	MysqlHandler *handler;
	int iErrno;
	std::string errmsg;
};

MysqlAsyncConn *mysqlAsyncConnect(const char *ip, int port,const char *user,const char *passwd,const char *dbname,
		const char *dbcharacterset,struct ev_loop *loop);

class MysqlResSet {
public:
	MysqlResSet();
	MysqlResSet(MYSQL_RES *pResultSet,MYSQL *mysql);
	~MysqlResSet();
	void Init(MYSQL_RES *pResultSet,MYSQL *mysql);
	//关闭结果集
	void Clear();
	//获取结果集
	int GetResultSet(T_vecResultSet& vecResultSet);
	//结果集操作
	//返回结果集记录行
	MYSQL_ROW GetRow();
	const MYSQL_RES* UseResult();
	//返回结果集内当前行的列的长度
	unsigned long* FetchLengths();
	//返回当前结果集列数
	unsigned int FetchFieldNum();
	//返回当前结果集行数
	unsigned int GetRowsNum();
	//返回当前结果集列名
	MYSQL_FIELD* FetchFields();
	//取上一次数据库操作错误码
	int GetErrno() const{return m_iErrno;}
	//取上一次数据库操作错误信息
	const std::string& GetError() const{return m_strError;}
private:
	MYSQL_RES *m_pResultSet;
	MYSQL_ROW m_stCurrRow;
	int m_dwRowCount;
	int m_dwFieldCount;

	int m_iErrno;
	std::string m_strError;

	MYSQL *m_pMysql;
};

enum mysql_conn_state {
	NO_CONNECTED,
	CONNECT_WAITING,
	WAIT_OPERATE,
	EXECSQL_WAITING,
	QUERY_WAITING,
	STORE_WAITING
};

#define util_offsetof(Type, Member) ( (size_t)( &(((Type*)8)->Member) ) - 8 )

class MysqlAsyncConn {
public:
	MysqlAsyncConn();
	~MysqlAsyncConn();
	static inline MysqlAsyncConn* get_self_by_watcher(ev_io* watcher) {
		return reinterpret_cast<MysqlAsyncConn*>(reinterpret_cast<uint8_t*>(watcher) - util_offsetof(MysqlAsyncConn, m_watcher));
	}
	static void libev_io_cb(struct ev_loop *loop, ev_io *watcher, int event);

	int init(const char *ip, int port,const char *user, const char *passwd,
			const char *dbname,const char *dbcharacterset,struct ev_loop *loop);
	int init(const tagDbConnInfo &stDbConnInfo,struct ev_loop *loop);
	int event_to_mysql_status(int event);
	int mysql_status_to_event(int status);
	int wait_next_task(int libev_event=EV_WRITE);//目前只有主动发起的写事件

	void stop_task();

	int state_handle(struct ev_loop *loop, ev_io *watcher, int event);

	int check_error_reconnect(SqlTask *task);

	int connect_start();
	int connect_wait(struct ev_loop *loop, ev_io *watcher, int event);

	int execsql_start();
	int execsql_wait(struct ev_loop *loop, ev_io *watcher, int event);

	int query_start();
	int query_wait(struct ev_loop *loop, ev_io *watcher, int event);

	int store_result_start();
	int store_result_wait(struct ev_loop *loop, ev_io *watcher, int event);

	int close();

	void add_task(SqlTask *task) {
		if (task)
		{
			m_SqlTaskList.push_back(task);
			wait_next_task();
		}
	}
	int task_size()const{return m_SqlTaskList.size();}
	int all_task_size()const{return m_SqlTaskList.size() + (m_curSqlTask ? 1:0);}
	SqlTask* fetch_next_task() {
		SqlTask *task = NULL;
		if (m_SqlTaskList.size() > 0)
		{
			task = m_SqlTaskList.front();
			m_SqlTaskList.pop_front();
		}
		return task;
	}
	SqlTask* fetch_top_task() {
		if (m_SqlTaskList.size() > 0)
		{
			return m_SqlTaskList.front();
		}
		return NULL;
	}

	void printf_all_sql() {
		for(std::list<SqlTask *>::const_iterator it = m_SqlTaskList.begin();it != m_SqlTaskList.end();++it)
		{
			printf("sql:%s\n", (*it)->sql.c_str());
		}
	}
	MYSQL* GetMysql(){return &m_mysql;}
private:
	bool m_boInit;

	char m_ip[32];
	int  m_port;
	char m_user[256];
	char m_passwd[256];
	char m_dbname[256];
	char m_dbcharacterset[32];

	int m_connIndex;
	MYSQL m_mysql;
	MYSQL_RES *m_queryres;
	MYSQL_ROW row;

	ev_io m_watcher;
	struct ev_loop *m_loop;
	std::list<SqlTask *> m_SqlTaskList;
	mysql_conn_state m_state;
	SqlTask *m_curSqlTask;
	int m_taskCounter;
};


}

#endif
