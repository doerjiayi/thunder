/*
 * DbAgentSession.cpp
 *
 *  Created on: 2018年1月8日
 *      Author: chenjiayi
 */
#include "DbAgentSession.h"

namespace core
{

DbAgentSession* GetDbAgentSession()
{
    DbAgentSession* pSess = (DbAgentSession*) net::GetSession(DBAGENT_SESSIN_ID,"net::DbAgentSession");
    if (pSess)
    {
        return (pSess);
    }
    pSess = new DbAgentSession();
    if (pSess == NULL)
    {
        LOG4_ERROR("error %d: new DbAgentSession() error!",
                        net::ERR_NEW);
        return (NULL);
    }
    if (net::RegisterCallback(pSess))
    {
    	if (!pSess->Init())
    	{
    		LOG4_ERROR("pSess->Init() error!");
    		GetLabor()->DeleteCallback(pSess);
    		return NULL;
    	}
        LOG4_DEBUG("register DbAgentSession ok!");
        return (pSess);
    }
    else
    {
        LOG4_ERROR("register DbAgentSession error!");
        delete pSess;
        pSess = NULL;
    }
    return (NULL);
}

bool DbAgentSession::Init()
{
	if (!net::GetConfig(m_oDbConf,net::GetConfigPath() + std::string("CmdDbOper.json")))
	{
		return false;
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

    int isync(0);
    net::GetCustomConf().Get("sync",isync);
    if (isync)
    {
        m_uiSync = 1;
        LOG4_TRACE("sync db connection");
    }
    else
    {
        LOG4_TRACE("async db connection");
    }
    return(true);
}

net::E_CMD_STATUS DbAgentSession::Timeout()
{
    LOG4_DEBUG("%s() check db keepalive.size(%u)", __FUNCTION__,m_mapDbiPool.size());
	for(auto& iter:m_mapDbiPool)
	{
		if (iter.second->pDbi->MysqlPing() != 0)
		{
			LOG4_WARN("%s() mysql(%s,%u) lost connection",__FUNCTION__,
							iter.second->pDbi->GetDbConf().m_stDbConnInfo.m_szDbHost,
							iter.second->pDbi->GetDbConf().m_stDbConnInfo.m_uiDbPort);
			iter.second->ullBeatTime = 0;
		}
		else
		{
			iter.second->ullBeatTime = ::time(NULL);
		}
	}
    return net::STATUS_CMD_RUNNING;
}

bool DbAgentSession::LocateDbConn(const DataMem::MemOperate &oQuery,std::string &strInstance,util::CJsonObject& dbInstanceConf)
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
		snprintf(m_sErrMsg,sizeof(m_sErrMsg),"no db config found for data_type %d section_factor_type %d", iDataType, iSectionFactorType);
		LOG4_ERROR(m_sErrMsg);
		return(false);
	}
	else
	{
		std::set<uint32>::const_iterator c_section_iter = c_factor_iter->second.lower_bound(oQuery.section_factor());
		if (c_section_iter == c_factor_iter->second.end())
		{
			snprintf(m_sErrMsg,sizeof(m_sErrMsg),"no factor_section config found for data_type %u section_factor_type %u section_factor %u",
					iDataType, iSectionFactorType, oQuery.section_factor());
			LOG4_ERROR(m_sErrMsg);
			return(false);
		}
		else
		{
			m_uiSectionTo = *c_section_iter;
			--c_section_iter;
			if (c_section_iter == c_factor_iter->second.end()) m_uiSectionFrom = 0;
			else m_uiSectionFrom = *c_section_iter + 1;

			snprintf(szFactor, 32, "%u:%u:%u", iDataType, iSectionFactorType, *c_section_iter);
			//数据库实例配置2:4:1000000 : {"db_userinfo":"robot_db_instance_1","im_relation":"robot_db_instance_1","db_singlechat":"robot_db_instance_1","im_group":"robot_db_instance_1","im_custom":"custom_db"}
			std::map<std::string, util::CJsonObject*>::iterator conf_iter = m_mapDbInstanceInfo.find(szFactor);
			if (conf_iter == m_mapDbInstanceInfo.end())
			{
				snprintf(m_sErrMsg,sizeof(m_sErrMsg),"no db config found for %s which consist of data_type %u section_factor_type %u section_factor %u",
								szFactor, iDataType, iSectionFactorType, oQuery.section_factor());
				LOG4_ERROR(m_sErrMsg);//ERR_LACK_CLUSTER_INFO
				return(false);
			}
			std::string strDbName = m_oDbConf["table"][oQuery.db_operate().table_name()]("db_name");
			//数据库实例对应的数据类型、因子和分段
			LOG4_TRACE("db(%s) instance szFactor(%s),uiSectionTo(%u),uiSectionFrom(%u)",strDbName.c_str(),szFactor,m_uiSectionTo,m_uiSectionFrom);
			if (!conf_iter->second->Get(strDbName, strInstance))
			{
				snprintf(m_sErrMsg,sizeof(m_sErrMsg),"no db instance config for strDbName \"%s\"!", strDbName.c_str());
				LOG4_ERROR(m_sErrMsg);//ERR_LACK_CLUSTER_INFO
				return(false);
			}
			dbInstanceConf = m_oDbConf["db_group"][strInstance];//cluster集群或者主从
			return(true);
		}
	}
}

bool DbAgentSession::GetDbConnection(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemOperate& oQuery,util::CMysqlDbi** ppMasterDbi, util::CMysqlDbi** ppSlaveDbi)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::string strInstance;
    util::CJsonObject oInstanceConf;
	if (!LocateDbConn(oQuery,strInstance,oInstanceConf))
	{
		LOG4_ERROR(m_sErrMsg);
		Response(stMsgShell,oInMsgHead,ERR_LACK_CLUSTER_INFO, m_sErrMsg);
		return(false);
	}
	std::string strUseGroup = oInstanceConf("use_group");
	util::CJsonObject& oGroupHostConf = oInstanceConf[strUseGroup];
	if (oQuery.db_operate().query_type() > atoi(oInstanceConf("query_permit").c_str()))
	{
		Response(stMsgShell,oInMsgHead,ERR_NO_RIGHT, "no right to excute oMemOperate.db_operate().query_type()!");
		LOG4_ERROR("no right to excute oMemOperate.db_operate().query_type() %d!",oQuery.db_operate().query_type());
		return(false);
	}

	int nArraySize = oGroupHostConf.GetArraySize();
	if (nArraySize > 0)
	{//集群
		if (!GetDbConnectionFromIdentifyForCluster(strInstance,oGroupHostConf,oInstanceConf, ppMasterDbi))
		{
			Response(stMsgShell,oInMsgHead,ERR_NEW, "GetDbConnectionFromIdentifyForCluster failed!");
			return(false);
		}
		*ppSlaveDbi = *ppMasterDbi;//不分主从
		return(true);
	}
	else
	{//主从
		std::string strMasterIdentify = oGroupHostConf("master_host");
		std::string strSlaveIdentify = oGroupHostConf("slave_host");
		LOG4_TRACE("strMasterIdentify(%s),strSlaveIdentify(%s)",strMasterIdentify.c_str(),strSlaveIdentify.c_str());
		return GetDbConnectionFromIdentify(stMsgShell,oInMsgHead,oQuery.db_operate().query_type(),strMasterIdentify, strSlaveIdentify,
				oInstanceConf, ppMasterDbi, ppSlaveDbi);
	}
}

bool DbAgentSession::GetDbConnectionFromIdentifyForCluster(const std::string &strInstance,util::CJsonObject& oHostListConf,
                const util::CJsonObject& oInstanceConf, util::CMysqlDbi** ppMasterDbi)
{
    LOG4_TRACE("%s(%s,%s)", __FUNCTION__, oHostListConf.ToString().c_str(), oInstanceConf.ToString().c_str());
    *ppMasterDbi = NULL;
    std::map<std::string, std::set<tagConnection*> >::iterator iter =  m_mapDBConnectSet.find(strInstance);
    if (iter != m_mapDBConnectSet.end())//原有的连接缓存
    {
        std::set<tagConnection*>::iterator dbi_iter = iter->second.begin();
        for(;(NULL == *ppMasterDbi) && dbi_iter != iter->second.end();++dbi_iter)
        {
            if ((*dbi_iter)->ullBeatTime > 0)//如果检查断开连接则会被设置为0
            {
                *ppMasterDbi = (*dbi_iter)->pDbi;
                LOG4_TRACE("succeed in get strInstance(%s)'s connection",strInstance.c_str());
            }
        }
    }
    if (NULL == *ppMasterDbi)
    {
        int nArraySize = oHostListConf.GetArraySize();
        for(int i = 0; (NULL == *ppMasterDbi) && (i < nArraySize);++i)//没有原有连接可用的再建立新连接
        {
            std::string strMasterIdentify = oHostListConf[i].ToString();
            RemoveFlag(strMasterIdentify,' ');
            RemoveFlag(strMasterIdentify,'\"');
            if (strMasterIdentify.size() == 0)
            {
                continue;
            }
            std::map<std::string, tagConnection*>::iterator dbi_iter = m_mapDbiPool.find(strMasterIdentify);
            if (dbi_iter == m_mapDbiPool.end())
            {
                tagConnection* pConnection = new tagConnection();
                if (NULL == pConnection)
                {
                    LOG4_WARN("malloc space for db connection failed!strMasterIdentify:%s", strMasterIdentify.c_str());
                    continue;
                }
                util::CMysqlDbi* pDbi = new util::CMysqlDbi();
                if (NULL == pDbi)
                {
                    LOG4_WARN("malloc space for db connection failed!strMasterIdentify:%s", strMasterIdentify.c_str());
                    delete pConnection;
                    pConnection = NULL;
                    continue;
                }
                pConnection->pDbi = pDbi;
                {
                    int iPosIpPortSeparator = strMasterIdentify.find(':');
                    std::string strHost = strMasterIdentify.substr(0, iPosIpPortSeparator);
                    std::string strPort = strMasterIdentify.substr(iPosIpPortSeparator + 1, std::string::npos);
                    util::tagDbConfDetail stDbConfDetail;
                    strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbHost,strHost.c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbHost));
                    strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbUser,oInstanceConf("user").c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbUser));
                    strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbPwd,oInstanceConf("password").c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbPwd));
                    strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbName, "test",sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbName));
                    strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbCharSet,oInstanceConf("charset").c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbCharSet));
                    stDbConfDetail.m_stDbConnInfo.m_uiDbPort = atoi(strPort.c_str());
                    stDbConfDetail.m_ucDbType = util::MYSQL_DB;
                    stDbConfDetail.m_ucAccess = 1; //直连
                    stDbConfDetail.m_stDbConnInfo.uiTimeOut = atoi(oInstanceConf("timeout").c_str());
                    if (0 != ConnectDb(stDbConfDetail, pDbi))
                    {
                        LOG4_WARN("failed to connect strMasterIdentify(%s) %s:%s", strMasterIdentify.c_str(),strHost.c_str(),strPort.c_str());
                        continue;
                    }
                    LOG4_TRACE("succeed in connecting strMasterIdentify(%s) %s:%s",strMasterIdentify.c_str(),strHost.c_str(),strPort.c_str());
                }
                pConnection->iQueryPermit = atoi(oInstanceConf("query_permit").c_str());
                pConnection->iTimeout = atoi(oInstanceConf("timeout").c_str());
                pConnection->ullBeatTime = time(NULL);
                {//缓存连接
                    m_mapDbiPool.insert(std::pair<std::string, tagConnection*>(strMasterIdentify, pConnection));
                    {
                        std::map<std::string, std::set<tagConnection*> >::iterator iter =  m_mapDBConnectSet.find(strInstance);
                        if (iter != m_mapDBConnectSet.end())
                        {
                            iter->second.insert(pConnection);
                        }
                        else
                        {
                            std::set<tagConnection*> set;
                            set.insert(pConnection);
                            m_mapDBConnectSet.insert(std::make_pair(strInstance,set));
                        }
                    }
                }
                if (NULL == *ppMasterDbi)
                {
                    *ppMasterDbi = pDbi;
                }
            }
            else
            {
                if (dbi_iter->second->ullBeatTime > 0)//如果检查断开连接则会被设置为0
                {
                    *ppMasterDbi = dbi_iter->second->pDbi;
                    LOG4_TRACE("succeed in get strInstance(%s)'s connection",strInstance.c_str());
                }
            }
        }
    }
    if (*ppMasterDbi)
    {
        LOG4_TRACE("%s() use mysql(%s,%u) connection",__FUNCTION__,
                                    (*ppMasterDbi)->GetDbConf().m_stDbConnInfo.m_szDbHost,
                                    (*ppMasterDbi)->GetDbConf().m_stDbConnInfo.m_uiDbPort);
        return(true);
    }
    else
    {
        LOG4_ERROR("%s() failed to get mysql connection",__FUNCTION__);
        return(false);
    }
}

bool DbAgentSession::GetDbConnectionFromIdentify(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,
				DataMem::MemOperate::DbOperate::E_QUERY_TYPE eQueryType,const std::string& strMasterIdentify, const std::string& strSlaveIdentify,
                const util::CJsonObject& oInstanceConf, util::CMysqlDbi** ppMasterDbi, util::CMysqlDbi** ppSlaveDbi)
{
    LOG4_TRACE("%s(%s, %s, %s)", __FUNCTION__, strMasterIdentify.c_str(), strSlaveIdentify.c_str(), oInstanceConf.ToString().c_str());
    *ppMasterDbi = NULL;
    *ppSlaveDbi = NULL;
    std::map<std::string, tagConnection*>::iterator dbi_iter = m_mapDbiPool.find(strMasterIdentify);
    if (dbi_iter == m_mapDbiPool.end())
    {
        tagConnection* pConnection = new tagConnection();
        if (NULL == pConnection)
        {
            Response(stMsgShell,oInMsgHead,ERR_NEW, "malloc space for db connection failed!");
            return(false);
        }
        util::CMysqlDbi* pDbi = new util::CMysqlDbi();
        if (NULL == pDbi)
        {
            Response(stMsgShell,oInMsgHead,ERR_NEW, "malloc space for db connection failed!");
            delete pConnection;
            pConnection = NULL;
            return(false);
        }
        pConnection->pDbi = pDbi;
        if (0 == ConnectDb(oInstanceConf, pDbi, strMasterIdentify))
        {
            LOG4_TRACE("succeed in connecting strMasterIdentify(%s)", strMasterIdentify.c_str());
            *ppMasterDbi = pDbi;
            pConnection->iQueryPermit = atoi(oInstanceConf("query_permit").c_str());
            pConnection->iTimeout = atoi(oInstanceConf("timeout").c_str());
            pConnection->ullBeatTime = time(NULL);
            m_mapDbiPool.insert(std::pair<std::string, tagConnection*>(strMasterIdentify, pConnection));
        }
        else
        {
            if (DataMem::MemOperate::DbOperate::SELECT != eQueryType)
            {
                Response(stMsgShell,oInMsgHead,pDbi->GetErrno(), pDbi->GetError());
            }
            delete pConnection;
            pConnection = NULL;
        }
    }
    else
    {
        *ppMasterDbi = dbi_iter->second->pDbi;
    }

    LOG4_TRACE("find slave %s.", strSlaveIdentify.c_str());
    dbi_iter = m_mapDbiPool.find(strSlaveIdentify);
    if (dbi_iter == m_mapDbiPool.end())
    {
        tagConnection* pConnection = new tagConnection();
        if (NULL == pConnection)
        {
            Response(stMsgShell,oInMsgHead,ERR_NEW, "malloc space for db connection failed!");
            return(false);
        }
        util::CMysqlDbi* pDbi = new util::CMysqlDbi();
        if (NULL == pDbi)
        {
            Response(stMsgShell,oInMsgHead,ERR_NEW, "malloc space for db connection failed!");
            delete pConnection;
            pConnection = NULL;
            return(false);
        }
        pConnection->pDbi = pDbi;
        if (0 == ConnectDb(oInstanceConf, pDbi, strSlaveIdentify))
        {
            LOG4_TRACE("succeed in connecting strSlaveIdentify(%s)", strSlaveIdentify.c_str());
            *ppSlaveDbi = pDbi;
            pConnection->iQueryPermit = atoi(oInstanceConf("query_permit").c_str());
            pConnection->iTimeout = atoi(oInstanceConf("timeout").c_str());
            pConnection->ullBeatTime = time(NULL);
            m_mapDbiPool.insert(std::pair<std::string, tagConnection*>(strSlaveIdentify, pConnection));
        }
        else
        {
            delete pConnection;
            pConnection = NULL;
        }
    }
    else
    {
        *ppSlaveDbi = dbi_iter->second->pDbi;
    }

    LOG4_TRACE("pMasterDbi = 0x%x, pSlaveDbi = 0x%x.", *ppMasterDbi, *ppSlaveDbi);
    if (*ppMasterDbi || *ppSlaveDbi)return(true);

	return(false);
}

int DbAgentSession::ConnectDb(const util::CJsonObject& oInstanceConf, util::CMysqlDbi* pDbi,const std::string& strDbIdentify)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    int iResult = 0;
    util::tagDbConfDetail stDbConfDetail;

    std::string::size_type nIndex = strDbIdentify.find(":");
	if (nIndex != std::string::npos)
	{
		std::string strDbHost = strDbIdentify.substr(0,nIndex);
		strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbHost,strDbHost.c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbHost));
		std::string strDBPort = strDbIdentify.substr(nIndex + 1);
		stDbConfDetail.m_stDbConnInfo.m_uiDbPort = atoi(strDBPort.c_str());
	}
	else
	{
		LOG4_ERROR("error strDbIdentify:%s", strDbIdentify.c_str());
		return -1;
	}
	strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbUser,oInstanceConf("user").c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbUser));
    strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbPwd,oInstanceConf("password").c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbPwd));
    strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbName, "test",sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbName));
    strncpy(stDbConfDetail.m_stDbConnInfo.m_szDbCharSet,oInstanceConf("charset").c_str(),sizeof(stDbConfDetail.m_stDbConnInfo.m_szDbCharSet));
    stDbConfDetail.m_ucDbType = util::MYSQL_DB;
    stDbConfDetail.m_ucAccess = 1; //直连
    stDbConfDetail.m_stDbConnInfo.uiTimeOut = atoi(oInstanceConf("timeout").c_str());

    LOG4_DEBUG("InitDbConn(%s, %s, %s, %s, %u, %s)", stDbConfDetail.m_stDbConnInfo.m_szDbHost,
                    stDbConfDetail.m_stDbConnInfo.m_szDbUser, stDbConfDetail.m_stDbConnInfo.m_szDbPwd,
                    stDbConfDetail.m_stDbConnInfo.m_szDbName, stDbConfDetail.m_stDbConnInfo.m_uiDbPort,
                    stDbConfDetail.m_stDbConnInfo.m_szDbCharSet);
    iResult = pDbi->InitDbConn(stDbConfDetail);
    if (0 != iResult)
    {
        LOG4_ERROR("error %d: %s", pDbi->GetErrno(),pDbi->GetError().c_str());
    }
    return(iResult);
}

int DbAgentSession::ConnectDb(const util::tagDbConfDetail &stDbConfDetail, util::CMysqlDbi* pDbi)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    int iResult = 0;
    LOG4_DEBUG("InitDbConn(%s, %s, %s, %s, %u, %s)", stDbConfDetail.m_stDbConnInfo.m_szDbHost,
                    stDbConfDetail.m_stDbConnInfo.m_szDbUser, stDbConfDetail.m_stDbConnInfo.m_szDbPwd,
                    stDbConfDetail.m_stDbConnInfo.m_szDbName, stDbConfDetail.m_stDbConnInfo.m_uiDbPort,
                    stDbConfDetail.m_stDbConnInfo.m_szDbCharSet);
    iResult = pDbi->InitDbConn(stDbConfDetail);
    if (0 != iResult)
    {
        LOG4_ERROR("error %d: %s", pDbi->GetErrno(),pDbi->GetError().c_str());
    }
    return(iResult);
}

int DbAgentSession::SyncQuery(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const DataMem::MemOperate& oQuery, util::CMysqlDbi* pDbi)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (NULL == pDbi)
    {
        LOG4_ERROR("pDbi is null!");
        Response(stMsgShell,oInMsgHead,ERR_QUERY, "pDbi is null");
        return(ERR_QUERY);
    }
    std::string strSql;
    if (!CreateSql(oQuery, pDbi, strSql))
    {
        LOG4_ERROR("Scrabble up sql error!");
        Response(stMsgShell,oInMsgHead,ERR_QUERY, strSql);
        return(ERR_QUERY);
    }
    LOG4_DEBUG("%s", strSql.c_str());
    int iResult = 0;
	MsgHead oOutMsgHead;
	MsgBody oOutMsgBody;
	DataMem::MemRsp oRsp;
	oRsp.set_from(DataMem::MemRsp::FROM_DB);
    iResult = pDbi->ExecSql(strSql);
    if (iResult == 0)
    {
        if (DataMem::MemOperate::DbOperate::SELECT == oQuery.db_operate().query_type())
        {
        	DataMem::Record* pRecord = NULL;
			DataMem::Field* pField = NULL;
            if (NULL != pDbi->UseResult())
            {
                uint32 uiDataLen = 0;
                uint32 uiRecordSize = 0;
                int32 iRecordNum = 0;
                unsigned int uiFieldNum = pDbi->FetchFieldNum();

                //字段值进行赋值
                MYSQL_ROW stRow;
                unsigned long* lengths;

                oRsp.set_err_no(ERR_OK);
                oRsp.set_err_msg("success");
                while (NULL != (stRow = pDbi->GetRow()))
                {
                    uiRecordSize = 0;
                    ++iRecordNum;
                    lengths = pDbi->FetchLengths();
                    pRecord = oRsp.add_record_data();
                    for(unsigned int i = 0; i < uiFieldNum; ++i)
                    {
                        uiRecordSize += lengths[i];
                    }
//                    if (uiRecordSize > 1000000)
//                    {
//                        LOG4_ERROR("error %d: %s", ERR_RESULTSET_EXCEED, "result set(one record) exceed 1 MB!");
//                        Response(stMsgShell,oInMsgHead,ERR_RESULTSET_EXCEED, "result set(one record) exceed 1 MB!");
//                        return(ERR_RESULTSET_EXCEED);
//                    }

                    for(unsigned int i = 0; i < uiFieldNum; ++i)
                    {
                        pField = pRecord->add_field_info();
                        if (0 == lengths[i])
                        {
                            pField->set_col_value("");
                        }
                        else
                        {
                            pField->set_col_value(stRow[i], lengths[i]);
                            uiDataLen += lengths[i];
                        }
                    }

                    if (uiDataLen + uiRecordSize > 64000000)
                    {
                        oRsp.set_curcount(iRecordNum);
                        oRsp.set_totalcount(iRecordNum + 1);    // 表示未完
                        if (Response(stMsgShell,oInMsgHead,oRsp))
                        {
                            oRsp.clear_record_data();
                            uiDataLen = 0;
                        }
                        else
                        {
                            return(ERR_RESULTSET_EXCEED);
                        }
                    }

                }
                oRsp.set_curcount(iRecordNum);
                oRsp.set_totalcount(iRecordNum);

                Response(stMsgShell,oInMsgHead,oRsp);
                return(pDbi->GetErrno());
            }
            else
            {
                Response(stMsgShell,oInMsgHead,pDbi->GetErrno(), pDbi->GetError());
                LOG4_ERROR("%d: %s", pDbi->GetErrno(), pDbi->GetError().c_str());
                return(pDbi->GetErrno());
            }
        }
        else
        {
            Response(stMsgShell,oInMsgHead,pDbi->GetErrno(), pDbi->GetError());
            LOG4_DEBUG("%d: %s", pDbi->GetErrno(), pDbi->GetError().c_str());
            return(pDbi->GetErrno());
        }
    }
    else
    {
        LOG4_ERROR("%s\t%d: %s", strSql.c_str(), pDbi->GetErrno(), pDbi->GetError().c_str());
        if (DataMem::MemOperate::DbOperate::SELECT != oQuery.db_operate().query_type()
                        && ((pDbi->GetErrno() >= 2001 && pDbi->GetErrno() <= 2018)
                                        || (pDbi->GetErrno() >= 2038 && pDbi->GetErrno() <= 2046)))
        {   // 由于连接方面原因数据写失败，将失败数据节点返回给数据代理，等服务从故障中恢复后再由数据代理自动重试
            oRsp.set_err_no(pDbi->GetErrno());
            oRsp.set_err_msg(pDbi->GetError());
            DataMem::MemRsp::DataLocate* pLocate = oRsp.mutable_locate();
            pLocate->set_section_from(m_uiSectionFrom);
            pLocate->set_section_to(m_uiSectionTo);
            pLocate->set_hash(m_uiHash);
            pLocate->set_divisor(m_uiDivisor);
            Response(stMsgShell,oInMsgHead,oRsp);
        }
        else
        {
            Response(stMsgShell,oInMsgHead,pDbi->GetErrno(), pDbi->GetError());
        }
    }
    return(iResult);
}

int DbAgentSession::AsyncQuery(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const DataMem::MemOperate& oQuery, util::CMysqlDbi* pDbi)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (NULL == pDbi)
    {
        LOG4_ERROR("pDbi is null!");
        Response(stMsgShell,oInMsgHead,ERR_QUERY, "pDbi is null");
        return(ERR_QUERY);
    }
    std::string strSql;
    if (!CreateSql(oQuery, pDbi, strSql))
    {
        LOG4_ERROR("Scrabble up sql error!");
        Response(stMsgShell,oInMsgHead,ERR_QUERY, strSql);
        return(ERR_QUERY);
    }
    LOG4_DEBUG("query_type:%d:%s", oQuery.db_operate().query_type(),strSql.c_str());
    struct QueryMysqlParam:public net::StepParam//mysql访问参数
	{
    	QueryMysqlParam(DbAgentSession* pSession,const DataMem::MemOperate& oReq,const std::string &sql,
    			const net::tagMsgShell &shell,const MsgHead &oHead,
				uint32 uiSectionFrom,uint32 uiSectionTo,uint32 uiHash,uint32 uiDivisor):
    		pDbAgentSession(pSession),oQuery(oReq),strSql(sql),stMsgShell(shell),oInMsgHead(oHead),
			m_uiSectionFrom(uiSectionFrom),m_uiSectionTo(uiSectionTo),m_uiHash(uiHash),m_uiDivisor(uiDivisor)
			{}
		DbAgentSession* pDbAgentSession;
		DataMem::MemOperate oQuery;
		std::string strSql;
		net::tagMsgShell stMsgShell;
		MsgHead oInMsgHead;

		uint32 m_uiSectionFrom;
		uint32 m_uiSectionTo;
		uint32 m_uiHash;
		uint32 m_uiDivisor;
	};
    if (DataMem::MemOperate::DbOperate::SELECT == oQuery.db_operate().query_type())
    {
    	auto mysqlSelectCallback = [](net::StepState* state)
		{
    		QueryMysqlParam* pStageParam = (QueryMysqlParam*)state->GetData();
			net::MysqlStep* pMysqlState = (net::MysqlStep*)state;
			pStageParam->pDbAgentSession->AsyncQueryCallback(pStageParam->oQuery,
					pMysqlState->m_pMysqlResSet,pMysqlState->m_pMysqlResSet->GetErrno(),pStageParam->strSql,
					pStageParam->stMsgShell,pStageParam->oInMsgHead,
					pStageParam->m_uiSectionFrom,pStageParam->m_uiSectionTo,pStageParam->m_uiHash,pStageParam->m_uiDivisor);
			return true;
		};
    	auto mysqlSelectFailCallback = [](net::StepState* state)
        {
    		QueryMysqlParam* pStageParam = (QueryMysqlParam*)state->GetData();
            net::MysqlStep* pMysqlState = (net::MysqlStep*)state;
            pStageParam->pDbAgentSession->AsyncQueryCallback(pStageParam->oQuery,
                    pMysqlState->m_pMysqlResSet,pMysqlState->m_pMysqlResSet->GetErrno(),pStageParam->strSql,
                    pStageParam->stMsgShell,pStageParam->oInMsgHead,
                    pStageParam->m_uiSectionFrom,pStageParam->m_uiSectionTo,pStageParam->m_uiHash,pStageParam->m_uiDivisor);
        };
    	net::MysqlStep* pstep = new net::MysqlStep(pDbi->GetDbConf().m_stDbConnInfo);
		pstep->SetTask(strSql,util::eSqlTaskOper_select);//第一个任务
		pstep->AddStateFunc(mysqlSelectCallback);
		pstep->SetFailFunc(mysqlSelectFailCallback);
		pstep->SetData(new QueryMysqlParam(this,oQuery,strSql,stMsgShell,oInMsgHead,
				m_uiSectionFrom,m_uiSectionTo,m_uiHash,m_uiDivisor));
		if (!net::MysqlStep::Launch(pstep))
		{
			LOG4_WARN("MysqlStep::Launch failed");
			return ERR_QUERY;
		}
    }
    else
    {
    	auto mysqlExecCallback = [](net::StepState* state)
		{
			QueryMysqlParam* pStageParam = (QueryMysqlParam*)state->GetData();
			net::MysqlStep* pMysqlState = (net::MysqlStep*)state;
			pStageParam->pDbAgentSession->AsyncQueryCallback(pStageParam->oQuery,
					pMysqlState->m_pMysqlResSet,pMysqlState->m_pMysqlResSet->GetErrno(),pStageParam->strSql,
					pStageParam->stMsgShell,pStageParam->oInMsgHead,
					pStageParam->m_uiSectionFrom,pStageParam->m_uiSectionTo,pStageParam->m_uiHash,pStageParam->m_uiDivisor);
			return true;
		};
    	auto mysqlExecFailCallback = [](net::StepState* state)
        {
    		QueryMysqlParam* pStageParam = (QueryMysqlParam*)state->GetData();
            net::MysqlStep* pMysqlState = (net::MysqlStep*)state;
            pStageParam->pDbAgentSession->AsyncQueryCallback(pStageParam->oQuery,
                    pMysqlState->m_pMysqlResSet,pMysqlState->m_pMysqlResSet->GetErrno(),pStageParam->strSql,
                    pStageParam->stMsgShell,pStageParam->oInMsgHead,
                    pStageParam->m_uiSectionFrom,pStageParam->m_uiSectionTo,pStageParam->m_uiHash,pStageParam->m_uiDivisor);
        };
    	net::MysqlStep* pstep = new net::MysqlStep(pDbi->GetDbConf().m_stDbConnInfo);
		pstep->SetTask(strSql,util::eSqlTaskOper_exec);//第一个任务
		pstep->AddStateFunc(mysqlExecCallback);
		pstep->SetFailFunc(mysqlExecFailCallback);
		pstep->SetData(new QueryMysqlParam(this,oQuery,strSql,stMsgShell,oInMsgHead,
				m_uiSectionFrom,m_uiSectionTo,m_uiHash,m_uiDivisor));
		if (!net::MysqlStep::Launch(pstep))
		{
			LOG4_WARN("MysqlStep::Launch failed");
			return ERR_QUERY;
		}
    }
    return(0);
}

int DbAgentSession::AsyncQueryCallback(const DataMem::MemOperate& oQuery, util::MysqlResSet* pMysqlResSet,int iResult,
		const std::string &strSql,const net::tagMsgShell &stMsgShell,const MsgHead &oInMsgHead,
		uint32 uiSectionFrom,uint32 uiSectionTo,uint32 uiHash,uint32 uiDivisor)
{
	MsgHead oOutMsgHead;
	MsgBody oOutMsgBody;
	DataMem::MemRsp oRsp;
	oRsp.set_from(DataMem::MemRsp::FROM_DB);
	if (iResult == 0)
	{
		if (DataMem::MemOperate::DbOperate::SELECT == oQuery.db_operate().query_type())
		{
			DataMem::Record* pRecord = NULL;
			DataMem::Field* pField = NULL;
			if (NULL != pMysqlResSet->UseResult())
			{
				uint32 uiDataLen = 0;
				uint32 uiRecordSize = 0;
				int32 iRecordNum = 0;
				unsigned int uiFieldNum = pMysqlResSet->FetchFieldNum();

				//字段值进行赋值
				MYSQL_ROW stRow;
				unsigned long* lengths;

				oRsp.set_err_no(ERR_OK);
				oRsp.set_err_msg("success");
				while (NULL != (stRow = pMysqlResSet->GetRow()))
				{
					uiRecordSize = 0;
					++iRecordNum;
					lengths = pMysqlResSet->FetchLengths();
					pRecord = oRsp.add_record_data();
					for(unsigned int i = 0; i < uiFieldNum; ++i)
					{
						uiRecordSize += lengths[i];
					}
//					if (uiRecordSize > 1000000)
//					{
//						LOG4_ERROR("error %d: %s", ERR_RESULTSET_EXCEED, "result set(one record) exceed 1 MB!");
//						Response(stMsgShell,oInMsgHead,ERR_RESULTSET_EXCEED, "result set(one record) exceed 1 MB!");
//						return(ERR_RESULTSET_EXCEED);
//					}

					for(unsigned int i = 0; i < uiFieldNum; ++i)
					{
						pField = pRecord->add_field_info();
						if (0 == lengths[i])
						{
							pField->set_col_value("");
						}
						else
						{
							pField->set_col_value(stRow[i],lengths[i]);
							uiDataLen += lengths[i];
						}
					}

					if (uiDataLen + uiRecordSize > 64000000)
					{
						oRsp.set_curcount(iRecordNum);
						oRsp.set_totalcount(iRecordNum + 1);    // 表示未完
						if (Response(stMsgShell,oInMsgHead,oRsp))
						{
							oRsp.clear_record_data();
							uiDataLen = 0;
						}
						else
						{
							return(ERR_RESULTSET_EXCEED);
						}
					}

				}
				oRsp.set_curcount(iRecordNum);
				oRsp.set_totalcount(iRecordNum);

				Response(stMsgShell,oInMsgHead,oRsp);
				return(pMysqlResSet->GetErrno());
			}
			else
			{
			    if (pMysqlResSet->GetErrno() > 0)
                {
                    Response(stMsgShell,oInMsgHead,pMysqlResSet->GetErrno(), pMysqlResSet->GetError());
                    LOG4_ERROR("error result set %d: %s", pMysqlResSet->GetErrno(), pMysqlResSet->GetError().c_str());
                }
                else
                {
                    LOG4_WARN("empty result set %d: %s", pMysqlResSet->GetErrno(), pMysqlResSet->GetError().c_str());
                }
				return(pMysqlResSet->GetErrno());
			}
		}
		else
		{
			Response(stMsgShell,oInMsgHead,pMysqlResSet->GetErrno(), pMysqlResSet->GetError());
			LOG4_DEBUG("%d: %s", pMysqlResSet->GetErrno(), pMysqlResSet->GetError().c_str());
			return(pMysqlResSet->GetErrno());
		}
	}
	else
	{
		LOG4_ERROR("%s\t%d: %s", strSql.c_str(), pMysqlResSet->GetErrno(), pMysqlResSet->GetError().c_str());
		if (DataMem::MemOperate::DbOperate::SELECT != oQuery.db_operate().query_type()
			&& ((pMysqlResSet->GetErrno() >= 2001 && pMysqlResSet->GetErrno() <= 2018) || (pMysqlResSet->GetErrno() >= 2038 && pMysqlResSet->GetErrno() <= 2046)))
		{   // 由于连接方面原因数据写失败，将失败数据节点返回给数据代理，等服务从故障中恢复后再由数据代理自动重试
			oRsp.set_err_no(pMysqlResSet->GetErrno());
			oRsp.set_err_msg(pMysqlResSet->GetError());
			DataMem::MemRsp::DataLocate* pLocate = oRsp.mutable_locate();
			pLocate->set_section_from(uiSectionFrom);
			pLocate->set_section_to(uiSectionTo);
			pLocate->set_hash(uiHash);
			pLocate->set_divisor(uiDivisor);
			Response(stMsgShell,oInMsgHead,oRsp);
		}
		else
		{
			Response(stMsgShell,oInMsgHead,pMysqlResSet->GetErrno(), pMysqlResSet->GetError());
		}
	}
	return(iResult);
}

void DbAgentSession::CheckConnection()
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

bool DbAgentSession::Response(const net::tagMsgShell &stMsgShell,const MsgHead &oInMsgHead,int iErrno, const std::string& strErrMsg)
{
    LOG4_TRACE("error %d: %s", iErrno, strErrMsg.c_str());
    DataMem::MemRsp oRsp;
    oRsp.set_from(DataMem::MemRsp::FROM_DB);
    oRsp.set_err_no(iErrno);
    oRsp.set_err_msg(strErrMsg);
    return GetLabor()->SendToClient(stMsgShell, oInMsgHead, oRsp);
}

bool DbAgentSession::Response(const net::tagMsgShell &stMsgShell,const MsgHead &oInMsgHead,const DataMem::MemRsp& oRsp)
{
    LOG4_TRACE("error %d: %s", oRsp.err_no(), oRsp.err_msg().c_str());
    return GetLabor()->SendToClient(stMsgShell, oInMsgHead, oRsp);
}

std::string DbAgentSession::GetFullTableName(const std::string& strTableName, uint64 uiFactor)
{
    char szFullTableName[128] = {0};
    std::string strDbName = m_oDbConf["table"][strTableName]("db_name");
    if (strDbName.size() > 0)
    {
    	int iTableNum = atoi(m_oDbConf["table"][strTableName]("table_num").c_str());
		if (1 == iTableNum)
		{
			snprintf(szFullTableName, sizeof(szFullTableName), "%s.%s", strDbName.c_str(), strTableName.c_str());
		}
		else
		{
			uint32 uiTableIndex = uiFactor % iTableNum;
			snprintf(szFullTableName, sizeof(szFullTableName), "%s.%s_%02d", strDbName.c_str(), strTableName.c_str(), uiTableIndex);
			m_uiHash = uiTableIndex;
			m_uiDivisor = iTableNum;
		}
    }
    return(szFullTableName);
}

bool DbAgentSession::CreateSql(const DataMem::MemOperate& oQuery, util::CMysqlDbi* pDbi, std::string& strSql)
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
		return true;
	}
    if (oQuery.db_operate().table_name().size() == 0)
    {
    	LOG4_ERROR("invalid oQuery.db_operate().table_name().size() == 0");
		return false;
    }
//    for (unsigned int i = 0; i < oQuery.db_operate().table_name().size(); ++i)
//    {
//        if ((oQuery.db_operate().table_name()[i] >= 'A' && oQuery.db_operate().table_name()[i] <= 'Z')
//            || (oQuery.db_operate().table_name()[i] >= 'a' && oQuery.db_operate().table_name()[i] <= 'z')
//            || (oQuery.db_operate().table_name()[i] >= '0' && oQuery.db_operate().table_name()[i] <= '9')
//            || oQuery.db_operate().table_name()[i] == '_' || oQuery.db_operate().table_name()[i] == '.')
//        {
//            ;
//        }
//        else
//        {
//            return(false);
//        }
//    }
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
            bResult = CreateInsert(oQuery, pDbi, strSql);
            break;
        }
        case DataMem::MemOperate::DbOperate::UPDATE:
        {
        	if (oQuery.db_operate().fields_size() <= 0)
			{
				LOG4_ERROR("invalid fields_size(%d) for UPDATE",oQuery.db_operate().fields_size());
				return false;
			}
            bResult = CreateUpdate(oQuery, pDbi, strSql);
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
        bResult = CreateConditionGroup(oQuery, pDbi, strCondition);
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

bool DbAgentSession::CreateSelect(const DataMem::MemOperate& oQuery, std::string& strSql)
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

bool DbAgentSession::CreateInsert(const DataMem::MemOperate& oQuery, util::CMysqlDbi* pDbi, std::string& strSql)
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
                pDbi->EscapeString(m_szColValue, oQuery.db_operate().fields(i).col_value().c_str(), oQuery.db_operate().fields(i).col_value().size());
                strSql += std::string("VALUES('") + std::string(m_szColValue) + std::string("'");
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
                pDbi->EscapeString(m_szColValue, oQuery.db_operate().fields(i).col_value().c_str(), oQuery.db_operate().fields(i).col_value().size());
                strSql += std::string(",'") + std::string(m_szColValue) + std::string("'");
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

bool DbAgentSession::CreateUpdate(const DataMem::MemOperate& oQuery, util::CMysqlDbi* pDbi, std::string& strSql)
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
                pDbi->EscapeString(m_szColValue, oQuery.db_operate().fields(i).col_value().c_str(), oQuery.db_operate().fields(i).col_value().size());
                strSql += std::string(" SET ") + oQuery.db_operate().fields(i).col_name() + std::string("=");
                strSql += std::string("'") + std::string(m_szColValue) + std::string("'");
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
                strSql += std::string(" SET ") + oQuery.db_operate().fields(i).col_name() + std::string("=") + oQuery.db_operate().fields(i).col_value();
            }
        }
        else
        {
            if (DataMem::STRING == oQuery.db_operate().fields(i).col_type())
            {
                pDbi->EscapeString(m_szColValue, oQuery.db_operate().fields(i).col_value().c_str(), oQuery.db_operate().fields(i).col_value().size());
                strSql += std::string(", ") + oQuery.db_operate().fields(i).col_name() + std::string("=");
                strSql += std::string("'") + std::string(m_szColValue) + std::string("'");
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

bool DbAgentSession::CreateDelete(const DataMem::MemOperate& oQuery, std::string& strSql)
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

bool DbAgentSession::CreateCondition(const DataMem::MemOperate::DbOperate::Condition& oCondition, util::CMysqlDbi* pDbi, std::string& strCondition)
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
                pDbi->EscapeString(m_szColValue, oCondition.col_values(i).c_str(), oCondition.col_values(i).size());
                strCondition += std::string("'") + std::string(m_szColValue) + std::string("'");
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

bool DbAgentSession::CreateConditionGroup(const DataMem::MemOperate& oQuery, util::CMysqlDbi* pDbi, std::string& strCondition)
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
            bResult = CreateCondition(oQuery.db_operate().conditions(i).condition(j), pDbi, strOneCondition);
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

bool DbAgentSession::CreateGroupBy(const DataMem::MemOperate& oQuery, std::string& strGroupBy)
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

bool DbAgentSession::CreateOrderBy(const DataMem::MemOperate& oQuery, std::string& strOrderBy)
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

bool DbAgentSession::CreateLimit(const DataMem::MemOperate& oQuery, std::string& strLimit)
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

bool DbAgentSession::CheckColName(const std::string& strColName)
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

}
;
//namespace core
