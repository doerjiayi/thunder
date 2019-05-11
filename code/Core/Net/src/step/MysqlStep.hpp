/*******************************************************************************
 * Project:  Net
 * @file     MysqlStep.hpp
 * @brief    带mysql的异步步骤基类
 * @author   cjy
 * @date:    2017年8月15日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_MYSQLSTEP_HPP_
#define SRC_STEP_MYSQLSTEP_HPP_
#include <set>
#include <list>
#include "dbi/MysqlAsyncConn.h"
#include "Step.hpp"
#include "StepState.hpp"

namespace net
{
class CustomMysqlHandler;
class Worker;
//通用参数类（可根据需求自定义参数类）
struct SendToMysqlParam:public StepParam
{
	SendToMysqlParam(const std::string &strSql,uint8 uiCmdType):m_strSql(strSql),m_uiCmdType(uiCmdType){}
	std::string m_strSql;
	uint8 m_uiCmdType;
};
//Mysql访问步骤，在同一个状态下只有一个mysql访问任务，不同状态下可以追加不同任务，设置任务后会异步发送到mysql，在访问结果到达之后，会进入下一个状态
class MysqlStep: public StepState
{
public:
	MysqlStep(const std::string& strHost, int iPort,const std::string& dbname,const std::string& user,
			const std::string& passwd,const std::string &dbcharacterset,uint32 uiTimeOut=3);
	MysqlStep(const util::tagDbConnInfo &dbConnInfo);
	MysqlStep(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead,const util::tagDbConnInfo &dbConnInfo);
	MysqlStep(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead,const std::string& strHost, int iPort,const std::string& dbname,
			const std::string& user,const std::string& passwd,const std::string &dbcharacterset,uint32 uiTimeOut=3);
	MysqlStep(const tagMsgShell& stReqMsgShell, const HttpMsg& oInHttpMsg,const std::string& strHost, int iPort,const std::string& dbname,
			const std::string& user,const std::string& passwd,const std::string &dbcharacterset,uint32 uiTimeOut=3);
	MysqlStep(const tagMsgShell& stReqMsgShell, const HttpMsg& oInHttpMsg,const util::tagDbConnInfo &dbConnInfo);
	void Init(const std::string& strHost, int iPort,const std::string& dbname,
			const std::string& user,const std::string& passwd,const std::string &dbcharacterset,uint32 uiTimeOut=3);

	virtual ~MysqlStep();
	void SetConf(const util::tagDbConnInfo &dbConnInfo);
	void SetConf(const std::string& strHost, int iPort,const std::string& dbname,
			const std::string& user,const std::string& passwd,const std::string &dbcharacterset,uint32 uiTimeOut=3);
	static bool Launch(MysqlStep *pStep,uint32 uiTimeOutMax=3,uint8 uiToRetry=1,double dTimeout=3.0);//开始步骤
	//追加mysql访问任务(注册过的MysqlStep才能追加).追加mysql访问任务后，会异步提交访问并返回结果
	bool AppendTask(const std::string &strCmd,uint8 uiCmdType);
	bool AppendTask(uint8 uiCmdType,const char *fmt,...);
    /**
     * @brief Mysql步骤回调
     * @param c Mysql连接上下文
     * @param status 回调状态
     * @param pReply 执行结果集
     */
    virtual E_CMD_STATUS Callback(util::MysqlAsyncConn *c,util::SqlTask *task,MYSQL_RES *pResultSet);
    virtual E_CMD_STATUS Emit(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = ""){return StepState::Emit(iErrno,strErrMsg,strErrShow);}
    /**
     * @brief 超时回调
     * @return 回调状态
     */
    virtual E_CMD_STATUS Timeout();
    //设置mysql访问任务
    void SetTask(const std::string &strCmd,uint8 uiCmdType)
    {
    	m_strCmd = strCmd;
    	m_uiCmdType = uiCmdType;
    	if (util::eSqlTaskOper_exec == uiCmdType)
    	{
    		m_uiTimeOutRetry = 0;//访问mysql的写操作不重发
    	}
    }
    int SetTask(uint8 uiCmdType,const char *fmt,...)
    {
    	m_uiCmdType = uiCmdType;
		char printf_buf[1024];//目前最长指令设1024,超过长度的调用SetTask(const std::string &strCmd,uint8 uiCmdType)
		va_list args;
		int printed;
		va_start(args, fmt);
		printed = vsnprintf(printf_buf,sizeof(printf_buf),fmt, args);//搜索字符串fmt中需要特定模式的字符(比如%s),args为后面参数的首地址
		va_end(args);
		m_strCmd = printf_buf;
		if (util::eSqlTaskOper_exec == uiCmdType)
		{
			m_uiTimeOutRetry = 0;//访问mysql的写操作不重发
		}
		m_strLastCmd = m_strCmd;
		return printed;
    }
    void ClearTask(){m_strCmd.clear();m_uiCmdType=0;}
    const std::string& CurTask()const {return m_strCmd;}
    //参数
    std::string m_strHost;
    int m_iPort;
    std::string m_strDbName;
    std::string m_strUser;
    std::string m_strPasswd;
    std::string m_dbcharacterset;
    unsigned int m_uiTimeOut;

    util::tagDbConnInfo m_dbConnInfo;

    //任务
    std::string m_strCmd;
    std::string m_strLastCmd;
    uint8 m_uiCmdType;
	//回调结果
	util::MysqlResSet *m_pMysqlResSet;
};

class CustomMysqlHandler: public util::MysqlHandler {
public:
	CustomMysqlHandler(MysqlStep* step):m_uiMysqlStepSeq(step->GetSequence()){}
	virtual ~CustomMysqlHandler(){}
	int on_execsql(util::MysqlAsyncConn *c, util::SqlTask *task);
	int on_query(util::MysqlAsyncConn *c, util::SqlTask *task, MYSQL_RES *pResultSet);
	uint32 m_uiMysqlStepSeq;
};

} /* namespace net */

#endif /* SRC_STEP_REDISSTEP_HPP_ */
