/*
 * PgAgentSession.cpp
 *
 *  Created on: 2018年1月8日
 *      Author: chenjiayi
 */
#include "util/UnixTime.hpp"
#include "PgAgentSession.h"

namespace core
{

PgAgentSession* GetPgAgentSession()
{
    PgAgentSession* pSess = (PgAgentSession*) net::GetSession(DBAGENT_SESSIN_ID,"net::PgAgentSession");
    if (pSess)
    {
        return (pSess);
    }
    pSess = new PgAgentSession();
    if (pSess == NULL)
    {
        LOG4_ERROR("error %d: new PgAgentSession() error!",ERR_NEW);
        return (NULL);
    }
	if (g_pLabor->RegisterCallback(pSess))
	{
		if (!pSess->Init())
		{
			g_pLabor->DeleteCallback(pSess);
			LOG4_ERROR("PgAgentSession init error!");
			return (NULL);
		}
		LOG4_DEBUG("register PgAgentSession ok!");
		return (pSess);
	}
	else
	{
		LOG4_ERROR("register PgAgentSession error!");
		delete pSess;
		pSess = NULL;
	}
    return (NULL);
}

PgAgentSession::~PgAgentSession()
{
	for (auto& iter:m_mapDbiPool)
	{
		SAFE_DELETE(iter.second);
	}
	m_mapDbiPool.clear();
}

bool PgAgentSession::LoadConfig()
{
	if (!net::GetJsonConfigFile(net::GetConfigPath() + std::string("PgAgentCmd.json"),m_oCurrentConf))
	{
		return (false);
	}
	LOAD_CONFIG(m_oCurrentConf,"job_time",m_uiJobTime);

	LOAD_CONFIG(m_oCurrentConf,"buff_sql_check_time",m_uiBuffSqlCheckTime);
	LOAD_CONFIG(m_oCurrentConf,"buff_sql_max_write_time",m_uiBuffSqlMaxWriteTime);
	LOAD_CONFIG(m_oCurrentConf,"buff_sql_max_write_count",m_uiBuffSqlMaxWriteCount);

	LOAD_CONFIG(m_oCurrentConf,"bulk_sql_enable",m_uiBulkSqlEnable);
	LOAD_CONFIG(m_oCurrentConf,"bulk_sql_max_sql_size",m_uiBulkSqlMaxSqlSize);

	LOAD_CONFIG(m_oCurrentConf,"pipeline_sql_enable",m_uiPipelineSqlEnable);
	LOAD_CONFIG(m_oCurrentConf,"pipeline_sql_max_num",m_uiPipelineSqlMaxNum);
	LOAD_CONFIG(m_oCurrentConf,"pipeline_sql_max_size",m_uiPipelineSqlMaxSize);

	m_oCurrentConf.Get("tests",m_objTestPGConfig);

	LOG4_INFO("%s job_time(%u) Buff(%u,%u,%u) Bulk(%u,%u) Pipeline(%u,%u,%u)",__FUNCTION__,
		m_uiJobTime,m_uiBuffSqlCheckTime,m_uiBuffSqlMaxWriteTime,m_uiBuffSqlMaxWriteCount,m_uiBulkSqlEnable,m_uiBulkSqlMaxSqlSize,
		m_uiPipelineSqlEnable,m_uiPipelineSqlMaxNum,m_uiPipelineSqlMaxSize);
	return LoadPgConfig();
}

bool PgAgentSession::LoadPgConfig()
{
	if (!net::GetJsonConfigFile(net::GetConfigPath() + std::string("CmdPgCluster.json"),m_oDbConf))
	{
		return (false);
	}
	LOG4_TRACE("m_oDbConf pasre OK");
	char szInstanceGroup[64] = {0};
	char szFactorSectionKey[32] = {0};
	uint32 uiDataType = 0;
	uint32 uiFactor = 0;
	uint32 uiFactorSection = 0;
	if (m_oDbConf["table"].IsEmpty())
	{
		LOG4_ERROR("m_oDbConf[\"table\"] is empty!");
		return(false);
	}
	if (m_oDbConf["cluster"].IsEmpty())
	{
		LOG4_ERROR("m_oDbConf[\"cluster\"] is empty!");
		return(false);
	}
	if (m_oDbConf["db_group"].IsEmpty())
	{
		LOG4_ERROR("m_oDbConf[\"db_group\"] is empty!");
		return(false);
	}

	for (int i = 0; i < m_oDbConf["data_type"].GetArraySize(); ++i)
	{
		if (m_oDbConf["data_type_enum"].Get(m_oDbConf["data_type"](i), uiDataType))
		{
			for (int j = 0; j < m_oDbConf["section_factor"].GetArraySize(); ++j)
			{
				if (m_oDbConf["section_factor_enum"].Get(m_oDbConf["section_factor"](j), uiFactor))
				{
					if (m_oDbConf["factor_section"][m_oDbConf["section_factor"](j)].IsArray())
					{
						std::set<uint32> setFactorSection;
						for (int k = 0; k < m_oDbConf["factor_section"][m_oDbConf["section_factor"](j)].GetArraySize(); ++k)
						{
							if (m_oDbConf["factor_section"][m_oDbConf["section_factor"](j)].Get(k, uiFactorSection))
							{
								snprintf(szInstanceGroup, sizeof(szInstanceGroup), "%u:%u:%u", uiDataType, uiFactor, uiFactorSection);
								snprintf(szFactorSectionKey, sizeof(szFactorSectionKey), "LE_%u", uiFactorSection);
								setFactorSection.insert(uiFactorSection);
								LOG4_DEBUG("szInstanceGroup(%s) add uiFactorSection(%u)",szInstanceGroup,uiFactorSection);
								util::CJsonObject* pInstanceGroup
									= new util::CJsonObject(m_oDbConf["cluster"][m_oDbConf["data_type"](i)][m_oDbConf["section_factor"](j)][szFactorSectionKey]);
								LOG4_TRACE("%s : %s", szInstanceGroup, pInstanceGroup->ToString().c_str());
								m_mapDbInstanceInfo.insert(std::pair<std::string, util::CJsonObject*>(szInstanceGroup, pInstanceGroup));
							}
							else
							{
								LOG4_ERROR("m_oDbConf[\"factor_section\"][%s](%d) is not exist!",
												m_oDbConf["section_factor"](j).c_str(), k);
								continue;
							}
						}
						snprintf(szInstanceGroup, sizeof(szInstanceGroup), "%u:%u", uiDataType, uiFactor);
						LOG4_DEBUG("szInstanceGroup(%s),factor size %u", szInstanceGroup, setFactorSection.size());
						m_mapFactorSection.insert(std::pair<std::string, std::set<uint32> >(szInstanceGroup, setFactorSection));
					}
					else
					{
						LOG4_ERROR("m_oDbConf[\"factor_section\"][%s] is not a json array!",
										m_oDbConf["section_factor"](j).c_str());
						continue;
					}
				}
				else
				{
					LOG4_ERROR("missing %s in m_oDbConf[\"section_factor_enum\"]", m_oDbConf["section_factor"](j).c_str());
					continue;
				}
			}
		}
		else
		{
			LOG4_ERROR("missing %s in m_oDbConf[\"data_type_enum\"]", m_oDbConf["data_type"](i).c_str());
			continue;
		}
	}
	return true;
}

bool PgAgentSession::GetDbConnection(DataMem::MemRsp &oRsp,const DataMem::MemOperate& oQuery,pqxx::connection** ppMasterDbi, pqxx::connection** ppSlaveDbi)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    char szFactor[32] = {0};
    int32 iDataType = 0;
    int32 iSectionFactorType = 0;
    m_oDbConf["table"][oQuery.db_operate().table_name()].Get("data_type", iDataType);
    m_oDbConf["table"][oQuery.db_operate().table_name()].Get("section_factor", iSectionFactorType);
    snprintf(szFactor, 32, "%d:%d", iDataType, iSectionFactorType);
    LOG4_TRACE("oQuery szFactor(%s),section_factor(%u),mod_factor(%llu)",szFactor,oQuery.section_factor(),oQuery.db_operate().mod_factor());//数据请求类型和因子
    auto c_factor_iter =  m_mapFactorSection.find(szFactor);
    if (c_factor_iter == m_mapFactorSection.end())
    {
        LOG4_ERROR("no db config found for data_type %d section_factor_type %d",iDataType, iSectionFactorType);
        oRsp.set_err_no(ERR_LACK_CLUSTER_INFO);
        oRsp.set_err_msg("no db config found for oMemOperate.cluster_info()!");
        return(false);
    }
    else
    {
    	auto c_section_iter = c_factor_iter->second.lower_bound(oQuery.section_factor());
        if (c_section_iter == c_factor_iter->second.end())
        {
            LOG4_ERROR("no factor_section config found for data_type %u section_factor_type %u section_factor %u",
                            iDataType, iSectionFactorType, oQuery.section_factor());
            oRsp.set_err_no(ERR_LACK_CLUSTER_INFO);
			oRsp.set_err_msg("no db config for the cluster info!");
            return(false);
        }
        else
        {
            snprintf(szFactor, 32, "%u:%u:%u", iDataType, iSectionFactorType, *c_section_iter);
            uint32 uiSectionTo = *c_section_iter;
            uint32 uiSectionFrom(0);
            if (--c_section_iter != c_factor_iter->second.end())uiSectionFrom = *c_section_iter + 1;

            //数据库实例配置2:4:1000000 : {"db_userinfo":"robot_db_instance_1","im_relation":"robot_db_instance_1","db_singlechat":"robot_db_instance_1","im_group":"robot_db_instance_1","im_custom":"custom_db"}
            std::map<std::string, util::CJsonObject*>::iterator conf_iter = m_mapDbInstanceInfo.find(szFactor);
            if (conf_iter == m_mapDbInstanceInfo.end())
            {
                LOG4_ERROR("no db config found for %s which consist of data_type %u section_factor_type %u section_factor %u",szFactor, iDataType, iSectionFactorType, oQuery.section_factor());
                oRsp.set_err_no(ERR_LACK_CLUSTER_INFO);
				oRsp.set_err_msg("no db config for the cluster info!");
                return(false);
            }
            std::string strDbName = m_oDbConf["table"][oQuery.db_operate().table_name()]("db_name");
            //数据库实例对应的数据类型、因子和分段
            LOG4_TRACE("db instance szFactor(%s),uiSectionTo(%u),uiSectionFrom(%u) strDbName(%s)",szFactor,uiSectionTo,uiSectionFrom,strDbName.c_str());
            std::string strInstance;
            if (!conf_iter->second->Get(strDbName, strInstance))
            {
            	LOG4_ERROR("no db instance config for strDbName \"%s\"!", strDbName.c_str());
            	oRsp.set_err_no(ERR_LACK_CLUSTER_INFO);
				oRsp.set_err_msg("no db instance config for db name!");
                return(false);
            }
            LOG4_DEBUG("strDbName(%s),strInstance(%s)",strDbName.c_str(),strInstance.c_str());
            util::CJsonObject& dbGroupConf = m_oDbConf["db_group"];
            if (oQuery.db_operate().query_type() > atoi(dbGroupConf[strInstance]("query_permit").c_str()))
            {
            	LOG4_ERROR("no right to excute oMemOperate.db_operate().query_type() %d!",oQuery.db_operate().query_type());
            	oRsp.set_err_no(ERR_NO_RIGHT);
				oRsp.set_err_msg("no right to excute oMemOperate.db_operate().query_type()!");
                return(false);
            }
            util::CJsonObject& oInstanceConf = dbGroupConf[strInstance];
            std::string strUseGroup = oInstanceConf("use_group");
            util::CJsonObject& oGroupHostConf = oInstanceConf[strUseGroup];
            {//主从
                std::string strMasterIdentify = oGroupHostConf("master_host");
                std::string strSlaveIdentify = oGroupHostConf("slave_host");
                LOG4_TRACE("strMasterIdentify(%s),strSlaveIdentify(%s)",strMasterIdentify.c_str(),strSlaveIdentify.c_str());
                bool bEstablishConnection = GetDbConnectionFromIdentify(oRsp,oQuery.db_operate().query_type(),strMasterIdentify, strSlaveIdentify,strDbName,
                                oInstanceConf, ppMasterDbi, ppSlaveDbi);
                return(bEstablishConnection);
            }
        }
    }
}

bool PgAgentSession::GetDbConnectionFromIdentify(DataMem::MemRsp &oRsp,DataMem::MemOperate::DbOperate::E_QUERY_TYPE eQueryType,
                const std::string& strMasterIdentify, const std::string& strSlaveIdentify,const std::string &strDbName,
                const util::CJsonObject& oInstanceConf, pqxx::connection** ppMasterDbi, pqxx::connection** ppSlaveDbi)
{
    LOG4_TRACE("%s(%s, %s,%s, %s)", __FUNCTION__, strMasterIdentify.c_str(), strSlaveIdentify.c_str(),strDbName.c_str(), oInstanceConf.ToString().c_str());
    *ppMasterDbi = NULL;
    *ppSlaveDbi = NULL;
    int iResult(0);
    std::string strMasterDbIdentify = strMasterIdentify +":"+strDbName;//ip:port:db
    std::string strSlaveDbIdentify = strSlaveIdentify +":"+strDbName;//ip:port:db
    std::map<std::string, tagConnection*>::iterator dbi_iter = m_mapDbiPool.find(strMasterDbIdentify);
    if (dbi_iter == m_mapDbiPool.end())
    {
        tagConnection* pConnection = new tagConnection();
        if (NULL == pConnection)
        {
        	oRsp.set_err_no(ERR_NEW);
        	oRsp.set_err_msg("malloc space for db connection failed!");
            return(false);
        }
        pqxx::connection* pPgConn(NULL);
        iResult = ConnectDb(oInstanceConf, pPgConn, strMasterDbIdentify);
        if (0 == iResult)
        {
        	pConnection->pPgConn = pPgConn;
            LOG4_TRACE("succeed in connecting strMasterIdentify(%s)", strMasterDbIdentify.c_str());
            *ppMasterDbi = pPgConn;
            pConnection->iQueryPermit = atoi(oInstanceConf("query_permit").c_str());
            pConnection->iTimeout = atoi(oInstanceConf("timeout").c_str());
            pConnection->ullBeatTime = time(NULL);
            m_mapDbiPool.insert(std::pair<std::string, tagConnection*>(strMasterDbIdentify, pConnection));
        }
        else
        {
            delete pConnection;
            pConnection = NULL;
        }
    }
    else
    {
        *ppMasterDbi = dbi_iter->second->pPgConn;
    }

    LOG4_TRACE("find slave %s.", strSlaveDbIdentify.c_str());
    dbi_iter = m_mapDbiPool.find(strSlaveDbIdentify);
    if (dbi_iter == m_mapDbiPool.end())
    {
        tagConnection* pConnection = new tagConnection();
        if (NULL == pConnection)
        {
        	oRsp.set_err_no(ERR_NEW);
			oRsp.set_err_msg("malloc space for db connection failed!");
            return(false);
        }
        pqxx::connection* pPgConn(NULL);
        iResult = ConnectDb(oInstanceConf, pPgConn, strSlaveDbIdentify);
        if (0 == iResult)
        {
        	pConnection->pPgConn = pPgConn;
            LOG4_TRACE("succeed in connecting strSlaveIdentify(%s)", strSlaveDbIdentify.c_str());
            *ppSlaveDbi = pPgConn;
            pConnection->iQueryPermit = atoi(oInstanceConf("query_permit").c_str());
            pConnection->iTimeout = atoi(oInstanceConf("timeout").c_str());
            pConnection->ullBeatTime = time(NULL);
            m_mapDbiPool.insert(std::pair<std::string, tagConnection*>(strSlaveDbIdentify, pConnection));
        }
        else
        {
            delete pConnection;
            pConnection = NULL;
        }
    }
    else
    {
        *ppSlaveDbi = dbi_iter->second->pPgConn;
    }

    LOG4_TRACE("pMasterDbi = 0x%x, pSlaveDbi = 0x%x.", *ppMasterDbi, *ppSlaveDbi);
    if (*ppMasterDbi || *ppSlaveDbi)
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

int PgAgentSession::ConnectDb(const util::CJsonObject& oInstanceConf, pqxx::connection* &pPgConn,const std::string& strDbIdentify)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    util::tagDbConfDetail stDbConfDetail;
    std::string strDbName;
    std::string::size_type nIndex = strDbIdentify.find(":");//ip:port:db
	if (nIndex != std::string::npos)
	{
		std::string strDbHost = strDbIdentify.substr(0,nIndex);
		strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbHost,strDbHost.c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbHost));
		std::string strRight = strDbIdentify.substr(nIndex + 1);
		nIndex = strRight.find(":");
		if (nIndex != std::string::npos)
		{
			std::string strDbPort = strRight.substr(0,nIndex);
			stDbConfDetail.m_stDbConnInfo.m_uiDbPort = atoi(strRight.c_str());

			strDbName = strRight.substr(nIndex + 1);
		}
		else
		{
			LOG4_ERROR("error strDbIdentify:%s", strDbIdentify.c_str());
			return -1;
		}
	}
	else
	{
		LOG4_ERROR("error strDbIdentify:%s", strDbIdentify.c_str());
		return -1;
	}
	strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbUser,oInstanceConf("user").c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbUser));
    strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbPwd,oInstanceConf("password").c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbPwd));
    strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbName, strDbName.c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbName));
    strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbCharSet,oInstanceConf("charset").c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbCharSet));
    stDbConfDetail.m_ucDbType = util::POSTGRESQL_DB;
    stDbConfDetail.m_ucAccess = 1; //直连
    stDbConfDetail.m_stDbConnInfo.uiTimeOut = atoi(oInstanceConf("timeout").c_str());

    LOG4_DEBUG("InitDbConn(%s, %s, %s, %s, %u, %s)", stDbConfDetail.m_stDbConnInfo.m_szDbHost,
                    stDbConfDetail.m_stDbConnInfo.m_szDbUser, stDbConfDetail.m_stDbConnInfo.m_szDbPwd,
                    stDbConfDetail.m_stDbConnInfo.m_szDbName, stDbConfDetail.m_stDbConnInfo.m_uiDbPort,
                    stDbConfDetail.m_stDbConnInfo.m_szDbCharSet);
    char connect[128];
    snprintf(connect,sizeof(connect),"dbname=%s hostaddr=%s user=%s password=%s port=%d",
    		stDbConfDetail.m_stDbConnInfo.m_szDbName,
    		stDbConfDetail.m_stDbConnInfo.m_szDbHost,
			stDbConfDetail.m_stDbConnInfo.m_szDbUser,
			stDbConfDetail.m_stDbConnInfo.m_szDbPwd,
			stDbConfDetail.m_stDbConnInfo.m_uiDbPort);
    //http://pqxx.org/devprojects/libpqxx/doc/4.0/html/Tutorial/
    //http://pqxx.org/devprojects/libpqxx/doc/4.0/html/Reference/modules.html
    try
    {
    	pPgConn = new pqxx::connection(connect);
		if(!pPgConn->is_open())
		{
			LOG4_ERROR("Connection failed!options:%s",pPgConn->options().c_str());
			delete pPgConn;
			pPgConn = NULL;
			return 1;
		}
    }
    catch (const pqxx::sql_error &e)
	{
		LOG4_ERROR("SQL error: %s.Query was:%s",e.what(),e.query().c_str());
		return 2;
	}
	catch (const std::exception &e)
	{
		LOG4_ERROR("SQL error: %s.Query was:%s",e.what());
		return 1;
	}
	//options:dbname=db_thunder_conf hostaddr=192.168.18.78 user=postgres password=postgres port=5402
	LOG4_TRACE("Connection succesful!options:%s",pPgConn->options().c_str());
	return 0;
}

int PgAgentSession::ConnectDb(const util::tagDbConfDetail &stDbConfDetail, pqxx::connection* &pPgConn)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    LOG4_DEBUG("InitDbConn(%s, %s, %s, %s, %u, %s)", stDbConfDetail.m_stDbConnInfo.m_szDbHost,
                    stDbConfDetail.m_stDbConnInfo.m_szDbUser, stDbConfDetail.m_stDbConnInfo.m_szDbPwd,
                    stDbConfDetail.m_stDbConnInfo.m_szDbName, stDbConfDetail.m_stDbConnInfo.m_uiDbPort,
                    stDbConfDetail.m_stDbConnInfo.m_szDbCharSet);
    char connect[128];
	snprintf(connect,sizeof(connect),"dbname=%s hostaddr=%s user=%s password=%s port=%d",
			stDbConfDetail.m_stDbConnInfo.m_szDbName,
			stDbConfDetail.m_stDbConnInfo.m_szDbHost,
			stDbConfDetail.m_stDbConnInfo.m_szDbUser,
			stDbConfDetail.m_stDbConnInfo.m_szDbPwd,
			stDbConfDetail.m_stDbConnInfo.m_uiDbPort);
	try
	{
		pPgConn = new pqxx::connection(connect);
		if(!pPgConn->is_open())
		{
			LOG4_ERROR("Connection failed!options:%s",pPgConn->options().c_str());
			delete pPgConn;
			pPgConn = NULL;
			return 1;
		}
	}
	catch (const pqxx::sql_error &e)
	{
		LOG4_ERROR("SQL error: %s.Query was:%s",e.what(),e.query().c_str());
		return 2;
	}
	catch (const std::exception &e)
	{
		LOG4_ERROR("SQL error: %s.Query was:%s",e.what());
		return 1;
	}
	LOG4_INFO("Connection succesful!options:%s",pPgConn->options().c_str());
	return 0;
}

void PgAgentSession::CheckConn()
{
    LOG4_TRACE("%s()",__FUNCTION__);
//    time_t lNowTime = time(NULL);
    //长连接。检查mapdbi中的连接实例有无超时的，超时的连接删除
//    std::map<std::string,tagConnection*>::iterator conn_iter;
//    for (conn_iter = m_mapDbiPool.begin(); conn_iter != m_mapDbiPool.end(); )
//    {
//        int iTimeOut = m_iConnectionTimeout;
//        if (conn_iter->second->iTimeout > 0)
//        {
//            iTimeOut = conn_iter->second->iTimeout;
//        }
//
//        int iDiffTime = lNowTime - conn_iter->second->ullBeatTime;  //求时差
//        if (iDiffTime > iTimeOut)
//        {
//            if (NULL != conn_iter->second)
//            {
//                delete conn_iter->second;
//                conn_iter->second = NULL;
//            }
//            m_mapDbiPool.erase(conn_iter);
//        }
//        else
//        {
//            ++conn_iter;
//        }
//    }
}

void PgAgentSession::RunResult()
{
	if (m_RunClock.LastUseTime() >= 1000)LOG4_INFO("%s() use time(%lf) ms",__FUNCTION__,m_RunClock.LastUseTime());
	else LOG4_TRACE("%s() use time(%lf) ms",__FUNCTION__,m_RunClock.LastUseTime());
	if (m_SqlRate.m_uiCounter % 1000 == 0)
	{
		LOG4_INFO("%s() NewCounter(%llu) uiCounter(%llu,%llu)",__FUNCTION__,m_SqlRate.NewCounter(),m_SqlRate.m_uiCounter,m_SqlRate.m_uiLastCounter);
	}
}

uint32 PgAgentSession::ExecuteSql(pqxx::connection* pPgConn,const std::string &strSql,pqxx::result &result,std::string &strErr)
{
	m_RunClock.StartClock();m_SqlRate.IncrCounter();
	uint32 iResult(0);
	try
	{
		pqxx::work W(*pPgConn);
		result = W.exec( strSql );
		W.commit();
	}
	catch (const pqxx::sql_error &e)
	{
		snprintf(sError,sizeof(sError)-1,"SQL error: %s.Query was:%s",e.what(),e.query().c_str());
		LOG4_ERROR(sError);
		strErr = sError;
		iResult = 2;
	}
	catch (const std::exception &e)
	{
		snprintf(sError,sizeof(sError)-1,"SQL error: %s.",e.what());
		LOG4_ERROR(sError);
		strErr = sError;
		iResult = 1;
	}
	m_RunClock.EndClock();RunResult();
	return iResult;
}

void PgAgentSession::StackSqlPineline(pqxx::connection* pPgConn,const std::string &strSql)
{
	auto iter = m_PinelineStrSqls.find(pPgConn);
	if (iter == m_PinelineStrSqls.end())
	{
		std::list<std::string> l;
		l.push_back(strSql);
		m_PinelineStrSqls.insert(std::make_pair(pPgConn,l));
	}
	else
	{
		iter->second.push_back(strSql);
	}
}
uint32 PgAgentSession::ExecuteSqlPineline(pqxx::connection* pPgConn,std::list<std::string> &strSqls,uint32 maxnum,uint32 maxsize,std::string &strErr,bool boNeedResult)
{
	m_PinelineLastResults.clear();
	m_PinelineLastSqls.clear();
	if (strSqls.size() == 0)return 0;
	m_RunClock.StartClock();m_SqlRate.IncrCounter();
	uint32 iResult(0),num(0),sqlsize(0);
	uint32 totalnum = (maxnum == 0 || maxnum > strSqls.size()) ? strSqls.size() :maxnum;
	try
	{
		pqxx::work W(*pPgConn);
		pqxx::pipeline P(W, "PgAgentSession");
		P.retain(totalnum);
		for(;strSqls.size() && num < totalnum && sqlsize < maxsize;++num)
		{
			sqlsize += strSqls.front().size();
			m_PinelineLastSqls.push_back(strSqls.front());
			P.insert(strSqls.front());
			strSqls.pop_front();
		}
		P.resume();
		P.complete();
		W.commit();
		if (boNeedResult)
		{
			while(!P.empty())m_PinelineLastResults.push_back(P.retrieve());
		}
	}
	catch (const pqxx::sql_error &e)
	{
		snprintf(sError,sizeof(sError)-1,"SQL error: %s.Query was:%s",e.what(),e.query().c_str());
		LOG4_ERROR(sError);
		strErr = sError;
		iResult = 2;
	}
	catch (const std::exception &e)
	{
		snprintf(sError,sizeof(sError)-1,"SQL error: %s.",e.what());
		LOG4_ERROR(sError);
		strErr = sError;
		iResult = 1;
	}
	if (num >= maxnum || sqlsize >= maxsize)LOG4_INFO("num(%u,%u) sqlsize(%u,%u)",num,maxnum,sqlsize,maxsize);
	else LOG4_TRACE("num(%u,%u) sqlsize(%u,%u)",num,maxnum,sqlsize,maxsize);
	m_RunClock.EndClock();RunResult();
	return iResult;
}

bool PgAgentSession::QueryRaw(DataMem::MemRsp &oRsp,const DataMem::MemOperate& oQuery, pqxx::connection* pPgConn)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (NULL == pPgConn)
    {
        LOG4_ERROR("pPgConn is null!");
        oRsp.set_err_no(ERR_QUERY);
		oRsp.set_err_msg("pPgConn is null");
        return false;
    }
    std::string strSql;
    if (!CreateSql(oQuery, pPgConn, strSql))
    {
        LOG4_ERROR("Scrabble up sql error!");
        oRsp.set_err_no(ERR_QUERY);
		oRsp.set_err_msg(strSql);
        return false;
    }
    LOG4_DEBUG("%s", strSql.c_str());
    if (strSql.size() == 0)//wait to bulk oper
    {
    	LOG4_DEBUG("strSql wait to bulk");
    	return true;
    }
	pqxx::result result;
	std::string strErr;
	uint32 iResult = ExecuteSql(pPgConn,strSql,result,strErr);
	if (iResult > 0)
	{
		// 由于连接方面原因数据写失败，将失败数据节点返回给数据代理，等服务从故障中恢复后再由数据代理自动重试
		oRsp.set_err_no(iResult);
		oRsp.set_err_msg(strErr);
		return false;
	}
	if (DataMem::MemOperate::DbOperate::SELECT == oQuery.db_operate().query_type())
	{
		DataMem::Record* pRecord = NULL;
		DataMem::Field* pField = NULL;
		// Results can be accessed and iterated again.  Even after the connection
		// has been closed.
//			for (auto row: result)
//			{
//			  std::cout << "Row: ";
//			  // Iterate over fields in a row.
//			  for (auto field: row) std::cout << field.c_str() << " ";
//			  std::cout << std::endl;
//			}
		uint32 uiDataLen = 0;
		int32 iRecordNum = 0;
		//字段值进行赋值
		oRsp.set_err_no(ERR_OK);
		oRsp.set_err_msg("success");
		for (const auto& row: result)
		{
			++iRecordNum;
			pRecord = oRsp.add_record_data();

			unsigned int uiFieldNum = row.size();
			for(unsigned int i = 0; i < uiFieldNum; ++i)
			{
				pField = pRecord->add_field_info();
				pField->set_col_value(row[i].c_str(),row[i].size());//row[1].as<int>()
				uiDataLen += row[i].size();
			}
			if (uiDataLen > 64000000)//64M
			{
				oRsp.set_curcount(iRecordNum);
				oRsp.set_totalcount(iRecordNum + 1);    // 表示未完
				return true;//由逻辑控制之后的请求 oRsp.clear_record_data();uiDataLen = 0;
			}
		}
		oRsp.set_curcount(iRecordNum);
		oRsp.set_totalcount(iRecordNum);
	}
	else
	{
		oRsp.set_err_no(ERR_OK);
		oRsp.set_err_msg("success");
		std::string strAffected = std::to_string(result.affected_rows());//修改行数
		DataMem::Record* pRecord = oRsp.add_record_data();
		DataMem::Field* pField = pRecord->add_field_info();
		pField->set_col_value(strAffected.c_str(),strAffected.size());
	}
	return true;
}


bool PgAgentSession::QueryOper(DataMem::MemRsp &oRsp,const DataMem::MemOperate& oQuery)
{
	m_MsgRate.IncrCounter();
	pqxx::connection* pMasterDbi(NULL);pqxx::connection* pSlaveDbi(NULL);
	if (!GetDbConnection(oRsp,oQuery, &pMasterDbi, &pSlaveDbi))
	{
		LOG4_WARN("failed to get db connection");
		return false;
	}
	LOG4_TRACE("succeed in getting db connection");
	if (DataMem::MemOperate::DbOperate::SELECT == oQuery.db_operate().query_type())
	{
		if (pSlaveDbi)
		{
			if (QueryRaw(oRsp,oQuery,pSlaveDbi))return true;
		}
		if (pMasterDbi)
		{
			return QueryRaw(oRsp,oQuery,pMasterDbi);
		}
	}
	else
	{
		return QueryRaw(oRsp,oQuery,pMasterDbi);
	}
	return false;
}

bool PgAgentSession::LocatePgConfig(DataMem::MemOperate& oQuery,int& nCode,std::string &strErrMsg,util::CJsonObject &oRspJson)
{
	char szFactor[32] = {0};
	int32 iDataType = 0;
	int32 iSectionFactorType = 0;
	m_oDbConf["table"][oQuery.db_operate().table_name()].Get("data_type", iDataType);
	m_oDbConf["table"][oQuery.db_operate().table_name()].Get("section_factor", iSectionFactorType);
	snprintf(szFactor, 32, "%d:%d", iDataType, iSectionFactorType);
	std::map<std::string, std::set<uint32> >::const_iterator c_factor_iter =  m_mapFactorSection.find(szFactor);
	if (c_factor_iter == m_mapFactorSection.end())
	{
		LOG4_ERROR("no db config found for data_type %d section_factor_type %d", iDataType, iSectionFactorType);
		nCode = ERR_LACK_CLUSTER_INFO; strErrMsg = "no db config found for oMemOperate.cluster_info()!";
		return(false);
	}
	else
	{
		std::set<uint32>::const_iterator c_section_iter = c_factor_iter->second.lower_bound(oQuery.section_factor());
		if (c_section_iter == c_factor_iter->second.end())
		{
			LOG4_ERROR("no factor_section config found for data_type %u section_factor_type %u section_factor %u",
							iDataType, iSectionFactorType, oQuery.section_factor());
			nCode = ERR_LACK_CLUSTER_INFO; strErrMsg = "no db config for the cluster info!";
			return(false);
		}
		else
		{
			snprintf(szFactor, 32, "%u:%u:%u", iDataType, iSectionFactorType, *c_section_iter);
			std::map<std::string, util::CJsonObject*>::iterator conf_iter = m_mapDbInstanceInfo.find(szFactor);
			if (conf_iter == m_mapDbInstanceInfo.end())
			{
				LOG4_ERROR("no db config found for %s which consist of data_type %u section_factor_type %u section_factor %u",
								szFactor, iDataType, iSectionFactorType, oQuery.section_factor());
				nCode = ERR_LACK_CLUSTER_INFO; strErrMsg = "no db config for the cluster info!";
				return(false);
			}
			std::string strDbName = m_oDbConf["table"][oQuery.db_operate().table_name()]("db_name");
			std::string strInstance;
			if (!conf_iter->second->Get(strDbName, strInstance))
			{
				LOG4_ERROR("no db instance config for strDbName \"%s\"!", strDbName.c_str());
				nCode = ERR_LACK_CLUSTER_INFO; strErrMsg = "no db instance config for db name!";
				return(false);
			}
			util::CJsonObject& dbGroupConf = m_oDbConf["db_group"];
			util::CJsonObject& dbInstanceConf = dbGroupConf[strInstance];
			std::string strUseGroup = dbInstanceConf("use_group");
			util::CJsonObject& oGroupHostConf = dbInstanceConf[strUseGroup];
			int nArraySize = oGroupHostConf.GetArraySize();
			std::string strMasterIdentify;
			std::string strSlaveIdentify;
			if (nArraySize > 0)//cluster集群
			{
				strMasterIdentify = oGroupHostConf[0].ToString();//只返回第一个节点
				strSlaveIdentify = oGroupHostConf[0].ToString();
			}
			else//主从
			{
				strMasterIdentify = dbInstanceConf("master_host");
				strSlaveIdentify = dbInstanceConf("slave_host");
			}

			std::string strTableName = GetFullTableName(oQuery.db_operate().table_name(), oQuery.db_operate().mod_factor());
			if (strTableName.empty())
			{
				LOG4_ERROR("dbname_table is NULL");
				return false;
			}
			nCode = ERR_OK; strErrMsg = "successfully";
			oRspJson.Add("db_node", util::CJsonObject("{}"));
			oRspJson["db_node"].Add("master", strMasterIdentify);
			oRspJson["db_node"].Add("slave", strSlaveIdentify);
			oRspJson["db_node"].Add("table", strTableName);
			return(true);
		}
	}
}

std::string PgAgentSession::GetFullTableName(const std::string& strTableName, uint64 uiFactor)
{
    char szFullTableName[128] = {0};
    std::string strDbName = m_oDbConf["table"][strTableName]("db_name");
    if (strDbName.size() > 0)
    {
    	int iTableNum = atoi(m_oDbConf["table"][strTableName]("table_num").c_str());
		if (1 == iTableNum)
		{
			//snprintf(szFullTableName, sizeof(szFullTableName), "%s.%s", strDbName.c_str(), strTableName.c_str());
			snprintf(szFullTableName, sizeof(szFullTableName), "%s",strTableName.c_str());
		}
		else
		{
			uint32 uiTableIndex = uiFactor % iTableNum;
			//snprintf(szFullTableName, sizeof(szFullTableName), "%s.%s_%02d", strDbName.c_str(), strTableName.c_str(), uiTableIndex);
			snprintf(szFullTableName, sizeof(szFullTableName), "%s_%02d", strTableName.c_str(), uiTableIndex);
		}
    }
    return(szFullTableName);
}

bool PgAgentSession::CreateSql(const DataMem::MemOperate& oQuery, pqxx::connection* pPgConn, std::string& strSql)
{
    LOG4_TRACE("%s()",__FUNCTION__);
    strSql.clear();
    if (oQuery.db_operate().query_type() == DataMem::MemOperate::DbOperate::CUSTOM)
    {
    	if (oQuery.db_operate().fields_size() <= 0)
		{
			LOG4_ERROR("invalid fields_size(%d) for CUSTOM",oQuery.db_operate().fields_size());
			return false;
		}
		//自定义sql则不执行检查（只适用于开发者自己知道存储分表结构）
		if (oQuery.db_operate().fields(0).col_name().size() > 0)
		{
			strSql = oQuery.db_operate().fields(0).col_name();
		}
		else if (oQuery.db_operate().fields(0).col_value().size() > 0)
		{
			strSql = oQuery.db_operate().fields(0).col_value();
		}
		else
		{
			LOG4_ERROR("invalid oQuery.db_operate().fields(0) empty for CUSTOM");
			return false;
		}
		if ("PGAGENT_JOB" == net::GetNodeType())
		{
			return true;
		}
		m_MsgCustomRate.IncrCounter();
		LOG4_TRACE("CUSTOM strSql:%s",strSql.c_str());
		if (m_uiPipelineSqlEnable)//pineline
		{
			StackSqlPineline(pPgConn,strSql);
			strSql.clear();//直接返回响应
			return true;
		}
		if (m_uiBulkSqlEnable)//缓存
		{
			m_listCustomSqls.push_back(SqlValues(pPgConn,strSql));
			strSql.clear();//直接返回响应
			return true;
		}
		return true;
    }
    if (oQuery.db_operate().query_type() == DataMem::MemOperate::DbOperate::BULK)
	{
    	if (oQuery.db_operate().fields_size() <= 0)
		{
			LOG4_ERROR("invalid fields_size(%d) for BULK",oQuery.db_operate().fields_size());
			return false;
		}
		//自定义sql则不执行检查（只适用于开发者自己知道存储分表结构）
		if (oQuery.db_operate().fields(0).col_name().size() > 0)
		{
			strSql = oQuery.db_operate().fields(0).col_name();
		}
		else if (oQuery.db_operate().fields(0).col_value().size() > 0)
		{
			strSql = oQuery.db_operate().fields(0).col_value();
		}
		else
		{
			LOG4_ERROR("invalid oQuery.db_operate().fields(0) empty for BULK");
			return false;
		}
		std::size_t found = strSql.find("values");
		if (found == std::string::npos)
		{
			LOG4_WARN("invalid strSql for BULK");
			return true;
		}
		m_MsgBulkRate.IncrCounter();
		LOG4_TRACE("BULK strSql:%s",strSql.c_str());
		if (m_uiPipelineSqlEnable)//pineline
		{
			StackSqlPineline(pPgConn,strSql);
			strSql.clear();//直接返回响应
			return true;
		}
		if (m_uiBulkSqlEnable)//缓存
		{
			int len = strlen("values");
			std::string tableOper = strSql.substr(0,found+len);//insert into(a,b) values
			std::string operValues = strSql.substr(found+len);//(1,2),(1,2)
			auto iter = m_mapBulkSqls.find(tableOper);
			if (iter != m_mapBulkSqls.end())
			{
				if (m_uiBulkSqlMaxSqlSize && iter->second.back().sqlvalues.size() >= m_uiBulkSqlMaxSqlSize)//sql最大大小
				{
					SqlValues sqlValues(pPgConn,operValues + ",");
					iter->second.push_back(sqlValues);
				}
				else
				{
					iter->second.back().sqlvalues.append(operValues + ",");
				}
			}
			else
			{
				SqlValues sqlValues(pPgConn,operValues + ",");
				std::list<SqlValues> list;
				list.push_back(sqlValues);
				m_mapBulkSqls.insert(std::make_pair(tableOper,list));
			}
			strSql.clear();//直接返回响应
			return true;
		}
		return true;
	}
    if (oQuery.db_operate().table_name().size() == 0)
    {
    	LOG4_ERROR("invalid oQuery.db_operate().table_name().size() == 0");
		return false;
    }
    bool bResult = false;
    switch (oQuery.db_operate().query_type())
    {
        case DataMem::MemOperate::DbOperate::SELECT:
        {
        	if (oQuery.db_operate().fields_size() <= 0)
			{
				LOG4_ERROR("invalid fields_size(%d) for SELECT",oQuery.db_operate().fields_size());
				return false;
			}
            bResult = CreateSelect(oQuery, strSql);
            break;
        }
        case DataMem::MemOperate::DbOperate::INSERT:
        case DataMem::MemOperate::DbOperate::INSERT_IGNORE:
        case DataMem::MemOperate::DbOperate::REPLACE:
        {
        	if (oQuery.db_operate().fields_size() <= 0)
			{
				LOG4_ERROR("invalid fields_size(%d) for UPDATE",oQuery.db_operate().fields_size());
				return false;
			}
            bResult = CreateInsert(oQuery, pPgConn, strSql);
            break;
        }
        case DataMem::MemOperate::DbOperate::UPDATE:
        {
        	if (oQuery.db_operate().fields_size() <= 0)
			{
				LOG4_ERROR("invalid fields_size(%d) for UPDATE",oQuery.db_operate().fields_size());
				return false;
			}
            bResult = CreateUpdate(oQuery, pPgConn, strSql);
            break;
        }
        case DataMem::MemOperate::DbOperate::DELETE:
        {
            bResult = CreateDelete(oQuery, strSql);
            break;
        }
        case DataMem::MemOperate::DbOperate::E_QUERY_TYPE_UNKNONW:
        {
            LOG4_ERROR("invalid query_type(%d)",oQuery.db_operate().query_type());
            return(false);
        }
        default:
        {
            LOG4_ERROR("invalid query_type(%d)",oQuery.db_operate().query_type());
            return(false);
        }
    }

    if (oQuery.db_operate().conditions_size() > 0)
    {
        std::string strCondition;
        bResult = CreateConditionGroup(oQuery, pPgConn, strCondition);
        if (bResult)
        {
            strSql += std::string(" WHERE ") + strCondition;
        }
    }

    if (oQuery.db_operate().groupby_col_size() > 0)
    {
        std::string strGroupBy;
        bResult = CreateGroupBy(oQuery, strGroupBy);
        if (bResult)
        {
            strSql += std::string(" GROUP BY ") + strGroupBy;
        }
    }

    if (oQuery.db_operate().orderby_col_size() > 0)
    {
        std::string strOrderBy;
        bResult = CreateOrderBy(oQuery, strOrderBy);
        if (bResult)
        {
            strSql += std::string(" ORDER BY ") + strOrderBy;
        }
    }

    if (oQuery.db_operate().limit() > 0)
    {
        std::string strLimit;
        bResult = CreateLimit(oQuery, strLimit);
        if (bResult)
        {
            strSql += std::string(" LIMIT ") + strLimit;
        }
    }

    return(bResult);
}

bool PgAgentSession::CreateSelect(const DataMem::MemOperate& oQuery, std::string& strSql)
{
    strSql = "SELECT ";
    for (int i = 0; i < oQuery.db_operate().fields_size(); ++i)
    {
        if (!CheckColName(oQuery.db_operate().fields(i).col_name()))
        {
            LOG4_ERROR("invalid col_name \"%s\".", oQuery.db_operate().fields(i).col_name().c_str());
            return(false);
        }
        if (i == 0)
        {
            if (oQuery.db_operate().fields(i).col_as().size() > 0)
            {
                strSql += oQuery.db_operate().fields(i).col_name() + std::string(" AS ") + oQuery.db_operate().fields(i).col_as();
            }
            else
            {
                strSql += oQuery.db_operate().fields(i).col_name();
            }
        }
        else
        {
            if (oQuery.db_operate().fields(i).col_as().size() > 0)
            {
                strSql += std::string(",") + oQuery.db_operate().fields(i).col_name() + std::string(" AS ") + oQuery.db_operate().fields(i).col_as();
            }
            else
            {
                strSql += std::string(",") + oQuery.db_operate().fields(i).col_name();
            }
        }
    }

    std::string strTableName = GetFullTableName(oQuery.db_operate().table_name(), oQuery.db_operate().mod_factor());
    if (strTableName.empty())
    {
        LOG4_ERROR("dbname_table is NULL");
        return false;
    }

    strSql += std::string(" FROM ") + strTableName;

    return true;
}

bool PgAgentSession::CreateInsert(const DataMem::MemOperate& oQuery, pqxx::connection* pPgConn, std::string& strSql)
{
    LOG4_TRACE("%s()",__FUNCTION__);

    strSql = "";
    if (DataMem::MemOperate::DbOperate::INSERT == oQuery.db_operate().query_type())
    {
        strSql = "INSERT INTO ";
    }
    else if (DataMem::MemOperate::DbOperate::INSERT_IGNORE == oQuery.db_operate().query_type())
    {
        strSql = "INSERT IGNORE INTO ";
    }
    else if (DataMem::MemOperate::DbOperate::REPLACE == oQuery.db_operate().query_type())
    {
        strSql = "REPLACE INTO ";
    }

    std::string strTableName = GetFullTableName(oQuery.db_operate().table_name(), oQuery.db_operate().mod_factor());
    if (strTableName.empty())
    {
        LOG4_ERROR("dbname_tbname is null");
        return false;
    }

    strSql += strTableName;

    for (int i = 0; i < oQuery.db_operate().fields_size(); ++i)
    {
        if (!CheckColName(oQuery.db_operate().fields(i).col_name()))
        {
            return(false);
        }
        if (i == 0)
        {
            strSql += std::string("(") + oQuery.db_operate().fields(i).col_name();
        }
        else
        {
            strSql += std::string(",") + oQuery.db_operate().fields(i).col_name();
        }
    }
    strSql += std::string(") ");

    for (int i = 0; i < oQuery.db_operate().fields_size(); ++i)
    {
        if (i == 0)
        {
            if (DataMem::STRING == oQuery.db_operate().fields(i).col_type())
            {
            	m_strColValue = pPgConn->esc(oQuery.db_operate().fields(i).col_value());
                strSql += std::string("VALUES('") + std::string(m_strColValue) + std::string("'");
            }
            else
            {
                for (unsigned int j = 0; j < oQuery.db_operate().fields(i).col_value().size(); ++j)
                {
                    if (oQuery.db_operate().fields(i).col_value()[j] >= '0' || oQuery.db_operate().fields(i).col_value()[j] <= '9'
                        || oQuery.db_operate().fields(i).col_value()[j] == '.')
                    {
                        ;
                    }
                    else
                    {
                        return(false);
                    }
                }
                strSql += std::string("VALUES(") + oQuery.db_operate().fields(i).col_value();
            }
        }
        else
        {
            if (DataMem::STRING == oQuery.db_operate().fields(i).col_type())
            {
            	m_strColValue = pPgConn->esc(oQuery.db_operate().fields(i).col_value());
                strSql += std::string(",'") + std::string(m_strColValue) + std::string("'");
            }
            else
            {
                for (unsigned int j = 0; j < oQuery.db_operate().fields(i).col_value().size(); ++j)
                {
                    if (oQuery.db_operate().fields(i).col_value()[j] >= '0' || oQuery.db_operate().fields(i).col_value()[j] <= '9'
                        || oQuery.db_operate().fields(i).col_value()[j] == '.')
                    {
                        ;
                    }
                    else
                    {
                        return(false);
                    }
                }
                strSql += std::string(",") + oQuery.db_operate().fields(i).col_value();
            }
        }
    }
    strSql += std::string(")");

    return true;
}

bool PgAgentSession::CreateUpdate(const DataMem::MemOperate& oQuery, pqxx::connection* pPgConn, std::string& strSql)
{
    LOG4_TRACE("%s()",__FUNCTION__);

    strSql = "UPDATE ";
    std::string strTableName = GetFullTableName(oQuery.db_operate().table_name(), oQuery.db_operate().mod_factor());
    if (strTableName.empty())
    {
        LOG4_ERROR("dbname_tbname is null");
        return false;
    }

    strSql += strTableName;

    for (int i = 0; i < oQuery.db_operate().fields_size(); ++i)
    {
        if (!CheckColName(oQuery.db_operate().fields(i).col_name()))
        {
            return(false);
        }
        if (i == 0)
        {
            if (DataMem::STRING == oQuery.db_operate().fields(i).col_type())
            {
            	m_strColValue = pPgConn->esc(oQuery.db_operate().fields(i).col_value());
                strSql += std::string(" SET ") + oQuery.db_operate().fields(i).col_name() + std::string("=");
                strSql += std::string("'") + std::string(m_strColValue) + std::string("'");
            }
            else
            {
                for (unsigned int j = 0; j < oQuery.db_operate().fields(i).col_value().size(); ++j)
                {
                    if (oQuery.db_operate().fields(i).col_value()[j] >= '0' || oQuery.db_operate().fields(i).col_value()[j] <= '9'
                        || oQuery.db_operate().fields(i).col_value()[j] == '.')
                    {
                        ;
                    }
                    else
                    {
                        return(false);
                    }
                }
                strSql += std::string(" SET ") + oQuery.db_operate().fields(i).col_name()
                    + std::string("=") + oQuery.db_operate().fields(i).col_value();
            }
        }
        else
        {
            if (DataMem::STRING == oQuery.db_operate().fields(i).col_type())
            {
            	m_strColValue = pPgConn->esc(oQuery.db_operate().fields(i).col_value());
                strSql += std::string(", ") + oQuery.db_operate().fields(i).col_name() + std::string("=");
                strSql += std::string("'") + std::string(m_strColValue) + std::string("'");
            }
            else
            {
                for (unsigned int j = 0; j < oQuery.db_operate().fields(i).col_value().size(); ++j)
                {
                    if (oQuery.db_operate().fields(i).col_value()[j] >= '0' || oQuery.db_operate().fields(i).col_value()[j] <= '9'
                        || oQuery.db_operate().fields(i).col_value()[j] == '.')
                    {
                        ;
                    }
                    else
                    {
                        return(false);
                    }
                }
                strSql += std::string(", ") + oQuery.db_operate().fields(i).col_name()
                    + std::string("=") + oQuery.db_operate().fields(i).col_value();
            }
        }
    }

    return true;
}

bool PgAgentSession::CreateDelete(const DataMem::MemOperate& oQuery, std::string& strSql)
{
    LOG4_TRACE("%s()",__FUNCTION__);

    strSql = "DELETE FROM ";
    std::string strTableName = GetFullTableName(oQuery.db_operate().table_name(), oQuery.db_operate().mod_factor());
    if (strTableName.empty())
    {
        LOG4_ERROR("dbname_tbname is null");
        return false;
    }

    strSql += strTableName;

    return true;
}

bool PgAgentSession::CreateCondition(const DataMem::MemOperate::DbOperate::Condition& oCondition, pqxx::connection* pPgConn, std::string& strCondition)
{
    LOG4_TRACE("%s()",__FUNCTION__);

    if (!CheckColName(oCondition.col_name()))
    {
        return(false);
    }
    strCondition = oCondition.col_name();
    switch (oCondition.relation())
    {
    case DataMem::MemOperate::DbOperate::Condition::EQ:
        strCondition += std::string("=");
        break;
    case DataMem::MemOperate::DbOperate::Condition::NE:
        strCondition += std::string("<>");
        break;
    case DataMem::MemOperate::DbOperate::Condition::GT:
        strCondition += std::string(">");
        break;
    case DataMem::MemOperate::DbOperate::Condition::LT:
        strCondition += std::string("<");
        break;
    case DataMem::MemOperate::DbOperate::Condition::GE:
        strCondition += std::string(">=");
        break;
    case DataMem::MemOperate::DbOperate::Condition::LE:
        strCondition += std::string("<=");
        break;
    case DataMem::MemOperate::DbOperate::Condition::LIKE:
        strCondition += std::string(" LIKE ");
        break;
    case DataMem::MemOperate::DbOperate::Condition::IN:
        strCondition += std::string(" IN (");
        break;
    default:
        break;
    }

    if (oCondition.col_name_right().size() > 0)
    {
        if (!CheckColName(oCondition.col_name_right()))
        {
            return(false);
        }
        strCondition += oCondition.col_name_right();
    }
    else
    {
        for (int i = 0; i < oCondition.col_values_size(); ++i)
        {
            if (i > 0)
            {
                strCondition += std::string(",");
            }

            if (DataMem::STRING == oCondition.col_type())
            {
            	m_strColValue = pPgConn->esc(oCondition.col_values(i));
                strCondition += std::string("'") + std::string(m_strColValue) + std::string("'");
            }
            else
            {
                for (unsigned int j = 0; j < oCondition.col_values(i).size(); ++j)
                {
                    if (oCondition.col_values(i)[j] >= '0' || oCondition.col_values(i)[j] <= '9'
                        || oCondition.col_values(i)[j] == '.')
                    {
                        ;
                    }
                    else
                    {
                        return(false);
                    }
                }
                strCondition += oCondition.col_values(i);
            }
        }

        if (DataMem::MemOperate::DbOperate::Condition::IN == oCondition.relation())
        {
            strCondition += std::string(")");
        }
    }

    return true;
}

bool PgAgentSession::CreateConditionGroup(const DataMem::MemOperate& oQuery, pqxx::connection* pPgConn, std::string& strCondition)
{
    LOG4_TRACE("%s()",__FUNCTION__);

    bool bResult = false;
    for (int i = 0; i < oQuery.db_operate().conditions_size(); ++i)
    {
        if (i > 0)
        {
            if (oQuery.db_operate().group_relation() > 0)
            {
                if (DataMem::MemOperate::DbOperate::ConditionGroup::OR == oQuery.db_operate().group_relation())
                {
                    strCondition += std::string(" OR ");
                }
                else
                {
                    strCondition += std::string(" AND ");
                }
            }
            else
            {
                strCondition += std::string(" AND ");
            }
        }
        if (oQuery.db_operate().conditions_size() > 1)
        {
            strCondition += std::string("(");
        }
        for (int j = 0; j < oQuery.db_operate().conditions(i).condition_size(); ++j)
        {
            std::string strRelation;
            std::string strOneCondition;
            if (DataMem::MemOperate::DbOperate::ConditionGroup::OR == oQuery.db_operate().conditions(i).relation())
            {
                strRelation = " OR ";
            }
            else
            {
                strRelation = " AND ";
            }
            bResult = CreateCondition(oQuery.db_operate().conditions(i).condition(j), pPgConn, strOneCondition);
            if (bResult)
            {
                if (j > 0)
                {
                    strCondition += strRelation;
                }
                strCondition += strOneCondition;
            }
            else
            {
                return(bResult);
            }
        }
        if (oQuery.db_operate().conditions_size() > 1)
        {
            strCondition += std::string(")");
        }
    }

    return true;
}

bool PgAgentSession::CreateGroupBy(const DataMem::MemOperate& oQuery, std::string& strGroupBy)
{
    LOG4_TRACE("%s()",__FUNCTION__);

    strGroupBy = "";
    for (int i = 0; i < oQuery.db_operate().groupby_col_size(); ++i)
    {
        if (!CheckColName(oQuery.db_operate().groupby_col(i)))
        {
            return(false);
        }
        if (i == 0)
        {
            strGroupBy += oQuery.db_operate().groupby_col(i);
        }
        else
        {
            strGroupBy += std::string(",") + oQuery.db_operate().groupby_col(i);
        }
    }

    return true;
}

bool PgAgentSession::CreateOrderBy(const DataMem::MemOperate& oQuery, std::string& strOrderBy)
{
    LOG4_TRACE("%s()",__FUNCTION__);

    strOrderBy = "";
    for (int i = 0; i < oQuery.db_operate().orderby_col_size(); ++i)
    {
        if (!CheckColName(oQuery.db_operate().orderby_col(i).col_name()))
        {
            return(false);
        }
        if (i == 0)
        {
            if (DataMem::MemOperate::DbOperate::OrderBy::DESC == oQuery.db_operate().orderby_col(i).relation())
            {
                strOrderBy += oQuery.db_operate().orderby_col(i).col_name() + std::string(" DESC");
            }
            else
            {
                strOrderBy += oQuery.db_operate().orderby_col(i).col_name() + std::string(" ASC");
            }
        }
        else
        {
            if (DataMem::MemOperate::DbOperate::OrderBy::DESC == oQuery.db_operate().orderby_col(i).relation())
            {
                strOrderBy += std::string(",") + oQuery.db_operate().orderby_col(i).col_name() + std::string(" DESC");
            }
            else
            {
                strOrderBy += std::string(",") + oQuery.db_operate().orderby_col(i).col_name() + std::string(" ASC");
            }
        }
    }

    return true;
}

bool PgAgentSession::CreateLimit(const DataMem::MemOperate& oQuery, std::string& strLimit)
{
    LOG4_TRACE("%s()",__FUNCTION__);
    char szLimit[16] = {0};
    if (oQuery.db_operate().limit_from() > 0 && oQuery.db_operate().limit() > 0)
    {
        snprintf(szLimit, sizeof(szLimit), "%u,%u", oQuery.db_operate().limit_from(), oQuery.db_operate().limit());
        strLimit = szLimit;
    }
    else
    {
        snprintf(szLimit, sizeof(szLimit), "%u", oQuery.db_operate().limit());
        strLimit = szLimit;
    }
    return true;
}

bool PgAgentSession::CheckColName(const std::string& strColName)
{
    std::string strUpperColName;
    for (unsigned int i = 0; i < strColName.size(); ++i)
    {
//        if (strColName[i] == '\'' || strColName[i] == '"' || strColName[i] == '\\'
//            || strColName[i] == ';' || strColName[i] == '*' || strColName[i] == ' ')
//        {
//            return(false);
//        }
        if (strColName[i] >= 'a' && strColName[i] <= 'z')
        {
            strUpperColName[i] = strColName[i] - 32;
        }
        else
        {
            strUpperColName[i] = strColName[i];
        }
    }
    if (strUpperColName == "AND" || strUpperColName == "OR"
        || strUpperColName == "UPDATE" || strUpperColName == "DELETE"
        || strUpperColName == "UNION" || strUpperColName == "AS"
        || strUpperColName == "CHANGE" || strUpperColName == "SET"
        || strUpperColName == "TRUNCATE" || strUpperColName == "DESC")
    {
        return(false);
    }
    return true;
}

bool PgAgentSession::Response(const net::tagMsgShell &stMsgShell,const MsgHead &oInMsgHead,int iErrno, const std::string& strErrMsg)
{
    DataMem::MemRsp oRsp;
    oRsp.set_from(DataMem::MemRsp::FROM_DB);
    oRsp.set_err_no(iErrno);
    oRsp.set_err_msg(strErrMsg);
    return Response(stMsgShell,oInMsgHead,oRsp);
}

bool PgAgentSession::Response(const net::tagMsgShell &stMsgShell,const MsgHead &oInMsgHead,const DataMem::MemRsp& oRsp)
{
    LOG4_TRACE("error %d: %s", oRsp.err_no(), oRsp.err_msg().c_str());
    return net::SendToClient(stMsgShell,oInMsgHead,oRsp.SerializeAsString());
}

bool PgAgentSession::Init()
{
    if(boInit)
    {
        return true;
    }
    if (!LoadConfig())
    {
    	LOG4_WARN("%s failed to LoadConfig",__FUNCTION__);
		return false;
    }
    SetCurrentTime();
    boInit = true;
    return true;
}

void PgAgentSession::Tests()
{
	if (boTest)return;
	boTest = true;
	if (net::GetNodeType() == "PGAGENT" && !m_objTestPGConfig.IsEmpty())
	{
		uint32 enable_test(0);
		if (m_objTestPGConfig.Get("enable_test",enable_test) && enable_test)
		{
			uint32 test_times(0);
			uint32 test_count(0);
			std::string sql;
			m_objTestPGConfig.Get("test_times",test_times);
			m_objTestPGConfig.Get("test_count",test_count);
			m_objTestPGConfig.Get("sql",sql);
			if (test_times && test_count && sql.size())
			{
				util::CJsonObject objDB;
				if (m_objTestPGConfig.Get("db",objDB))
				{
					LOG4_INFO("%s()", __FUNCTION__);
					util::tagDbConfDetail stDbConfDetail;
					util::tagDbConnInfo& tagDbConnInfo = stDbConfDetail.m_stDbConnInfo;
					snprintf(tagDbConnInfo.m_szDbName,32,objDB("dbname").c_str());
					snprintf(tagDbConnInfo.m_szDbPwd,32,objDB("pwd").c_str());
					snprintf(tagDbConnInfo.m_szDbUser,32,objDB("user").c_str());
					snprintf(tagDbConnInfo.m_szDbHost,32,objDB("host").c_str());
					snprintf(tagDbConnInfo.m_szDbCharSet,32,objDB("charset").c_str());
					objDB.Get("port",tagDbConnInfo.m_uiDbPort);
					objDB.Get("timeout",tagDbConnInfo.uiTimeOut);
					pqxx::connection* pPgConn(NULL);
					if (!ConnectDb(stDbConfDetail,pPgConn) && pPgConn)
					{
						m_TestClock.StartClock();
						std::list<std::string> strSqls;
						std::string strErr;
						for(uint32 n = 0;n < test_times;++n)
						{
							strSqls.clear();
							for(uint32 i = 0;i < test_count;++i) strSqls.push_back(sql);

							if (ExecuteSqlPineline(pPgConn,strSqls,strSqls.size(),INT_MAX,strErr) > 0)
							{
								LOG4_ERROR("%s() ExecuteSqlPineline:%s",__FUNCTION__,strErr.c_str());
								break;
							}
							LOG4_INFO("%s() test_times %u,counting to %u,test_count:%u", __FUNCTION__,test_times,n,test_count);
						}
						m_TestClock.EndClock();
						LOG4_INFO("%s() TestClock use time:%lf",__FUNCTION__,m_TestClock.LastUseTime());
						/*
						(1/5/10)连接（进程）发送100次，每次1000sql（52000字节），耗时(4248/4576/10484)毫秒， (23809/109265/95383)qps
						 * */
					}
					else
					{
						LOG4_ERROR("%s() failed to connect to db:%s",__FUNCTION__,objDB.ToString().c_str());
					}
				}
			}
		}
	}
}

bool PgAgentSession::Get_tb_job_record()
{
	net::DbOperator oOperator(0,"tb_job_record",DataMem::MemOperate::DbOperate::SELECT);
	oOperator.AddDbField("tag_id","",DataMem::INT,"",false,true,"");//升序排列
	oOperator.AddDbField("job_id","",DataMem::INT,"",false,true,"");//升序排列
	oOperator.AddDbField("describe");
	oOperator.AddDbField("type");
	oOperator.AddDbField("sql");
	oOperator.AddDbField("enable");
	oOperator.AddDbField("tablename");
	oOperator.AddDbField("period");
	oOperator.AddDbField("stage");
	oOperator.AddDbField("allow_start_time");
	oOperator.AddDbField("allow_end_time");
	oOperator.AddDbField("start_time");
	oOperator.AddDbField("end_time");
	oOperator.AddDbField("date");
	DataMem::MemRsp oRsp;
	if (QueryOper(oRsp,*oOperator.MakeMemOperate()))
	{
		if (oRsp.record_data_size() == 0)
		{
			LOG4_WARN("%s() oRsp.record_data_size() == 0", __FUNCTION__);
			return false;
		}
		for (int i = 0; i < oRsp.record_data_size(); ++i)
		{
			const ::DataMem::Record& oRecord = oRsp.record_data(i);
			if (oRecord.field_info_size() != 14)
			{
				LOG4_ERROR("%s() field_info_size() not matched", __FUNCTION__);
				return false;
			}
			tb_job_record record;
			record.tag_id = atoi(oRecord.field_info(0).col_value().c_str());
			record.job_id = atoi(oRecord.field_info(1).col_value().c_str());
			record.describe = oRecord.field_info(2).col_value();
			record.type = atoi(oRecord.field_info(3).col_value().c_str());
			record.sql = oRecord.field_info(4).col_value();
			record.enable = atoi(oRecord.field_info(5).col_value().c_str());
			record.tablename = oRecord.field_info(6).col_value();
			record.period = oRecord.field_info(7).col_value();
			record.stage = atoi(oRecord.field_info(8).col_value().c_str());
			record.allow_start_time = oRecord.field_info(9).col_value();
			record.allow_end_time = oRecord.field_info(10).col_value();
			record.start_time = oRecord.field_info(11).col_value();
			record.end_time = oRecord.field_info(12).col_value();
			record.date = oRecord.field_info(13).col_value();

			if (record.enable != tb_job_record::eEnable_exe)continue;

			std::string date = util::time_t2TimeStr(m_uiCurrentTime,"YYYY-MM-DD");//日期
			LOG4_TRACE("%s tb_job_record(%s) date(%s,%s)",__FUNCTION__,record.serial().c_str(),record.date.c_str(),date.c_str());

			std::string dhms = util::time_t2TimeStr(m_uiCurrentTime,"DD HH:MI:SS");//日时分秒
			std::string mdhms = util::time_t2TimeStr(m_uiCurrentTime,"MM-DD HH:MI:SS");//月日时分秒
			LOG4_TRACE("dhms mdhms:%s %s",dhms.c_str(),mdhms.c_str());
			if (record.sql.size() && record.tablename.size() && (tb_job_record::eStage_init == record.stage || tb_job_record::eStage_done_cal == record.stage) && record.date != date)//不同日期的才检查任务
			{
				std::string strTime = util::time_t2TimeStr(m_uiCurrentTime);
				if ("day" == record.period)//02:00:00  04:00:00
				{
					RemoveFlag(record.allow_start_time);
					RemoveFlag(record.allow_end_time);
					std::string hms = util::time_t2TimeStr(m_uiCurrentTime,"HH:MI:SS");//时分秒
					LOG4_TRACE("%s date(%s) strTime(%s) hms(%s) allow(%s-%s)",__FUNCTION__,date.c_str(),strTime.c_str(),hms.c_str(),record.allow_start_time.c_str(),record.allow_end_time.c_str());
					//限时分秒
					if (record.allow_start_time.size() == hms.size() && record.allow_start_time <= hms && record.allow_end_time >= hms)
					{
						Set_tb_job_record(record,date);
					}
				}
				else if ("month" == record.period)//01 2:00:00   01 4:00:00
				{
					std::string dhms = util::time_t2TimeStr(m_uiCurrentTime,"DD HH:MI:SS");//日时分秒
					LOG4_TRACE("%s date(%s) strTime(%s) dhms(%s) allow(%s-%s)",__FUNCTION__,date.c_str(),strTime.c_str(),dhms.c_str(),record.allow_start_time.c_str(),record.allow_end_time.c_str());
					//限日时分秒
					if (record.allow_start_time.size() == dhms.size() && record.allow_start_time <= dhms && record.allow_end_time >= dhms)
					{
						Set_tb_job_record(record,date);
					}
				}
				else if ("year" == record.period)//01-01 2:00:00   01-01 4:00:00
				{
					std::string mdhms = util::time_t2TimeStr(m_uiCurrentTime,"MM-DD HH:MI:SS");//月日时分秒
					LOG4_TRACE("%s date(%s) strTime(%s) mdhms(%s) allow(%s-%s)",__FUNCTION__,date.c_str(),strTime.c_str(),mdhms.c_str(),record.allow_start_time.c_str(),record.allow_end_time.c_str());
					//限月日时分秒
					if (record.allow_start_time.size() == mdhms.size() && record.allow_start_time <= dhms && record.allow_end_time >= dhms)
					{
						Set_tb_job_record(record,date);
					}
				}
			}
		}
		return true;
	}
	return false;
}

bool PgAgentSession::Set_tb_job_record(tb_job_record &record,const std::string &date)
{
	if (record.date != date && (tb_job_record::eStage_init == record.stage || tb_job_record::eStage_done_cal == record.stage))//不同日期运行指令
	{
		record.date = date;
		record.start_time = util::time_t2TimeStr(m_uiCurrentTime);
		record.stage = tb_job_record::eStage_runnning;
		net::DbOperator oOperator(0,"tb_job_record",DataMem::MemOperate::DbOperate::UPDATE);
		oOperator.AddDbField("stage",tb_job_record::eStage_runnning);//eFuncRecordStage_running
		oOperator.AddDbField("start_time",record.start_time);
		oOperator.AddDbField("date",record.date);
		oOperator.AddCondition(0,
				DataMem::MemOperate::DbOperate::ConditionGroup::AND,
				DataMem::MemOperate::DbOperate::Condition::EQ,
				"job_id",record.job_id);
		oOperator.AddCondition(1,
				DataMem::MemOperate::DbOperate::ConditionGroup::OR,
				DataMem::MemOperate::DbOperate::Condition::EQ,
				"stage",tb_job_record::eStage_init);//eFuncRecordStage_init
		oOperator.AddCondition(1,
				DataMem::MemOperate::DbOperate::ConditionGroup::OR,
				DataMem::MemOperate::DbOperate::Condition::EQ,
				"stage",tb_job_record::eStage_done_cal);
		DataMem::MemRsp oRsp;
		if (!QueryOper(oRsp,*oOperator.MakeMemOperate()))
		{
			LOG4_WARN("%s oRsp(%s) failed",__FUNCTION__,oRsp.DebugString().c_str());
			return false;
		}
		else
		{
			LOG4_TRACE("%s oRsp(%s)",__FUNCTION__,oRsp.DebugString().c_str());
			if (oRsp.record_data_size() && oRsp.record_data(0).field_info_size())
			{
				int affect = atoi(oRsp.record_data(0).field_info(0).col_value().c_str());
				if (affect > 0)
				{
					//只有设置成功的才执行任务
					LOG4_INFO("%s affect(%d) try to ExeJob:%s",__FUNCTION__,affect,record.sql.c_str());
					ExeJob(record);
					Set_tb_job_record(record,date);//执行成功或失败都会记录
					return true;
				}
			}
			LOG4_INFO("%s() job(%s) is using by others,ignore it",__FUNCTION__,record.sql.c_str());
		}
	}
	else if (record.stage == tb_job_record::eStage_runnning)//统计结果、恢复状态
	{
		record.stage = tb_job_record::eStage_done_cal;
		SetCurrentTime();
		record.end_time = util::time_t2TimeStr(m_uiCurrentTime);
		net::DbOperator oOperator(0,"tb_job_record",DataMem::MemOperate::DbOperate::UPDATE);
		oOperator.AddDbField("stage",tb_job_record::eStage_done_cal);//eFuncRecordStage_init
		oOperator.AddDbField("end_time",record.end_time);
		oOperator.AddDbField("result",record.result);
		oOperator.AddCondition(DataMem::MemOperate::DbOperate::Condition::EQ,"job_id",record.job_id);
		oOperator.AddCondition(DataMem::MemOperate::DbOperate::Condition::EQ,"stage",tb_job_record::eStage_runnning);
		DataMem::MemRsp oRsp;
		if (!QueryOper(oRsp,*oOperator.MakeMemOperate()))
		{
			LOG4_WARN("%s oRsp(%s) failed",__FUNCTION__,oRsp.DebugString().c_str());
			return false;
		}
	}
	return true;
}

bool PgAgentSession::ExeJob(tb_job_record &record)
{
	if (record.tablename.size() && record.sql.size())
	{
		m_RunClock.StartClock(record.sql.c_str());
		net::DbOperator oOperator(0,record.tablename,DataMem::MemOperate::DbOperate::CUSTOM);
		oOperator.AddDbField(record.sql);
		DataMem::MemRsp oRsp;
		if (!QueryOper(oRsp,*oOperator.MakeMemOperate()))
		{
			LOG4_WARN("%s oRsp(%s) failed",__FUNCTION__,oRsp.DebugString().c_str());
			m_RunClock.EndClock();
			record.result = oRsp.err_msg().size() ? oRsp.err_msg(): "fail";
			return false;
		}
		record.result = oRsp.err_msg().size() ? oRsp.err_msg(): "success";
		m_RunClock.EndClock();
		LOG4_INFO("%s oRsp(%s) succ LastUseTime(%lf) ms",__FUNCTION__,oRsp.DebugString().c_str(),m_RunClock.LastUseTime());
		return true;
	}
	return false;
}

bool PgAgentSession::Time2BreakBulkWrite(float totalTime,uint32 totalCounter)
{
	if (m_uiBuffSqlMaxWriteTime && totalTime >= m_uiBuffSqlMaxWriteTime)
	{
		if (totalTime >= m_uiBuffSqlMaxWriteTime)LOG4_INFO("%s() totalTime(%lf) ms",__FUNCTION__,totalTime);
		return true;
	}
	if (m_uiBuffSqlMaxWriteCount && totalCounter >= m_uiBuffSqlMaxWriteCount)
	{
		if (totalCounter >= m_uiBuffSqlMaxWriteCount)LOG4_INFO("%s() totalTime(%lf) ms",__FUNCTION__,totalCounter);
		return true;
	}
	return false;
}

net::E_CMD_STATUS PgAgentSession::Timeout()
{
	SetCurrentTime();
	Tests();
	++m_uiTimeOutCounter;
	if ("PGAGENT_JOB" == net::GetNodeType())
	{
		if (m_uiJobTime && ((m_uiTimeOutCounter % m_uiJobTime) == 0))
		{
			Get_tb_job_record();
		}
	}
	bool boWrite = m_uiBuffSqlCheckTime ? false :true;//是否延时写
	if (m_uiBuffSqlCheckTime && ((m_uiTimeOutCounter % m_uiBuffSqlCheckTime) == 0)) boWrite= true;
	if (boWrite)
	{
		float totalTime = 0;
		uint32 totalCounter = 0;
		if (m_listCustomSqls.size())
		{
			LOG4_INFO("%s m_listCustomSqls total size(%u)",__FUNCTION__,m_listCustomSqls.size());
			while(m_listCustomSqls.size())
			{
				if (Time2BreakBulkWrite(totalTime,totalCounter))break;
				pqxx::result result;std::string strErr;
				SqlValues& sqlValues = m_listCustomSqls.front();
				const std::string& strSql = sqlValues.sqlvalues;
				if (ExecuteSql(sqlValues.pPgConn,strSql,result,strErr) > 0)
				{
					snprintf(sBuff,sizeof(sBuff)-1,"%s",strSql.c_str());
					LOG4_WARN("%s failed to execute sql(size:%u,%s ...)",__FUNCTION__,strSql.size(),sBuff);
				}
				else
				{
					snprintf(sBuff,sizeof(sBuff)-1,"%s",strSql.c_str());
					LOG4_TRACE("%s affected_rows(%u) succ to execute sql(size:%u,%s ...)",__FUNCTION__,result.affected_rows(),strSql.size(),sBuff);
				}
				totalTime += m_RunClock.LastUseTime();
				++totalCounter;
				m_listCustomSqls.pop_front();
			}
		}
		if(m_mapBulkSqls.size())
		{
			uint32 listsize(0);for(auto& iter:m_mapBulkSqls)listsize += iter.second.size();
			LOG4_INFO("%s m_mapBulkSqls total size(%u)",__FUNCTION__,listsize);
			while(m_mapBulkSqls.size())
			{
				if (Time2BreakBulkWrite(totalTime,totalCounter))break;
				const std::string& table = m_mapBulkSqls.begin()->first;
				std::list<SqlValues>& sqllist = m_mapBulkSqls.begin()->second;
				while(sqllist.size())
				{
					SqlValues& sqlValues = sqllist.front();
					const std::string& values = sqlValues.sqlvalues;
					if (Time2BreakBulkWrite(totalTime,totalCounter))break;
					pqxx::result result;std::string strErr;
					std::string strSql = table + values.substr(0,values.size()-1);//组合sql
					if (ExecuteSql(sqlValues.pPgConn,strSql,result,strErr) > 0)
					{
						snprintf(sBuff,sizeof(sBuff)-1,"%s",strSql.c_str());
						LOG4_WARN("%s failed to execute sql(size:%u,%s ...)",__FUNCTION__,strSql.size(),sBuff);
					}
					else
					{
						snprintf(sBuff,sizeof(sBuff)-1,"%s",strSql.c_str());
						LOG4_TRACE("%s affected_rows(%u) succ to execute sql(size:%u,%s ...)",__FUNCTION__,result.affected_rows(),strSql.size(),sBuff);
					}
					totalTime += m_RunClock.LastUseTime();
					++totalCounter;
					sqllist.pop_front();
				}
				if (sqllist.size() == 0)m_mapBulkSqls.erase(m_mapBulkSqls.begin());
			}
		}
		if (m_PinelineStrSqls.size())
		{
			uint32 listsize(0);for(auto& iter:m_PinelineStrSqls)listsize += iter.second.size();
			LOG4_INFO("%s m_PinelineStrSqls total size(%u)",__FUNCTION__,listsize);
			while (m_PinelineStrSqls.size())
			{

				if (Time2BreakBulkWrite(totalTime,totalCounter))break;
				pqxx::connection* pPgConn = m_PinelineStrSqls.begin()->first;
				std::list<std::string> &strSqls = m_PinelineStrSqls.begin()->second;
				if (strSqls.size())
				{
					std::string strErr;
					if (ExecuteSqlPineline(pPgConn,strSqls,m_uiPipelineSqlMaxNum,m_uiPipelineSqlMaxSize,strErr) > 0)
					{
						LOG4_WARN("%s failed to execute sql.use time(%lf)",__FUNCTION__,m_RunClock.LastUseTime());
					}
					else
					{
						snprintf(sBuff,sizeof(sBuff)-1,"%s",m_PinelineLastSqls.size()?m_PinelineLastSqls.front().c_str():"");
						LOG4_TRACE("%s succ to execute sql(%s...).use time(%lf)",__FUNCTION__,sBuff,m_RunClock.LastUseTime());
					}
				}
				totalTime += m_RunClock.LastUseTime();
				++totalCounter;
				if (strSqls.size() == 0) m_PinelineStrSqls.erase(m_PinelineStrSqls.begin());
			}
		}
	}
    return net::STATUS_CMD_RUNNING;
}

}
;
//namespace core
