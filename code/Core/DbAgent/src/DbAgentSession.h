/*
 * DbAgentSession.h
 *
 *  Created on: 2018年1月8日
 *      Author: chen
 */
#ifndef CODE_SRC_DBAGENTSSESSION_H_
#define CODE_SRC_DBAGENTSSESSION_H_
#include <string>
#include <map>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util/json/CJsonObject.hpp"
#include "dbi/MysqlDbi.hpp"
#include "session/Session.hpp"
#include "NetDefine.hpp"
#include "NetError.hpp"
#include "ProtoError.h"
#include "step/Step.hpp"
#include "cmd/Cmd.hpp"
#include "step/StepState.hpp"
#include "step/MysqlStep.hpp"

#define DBAGENT_SESSIN_ID (20000)

namespace core
{

const int gc_iMaxBeatTimeInterval = 30;
const int gc_iMaxColValueSize = 65535;

//数据库连接结构体定义
struct tagConnection
{
    util::CMysqlDbi* pDbi;
    time_t ullBeatTime;
    int iQueryPermit;
    int iTimeout;

    tagConnection() : pDbi(NULL), ullBeatTime(0), iQueryPermit(0), iTimeout(0)
    {
    }

    ~tagConnection()
    {
    	SAFE_DELETE(pDbi);
    }
};

class DbAgentSession: public net::Session
{
public:
    DbAgentSession(double session_timeout = 1.0)
		: net::Session(DBAGENT_SESSIN_ID, session_timeout,"net::DbAgentSession"),
		  m_uiCurrentTime(0),m_uiSync(1),m_uiSectionFrom(0),m_uiSectionTo(0),m_uiHash(0),m_uiDivisor(0),m_iConnectionTimeout(gc_iMaxBeatTimeInterval)
    {
    }
    virtual ~DbAgentSession()
    {
    	for (auto& iter:m_mapDbiPool)
		{
			SAFE_DELETE(iter.second);
		}
		m_mapDbiPool.clear();
    }
    bool Init();
    net::E_CMD_STATUS Timeout();

    bool LocateDbConn(const DataMem::MemOperate &oQuery,std::string &strInstance,util::CJsonObject& dbInstanceConf);
    bool GetDbConnection(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemOperate& oQuery, util::CMysqlDbi** ppMasterDbi, util::CMysqlDbi** ppSlaveDbi);
    bool GetDbConnectionFromIdentify(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,DataMem::MemOperate::DbOperate::E_QUERY_TYPE eQueryType,
    					const std::string& strMasterIdentify, const std::string& strSlaveIdentify,
    					const util::CJsonObject& oInstanceConf, util::CMysqlDbi** ppMasterDbi, util::CMysqlDbi** ppSlaveDbi);
    bool GetDbConnectionFromIdentifyForCluster(const std::string &strInstance,util::CJsonObject& oHostListConf,const util::CJsonObject& oInstanceConf,util::CMysqlDbi** ppMasterDbi);

	std::string GetFullTableName(const std::string& strTableName, uint64 uiFactor);

	int ConnectDb(const util::CJsonObject& oInstanceConf, util::CMysqlDbi* pDbi,const std::string& strDbIdentify);
	int ConnectDb(const util::tagDbConfDetail &stDbConfDetail, util::CMysqlDbi* pDbi);
	int SyncQuery(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const DataMem::MemOperate& oQuery, util::CMysqlDbi* pDbi);
	int AsyncQuery(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const DataMem::MemOperate& oQuery, util::CMysqlDbi* pDbi);
	int AsyncQueryCallback(const DataMem::MemOperate& oQuery, util::MysqlResSet* pMysqlResSet,int iResult,
			const std::string &strSql,const net::tagMsgShell &stMsgShell,const MsgHead &oInMsgHead,
			uint32 uiSectionFrom,uint32 uiSectionTo,uint32 uiHash,uint32 uiDivisor);
	void CheckConnection(); //检查连接是否已超时
	bool Response(const net::tagMsgShell &stMsgShell,const MsgHead &oInMsgHead,int iErrno, const std::string& strErrMsg);
	bool Response(const net::tagMsgShell &stMsgShell,const MsgHead &oInMsgHead,const DataMem::MemRsp& oRsp);

	bool CreateSql(const DataMem::MemOperate& oQuery, util::CMysqlDbi* pDbi, std::string& strSql);
	bool CreateSelect(const DataMem::MemOperate& oQuery, std::string& strSql);
	bool CreateInsert(const DataMem::MemOperate& oQuery, util::CMysqlDbi* pDbi, std::string& strSql);
	bool CreateUpdate(const DataMem::MemOperate& oQuery, util::CMysqlDbi* pDbi, std::string& strSql);
	bool CreateDelete(const DataMem::MemOperate& oQuery, std::string& strSql);
	bool CreateCondition(const DataMem::MemOperate::DbOperate::Condition& oCondition, util::CMysqlDbi* pDbi, std::string& strCondition);
	bool CreateConditionGroup(const DataMem::MemOperate& oQuery, util::CMysqlDbi* pDbi, std::string& strCondition);
	bool CreateGroupBy(const DataMem::MemOperate& oQuery, std::string& strGroupBy);
	bool CreateOrderBy(const DataMem::MemOperate& oQuery, std::string& strOrderBy);
	bool CreateLimit(const DataMem::MemOperate& oQuery, std::string& strLimit);
	bool CheckColName(const std::string& strColName);

	void RemoveFlag(std::string &str, char flag)const{str.erase(std::remove(str.begin(), str.end(), flag), str.end());}

	std::map<std::string, tagConnection*> m_mapDbiPool;     //数据库连接池，key为identify（如：192.168.18.22:3306）
	std::map<std::string, std::set<tagConnection*> > m_mapDBConnectSet;//数据库连接缓存 strInstance => set (DBConnect)

    uint32 m_uiCurrentTime;
    //配置
    uint32 m_uiSync;

    util::CJsonObject m_oDbConf;
	uint32 m_uiSectionFrom;
	uint32 m_uiSectionTo;
	uint32 m_uiHash;
	uint32 m_uiDivisor;
	int m_iConnectionTimeout;   //空闲连接超时（单位秒）

    std::map<std::string, std::set<uint32> > m_mapFactorSection; //分段因子区间配置，key为因子类型
	std::map<std::string, util::CJsonObject*> m_mapDbInstanceInfo;  //数据库配置信息key为("%u:%u:%u", uiDataType, uiFactor, uiFactorSection)

	char m_sErrMsg[256];
	char m_szColValue[gc_iMaxColValueSize];         //字段值
};

DbAgentSession* GetDbAgentSession();

}
;

#endif /* CODE_WEBSERVER_SRC_WEBSESSION_H_ */
