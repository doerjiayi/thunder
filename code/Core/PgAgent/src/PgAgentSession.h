/*
 * PgAgentSession.h
 *
 *  Created on: 2018年1月8日
 *      Author: chen
 */
#ifndef CODE_SRC_PGAGENTSSESSION_H_
#define CODE_SRC_PGAGENTSSESSION_H_
#include <string>
#include <map>
#include <vector>
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "postgresql/pqxx/pqxx"
#include "util/json/CJsonObject.hpp"
#include "dbi/Dbi.hpp"
#include "session/Session.hpp"
#include "NetDefine.hpp"
#include "NetError.hpp"
#include "ProtoError.h"
#include "step/Step.hpp"
#include "cmd/Cmd.hpp"
#include "storage/PgOperator.hpp"
#include "storage/DbOperator.hpp"

#define DBAGENT_SESSIN_ID (20000)
#define BULK_TIME (2)

namespace core
{

const int gc_iMaxBeatTimeInterval = 30;

//数据库连接结构体定义
struct tagConnection
{
    pqxx::connection* pPgConn;
    time_t ullBeatTime;
    int iQueryPermit;
    int iTimeout;

    tagConnection() : pPgConn(NULL), ullBeatTime(0), iQueryPermit(0), iTimeout(0)
    {
    }

    ~tagConnection()
    {
    	SAFE_DELETE(pPgConn);
    }
};

//enum FuncRecordStage
//{
//	eFuncRecordStage_init = 0,
//	eFuncRecordStage_running = 1,
//};
struct tb_job_record
{
	enum eEnable
	{
		eEnable_no = 0,//0：不执行
		eEnable_exe = 1,//1：执行
		eEnable_wait_del = 2,//2:准备删除
	};
	enum eStage
	{
		eStage_init = 0,//0：初始
		eStage_runnning = 1,//1：正在运行
		eStage_done_cal = 2,//2：计算完毕
	};
	tb_job_record(){ job_id = type = tag_id = enable = stage = 0; }
	std::string serial()const {return std::to_string(job_id) + "#" + describe + std::to_string(type) + "#" + sql +
			"#" + std::to_string(tag_id) + "#" + std::to_string(enable) + "#" + tablename + "#" + period + "#" + std::to_string(stage) +
			"#" + allow_start_time + "#" + allow_end_time + "#" + start_time + "#" + end_time + "#" + date + "#" + result;}
	uint32 job_id;
	std::string describe;
	uint32 type;
	std::string sql;
	uint32 tag_id;
	uint32 enable;
	std::string tablename;
	std::string period;
	uint32 stage;
	std::string allow_start_time;
	std::string allow_end_time;
	std::string start_time;
	std::string end_time;
	std::string date;
	std::string result;
};

struct SqlValues
{
	SqlValues(pqxx::connection* pConn,const std::string &values):pPgConn(pConn),sqlvalues(values){}
	pqxx::connection* pPgConn;
	std::string sqlvalues;
	SqlValues(const SqlValues & sql){*this = sql;}
	const SqlValues& operator=(const SqlValues & sql){pPgConn = sql.pPgConn;sqlvalues = sql.sqlvalues;return *this;}
};

class PgAgentSession: public net::Session
{
public:
    PgAgentSession(double session_timeout = 1.0)
		: net::Session(DBAGENT_SESSIN_ID, session_timeout,"net::PgAgentSession"),
		  boInit(false),boTest(false),m_iConnectionTimeout(gc_iMaxBeatTimeInterval),m_uiBuffSqlCheckTime(2),m_uiJobTime(10)
    {
    	m_uiCurrentTime = m_uiTimeOutCounter = 0;
    	m_uiBulkSqlEnable = m_uiBuffSqlMaxWriteCount = m_uiBulkSqlMaxSqlSize = 0;
    	m_uiPipelineSqlEnable = m_uiPipelineSqlMaxNum = m_uiPipelineSqlMaxSize = 0;
    	m_MsgRate.SetName("MsgRate");
    	m_MsgBulkRate.SetName("MsgBulkRate");
    	m_MsgCustomRate.SetName("MsgCustomRate");
    	m_SqlRate.SetName("SqlRate");
    	SetCurrentTime();
    }
    ~PgAgentSession();

    bool Init();
    void Tests();
    net::E_CMD_STATUS Timeout();
    bool Time2BreakBulkWrite(float totalTime,uint32 totalCounter);
    bool LoadConfig();
    bool LoadPgConfig();
    bool LocatePgConfig(DataMem::MemOperate& oQuery,int& nCode,std::string &strErrMsg,util::CJsonObject &oRspJson);

    //数据库api
    bool GetDbConnection(DataMem::MemRsp &oRsp,const DataMem::MemOperate& oQuery, pqxx::connection** ppMasterDbi, pqxx::connection** ppSlaveDbi);
	bool GetDbConnectionFromIdentify(DataMem::MemRsp &oRsp,DataMem::MemOperate::DbOperate::E_QUERY_TYPE eQueryType,
					const std::string& strMasterIdentify, const std::string& strSlaveIdentify,const std::string &strDbName,
					const util::CJsonObject& oInstanceConf, pqxx::connection** ppMasterDbi, pqxx::connection** ppSlaveDbi);

	bool QueryOper(DataMem::MemRsp &oRsp,const DataMem::MemOperate& oQuery);
	bool QueryRaw(DataMem::MemRsp &oRsp,const DataMem::MemOperate& oQuery, pqxx::connection* pPgConn);

	uint32 ExecuteSql(pqxx::connection* pPgConn,const std::string &strSql,pqxx::result &result,std::string &strErr);
	void StackSqlPineline(pqxx::connection* pPgConn,const std::string &strSql);
	uint32 ExecuteSqlPineline(pqxx::connection* pPgConn,std::list<std::string> &strSqls,uint32 maxnum,uint32 maxsize,std::string &strErr,bool boNeedResult=false);

	int ConnectDb(const util::CJsonObject& oInstanceConf, pqxx::connection* &pPgConn,const std::string& strDbIdentify);
	int ConnectDb(const util::tagDbConfDetail &stDbConfDetail, pqxx::connection* &pPgConn);
	void CheckConn(); //检查连接是否已超时

	bool Response(const net::tagMsgShell &stMsgShell,const MsgHead &oInMsgHead,int iErrno, const std::string& strErrMsg);
	bool Response(const net::tagMsgShell &stMsgShell,const MsgHead &oInMsgHead,const DataMem::MemRsp& oRsp);

	std::string GetFullTableName(const std::string& strTableName, uint64 uiFactor);
	bool CreateSql(const DataMem::MemOperate& oQuery, pqxx::connection* pPgConn, std::string& strSql);
	bool CreateSelect(const DataMem::MemOperate& oQuery, std::string& strSql);
	bool CreateInsert(const DataMem::MemOperate& oQuery, pqxx::connection* pPgConn, std::string& strSql);
	bool CreateUpdate(const DataMem::MemOperate& oQuery, pqxx::connection* pPgConn, std::string& strSql);
	bool CreateDelete(const DataMem::MemOperate& oQuery, std::string& strSql);
	bool CreateCondition(const DataMem::MemOperate::DbOperate::Condition& oCondition, pqxx::connection* pPgConn, std::string& strCondition);
	bool CreateConditionGroup(const DataMem::MemOperate& oQuery, pqxx::connection* pPgConn, std::string& strCondition);
	bool CreateGroupBy(const DataMem::MemOperate& oQuery, std::string& strGroupBy);
	bool CreateOrderBy(const DataMem::MemOperate& oQuery, std::string& strOrderBy);
	bool CreateLimit(const DataMem::MemOperate& oQuery, std::string& strLimit);
	bool CheckColName(const std::string& strColName);

	void SetCurrentTime(){m_uiCurrentTime = ::time(NULL);}
	void RemoveFlag(std::string &str,char flag = ' ')const{str.erase(std::remove(str.begin(), str.end(), flag), str.end());}
	void RunResult();

	util::CJsonObject m_objTestPGConfig;//测试

	bool boInit;
	bool boTest;
	uint32 m_iConnectionTimeout;   //空闲连接超时（单位秒）
	uint32 m_uiCurrentTime; //当前时间
	uint64 m_uiTimeOutCounter;

	char sBuff[256];
	char sError[1024];
	net::RunClock m_TestClock;
	net::RunClock m_RunClock;
	net::SendRate m_MsgRate;
	net::SendRate m_MsgBulkRate;
	net::SendRate m_MsgCustomRate;
	net::SendRate m_SqlRate;

	//buff
	uint32 m_uiBuffSqlCheckTime;//检查批量sql时间
	uint32 m_uiBuffSqlMaxWriteTime;//写批量sql时间限制（ms）
	uint32 m_uiBuffSqlMaxWriteCount;//写批量sql数量限制

	//bulk
	uint32 m_uiBulkSqlEnable;//是否缓存批量sql
	uint32 m_uiBulkSqlMaxSqlSize;//写批量sql长度限制
	std::map<std::string,std::list<SqlValues> > m_mapBulkSqls;//table operation:sqlvalues
	std::list<SqlValues> m_listCustomSqls;

	//Pipeline
	uint32 m_uiPipelineSqlEnable;//是否使用sql管道
	uint32 m_uiPipelineSqlMaxNum;//使用管道sql数量限制
	uint32 m_uiPipelineSqlMaxSize;//使用管道sql长度限制
	std::map<pqxx::connection*,std::list<std::string> > m_PinelineStrSqls;
	std::vector<std::pair<pqxx::pipeline::query_id, pqxx::result> > m_PinelineLastResults;
	std::vector<std::string> m_PinelineLastSqls;
	//配置
	util::CJsonObject m_oCurrentConf;
	util::CJsonObject m_oDbConf;
    std::map<std::string, tagConnection*> m_mapDbiPool;     //数据库连接池，key为identify（如：192.168.18.22:3306）
    std::map<std::string, std::set<tagConnection*> > m_mapDBConnectSet;//数据库连接缓存 strInstance => set (DBConnect)
    std::string m_strColValue;         //字段值缓存
    std::map<std::string, std::set<uint32> > m_mapFactorSection; //分段因子区间配置，key为因子类型
    std::map<std::string, util::CJsonObject*> m_mapDbInstanceInfo;  //数据库配置信息key为("%u:%u:%u", uiDataType, uiFactor, uiFactorSection)

    //job
	bool Get_tb_job_record();
	bool Set_tb_job_record(tb_job_record &record,const std::string &date);
	bool ExeJob(tb_job_record &record);
	uint32 m_uiJobTime;
};

PgAgentSession* GetPgAgentSession();

}
;

#endif /* CODE_WEBSERVER_SRC_WEBSESSION_H_ */
