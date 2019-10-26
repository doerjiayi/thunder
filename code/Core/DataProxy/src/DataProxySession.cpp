/*
 * DataProxySession.cpp
 *
 *  Created on: 2018年1月8日
 *      Author: chenjiayi
 */
#include "util/HashCalc.hpp"
#include "util/UnixTime.hpp"
#include "DataProxySession.h"

namespace core
{

DataProxySession* GetDataProxySession()
{
    DataProxySession* pSess = (DataProxySession*) net::GetSession(DATAPROXY_SESSION_ID,"net::DataProxySession");
    if (pSess)
    {
        return (pSess);
    }
    pSess = new DataProxySession();
    if (pSess == NULL)
    {
        LOG4_ERROR("error %d: new DataProxySession() error!",ERR_NEW);
        return (NULL);
    }
	if (net::RegisterCallback(pSess))
	{
		if (!pSess->Init())
		{
			g_pLabor->DeleteCallback(pSess);
			return NULL;
		}
		LOG4_DEBUG("register DataProxySession ok!");
		return (pSess);
	}
	else
	{
		LOG4_ERROR("register DataProxySession error!");
		delete pSess;
		pSess = NULL;
	}
    return (NULL);
}

net::E_CMD_STATUS DataProxySession::Timeout()
{
    return net::STATUS_CMD_RUNNING;
}

bool DataProxySession::Init()
{
	if (boInit)return true;
	boInit = true;
	return ReadNosqlClusterConf() && ReadNosqlDbRelativeConf();
}

bool DataProxySession::ReadNosqlClusterConf()
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    util::CJsonObject oConfJson;
    if (!net::GetJsonConfigFile(net::GetConfigPath() + std::string("NosqlCluster.json"),oConfJson))
	{
		return false;
	}
	LOG4_TRACE("oConfJson pasre OK");
	char szSessionId[64] = {0};
	char szFactorSectionKey[32] = {0};
	int32 iVirtualNodeNum = 200;
	int32 iHashAlgorithm = 0;
	uint32 uiDataType = 0;
	uint32 uiFactor = 0;
	uint32 uiFactorSection = 0;
	std::string strRedisNode;
	std::string strMaster;
	std::string strSlave;
	oConfJson.Get("hash_algorithm", iHashAlgorithm);
	oConfJson.Get("virtual_node_num", iVirtualNodeNum);
	if (oConfJson["cluster"].IsEmpty())
	{
		LOG4_ERROR("oConfJson[\"cluster\"] is empty!");
		return(false);
	}
	std::string strGroupUse;
	if (!oConfJson.Get("group_use",strGroupUse))
	{
		strGroupUse = "redis_group";
	}
	LOG4_INFO("server start to use data group:%s!",strGroupUse.c_str());
	if (oConfJson[strGroupUse].IsEmpty())
	{
		LOG4_ERROR("oConfJson[\"redis_group\"] is empty!");
		return(false);
	}
	for (int i = 0; i < oConfJson["data_type"].GetArraySize(); ++i)
	{
		if (oConfJson["data_type_enum"].Get(oConfJson["data_type"](i), uiDataType))
		{
			for (int j = 0; j < oConfJson["section_factor"].GetArraySize(); ++j)
			{
				if (oConfJson["section_factor_enum"].Get(oConfJson["section_factor"](j), uiFactor))
				{
					if (oConfJson["factor_section"][oConfJson["section_factor"](j)].IsArray())
					{
						std::set<uint32> setFactorSection;
						for (int k = 0; k < oConfJson["factor_section"][oConfJson["section_factor"](j)].GetArraySize(); ++k)
						{
							if (oConfJson["factor_section"][oConfJson["section_factor"](j)].Get(k, uiFactorSection))
							{
								snprintf(szSessionId, sizeof(szSessionId), "%u:%u:%u", uiDataType, uiFactor, uiFactorSection);
								snprintf(szFactorSectionKey, sizeof(szFactorSectionKey), "LE_%u", uiFactorSection);
								setFactorSection.insert(uiFactorSection);
								if (oConfJson["cluster"][oConfJson["data_type"](i)][oConfJson["section_factor"](j)][szFactorSectionKey].IsArray())
								{
									for (int l = 0; l < oConfJson["cluster"][oConfJson["data_type"](i)][oConfJson["section_factor"](j)][szFactorSectionKey].GetArraySize(); ++l)
									{
										SessionRedisNode* pNodeSession = (SessionRedisNode*)net::GetSession(szSessionId, "net::SessionRedisNode");
										if (pNodeSession == NULL)
										{
											pNodeSession = new SessionRedisNode(szSessionId, iHashAlgorithm, iVirtualNodeNum,strGroupUse);
											LOG4_TRACE("register node session %s", szSessionId);
											RegisterCallback(pNodeSession);
										}
										strRedisNode = oConfJson["cluster"][oConfJson["data_type"](i)][oConfJson["section_factor"](j)][szFactorSectionKey](l);
										if (oConfJson[strGroupUse][strRedisNode].Get("master", strMaster)
														&& oConfJson[strGroupUse][strRedisNode].Get("slave", strSlave))
										{
											LOG4_TRACE("Add node %s[%s, %s] to %s!",strRedisNode.c_str(), strMaster.c_str(), strSlave.c_str(), szSessionId);
											pNodeSession->AddRedisNode(strRedisNode, strMaster, strSlave);
										}
									}
								}
								else
								{
									LOG4_ERROR("oConfJson[\"cluster\"][\"%s\"][\"%s\"][\"%s\"] is not a json array!",oConfJson["data_type"](i).c_str(), oConfJson["section_factor"](j).c_str(), szFactorSectionKey);
									continue;
								}
							}
							else
							{
								LOG4_ERROR("oConfJson[\"factor_section\"][\"%s\"](%d) is not exist!",oConfJson["section_factor"](j).c_str(), k);
								continue;
							}
						}
						snprintf(szSessionId, sizeof(szSessionId), "%u:%u", uiDataType, uiFactor);
						LOG4_TRACE("add data_type and factor [%s] to m_mapFactorSection", szSessionId);
						m_mapFactorSection.insert(std::pair<std::string, std::set<uint32> >(szSessionId, setFactorSection));
					}
					else
					{
						LOG4_ERROR("oConfJson[\"factor_section\"][\"%s\"] is not a json array!",oConfJson["section_factor"](j).c_str());
						continue;
					}
				}
				else
				{
					LOG4_ERROR("missing %s in oConfJson[\"section_factor_enum\"]", oConfJson["section_factor"](j).c_str());
					continue;
				}
			}
		}
		else
		{
			LOG4_ERROR("missing %s in oConfJson[\"data_type_enum\"]", oConfJson["data_type"](i).c_str());
			continue;
		}
	}
    return true;
}

bool DataProxySession::ReadNosqlDbRelativeConf()
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    if (!net::GetJsonConfigFile(net::GetConfigPath() + std::string("NosqlDbRelative.json"),m_oNosqlDbRelative))
	{
		return false;
	}
    return(true);
}

bool DataProxySession::GetSectionFactor(int32 data_purpose,uint64 uiFactor,std::string& strSectionFactor)
{
	int32 iDataType(0);int32 iSectionFactorType(0);
	std::string strRedisDataPurpose = std::to_string(data_purpose);
	m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose].Get("data_type", iDataType);
	m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose].Get("section_factor", iSectionFactorType);
	return GetSectionFactor(iDataType,iSectionFactorType,uiFactor,strSectionFactor);
}

bool DataProxySession::GetSectionFactor(int32 iDataType, int32 iSectionFactorType, uint64 uiFactor,std::string& strSectionFactor)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    char szFactor[32] = {0};
    snprintf(szFactor, 32, "%d:%d", iDataType, iSectionFactorType);
	std::map<std::string, std::set<uint32> >::const_iterator c_factor_iter = m_mapFactorSection.find(szFactor);//section => set(segment)
	if (c_factor_iter == m_mapFactorSection.end())
	{
		snprintf(m_pErrBuff, sizeof(m_pErrBuff), "no redis node session for data_type %u and section_factor_type %u!",iDataType, iSectionFactorType);
		LOG4_ERROR("%d: %s", ERR_LACK_CLUSTER_INFO, m_pErrBuff);
		return(false);
	}
	uint32 uiSwitchFactor = (uint32)(uiFactor & 0xFFFFFFFE);//目前 最大的范围是 [0,4294967295)
	std::set<uint32>::const_iterator c_section_iter = c_factor_iter->second.lower_bound(uiSwitchFactor);
	if (c_section_iter == c_factor_iter->second.end())
	{
		snprintf(m_pErrBuff, sizeof(m_pErrBuff), "no redis node found for data_type %u and section_factor_type %u and factor %u!",
						iDataType, iSectionFactorType, uiSwitchFactor);
		LOG4_ERROR("%d: %s", ERR_LACK_CLUSTER_INFO,m_pErrBuff);
		return(false);
	}
	snprintf(szFactor, 32, "%u:%u:%u", iDataType, iSectionFactorType, *c_section_iter);
	strSectionFactor = szFactor;
	return(true);
}

bool DataProxySession::GetNosqlTableName(int32 data_purpose,std::string& strTableName)
{
	std::string strRedisDataPurpose = std::to_string(data_purpose);
	if (m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose].Get("table", strTableName))
	{
		if ((m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("data_type") != m_oNosqlDbRelative["tables"][strTableName]("data_type"))
			|| (m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("section_factor") != m_oNosqlDbRelative["tables"][strTableName]("section_factor")))
		{
			snprintf(m_pErrBuff, sizeof(m_pErrBuff), "the \"data_type\" or \"section_factor\" config were conflict in m_oNosqlDbRelative[\"redis_struct\"][\"%s\"] ",strRedisDataPurpose.c_str());
			LOG4_ERROR("error %d: %s", ERR_INVALID_REDIS_ROUTE, m_pErrBuff);
			return(false);
		}
		return true;
	}
	snprintf(m_pErrBuff, sizeof(m_pErrBuff), "failed to get strTableName for data_purpose:%d",data_purpose);
	LOG4_ERROR("error %d: %s", ERR_INVALID_REDIS_ROUTE,m_pErrBuff);
	return false;
}

bool DataProxySession::PreprocessRedis(DataMem::MemOperate& oMemOperate)
{
	if (oMemOperate.has_redis_operate())
	{
		int32 iKeyTtl = 0;
		char cCmdChar;
		char szRedisCmd[64] = {0};
		size_t i = 0;
		// 把redis命令转成全大写
		for (i = 0; i < oMemOperate.redis_operate().redis_cmd_read().size(); ++i)
		{
			cCmdChar = oMemOperate.redis_operate().redis_cmd_read()[i];
			szRedisCmd[i] = (cCmdChar <= 'z' && cCmdChar >= 'a') ? cCmdChar - ('a'-'A') : cCmdChar;
		}
		szRedisCmd[i] = '\0';
		oMemOperate.mutable_redis_operate()->set_redis_cmd_read(szRedisCmd);

		for (i = 0; i < oMemOperate.redis_operate().redis_cmd_write().size(); ++i)
		{
			cCmdChar = oMemOperate.redis_operate().redis_cmd_write()[i];
			szRedisCmd[i] = (cCmdChar <= 'z' && cCmdChar >= 'a') ? cCmdChar - ('a'-'A') : cCmdChar;
		}
		szRedisCmd[i] = '\0';
		oMemOperate.mutable_redis_operate()->set_redis_cmd_write(szRedisCmd);
		char szRedisDataPurpose[16] = {0};
		snprintf(szRedisDataPurpose, 16, "%d", oMemOperate.redis_operate().data_purpose());
		if (m_oNosqlDbRelative["redis_struct"][szRedisDataPurpose].Get("ttl", iKeyTtl))
		{
			oMemOperate.mutable_redis_operate()->set_key_ttl(iKeyTtl);
		}
		if (m_oNosqlDbRelative["section_factor_enum"]("string") == m_oNosqlDbRelative["redis_struct"][szRedisDataPurpose]("section_factor"))
		{
			const std::string& hash_key = oMemOperate.redis_operate().hash_key().size() ? oMemOperate.redis_operate().hash_key():oMemOperate.redis_operate().key_name();
			uint32 uiHash = 0;
			uiHash = util::CalcKeyHash(hash_key.c_str(), hash_key.size());
			oMemOperate.set_section_factor(uiHash);
		}
	}
	return true;
}

bool DataProxySession::CheckRedisOperate(const DataMem::MemOperate::RedisOperate& oRedisOperate)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if ("FLUSHALL" == oRedisOperate.redis_cmd_write()
		|| "FLUSHDB" == oRedisOperate.redis_cmd_write()
		|| "FLUSHALL" == oRedisOperate.redis_cmd_read()
		|| "FLUSHDB" == oRedisOperate.redis_cmd_read())
    {
    	snprintf(m_pErrBuff, sizeof(m_pErrBuff), "cmd flush was forbidden!");
		LOG4_ERROR("error %d: %s!",ERR_DB_TABLE_NOT_DEFINE, m_pErrBuff);
        return(false);
    }
    util::CJsonObject oJsonRedis;
    char szRedisDataPurpose[16] = {0};
    snprintf(szRedisDataPurpose, 16, "%d", oRedisOperate.data_purpose());
    if (!m_oNosqlDbRelative["redis_struct"].Get(szRedisDataPurpose, oJsonRedis))
    {
        snprintf(m_pErrBuff, gc_iErrBuffSize, "the redis structure \"%s\" was not define!", szRedisDataPurpose);
        LOG4_ERROR("error %d: %s!",ERR_DB_TABLE_NOT_DEFINE, m_pErrBuff);
        return(false);
    }
    /*
    if ((oJsonRedis("table").size()  > 0) && (!oMemOperate.has_db_operate()))
    {
        LOG4_ERROR("error %d: the redis structure \"%s\" was not define!",
                        ERR_REDIS_STRUCTURE_NOT_DEFINE, oMemOperate.redis_operate().key_name().c_str());
        Response(stMsgShell, oInMsgHead, ERR_REDIS_STRUCTURE_NOT_DEFINE,
                        "the redis structure was not define in dataproxy config file!");
        return(false);
    }
    */
    if (oRedisOperate.redis_cmd_read().size() > 0 && oRedisOperate.redis_cmd_write().size() > 0)
    {
        if (NOSQL_T_HASH == oRedisOperate.redis_structure())
        {
            if ((std::string("HVALS") == oRedisOperate.redis_cmd_read()
                            || std::string("HMGET") == oRedisOperate.redis_cmd_read()
                            || std::string("HGETALL") == oRedisOperate.redis_cmd_read())
                            && std::string("HMSET") != oRedisOperate.redis_cmd_write())
            {
                snprintf(m_pErrBuff, gc_iErrBuffSize, "the redis read cmd \"%s\" matching \"HMSET\" not \"%s\"!",
                                oRedisOperate.redis_cmd_read().c_str(), oRedisOperate.redis_cmd_write().c_str());
                LOG4_ERROR("%d: %s!", ERR_REDIS_READ_WRITE_CMD_NOT_MATCH, m_pErrBuff);
                return(false);
            }
            else if (std::string("HGET") == oRedisOperate.redis_cmd_read()
                            && std::string("HSET") != oRedisOperate.redis_cmd_write())
            {
                snprintf(m_pErrBuff, gc_iErrBuffSize, "the redis read cmd \"%s\" matching \"HMSET\" not \"%s\"!",
                                oRedisOperate.redis_cmd_read().c_str(), oRedisOperate.redis_cmd_write().c_str());
                LOG4_ERROR("%d: %s!", ERR_REDIS_READ_WRITE_CMD_NOT_MATCH, m_pErrBuff);
                return(false);
            }
        }
        else if (NOSQL_T_STRING == oRedisOperate.redis_structure())
        {
            if (std::string("GET") == oRedisOperate.redis_cmd_read()
                            && std::string("SET") != oRedisOperate.redis_cmd_write())
            {
                snprintf(m_pErrBuff, gc_iErrBuffSize, "the redis read cmd \"%s\" matching \"HMSET\" not \"%s\"!",
                                oRedisOperate.redis_cmd_read().c_str(), oRedisOperate.redis_cmd_write().c_str());
                LOG4_ERROR("%d: %s!", ERR_REDIS_READ_WRITE_CMD_NOT_MATCH, m_pErrBuff);
                return(false);
            }
        }
    }
    return(true);
}

bool DataProxySession::CheckDbOperate(const DataMem::MemOperate::DbOperate& oDbOperate)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (m_oNosqlDbRelative["tables"][oDbOperate.table_name()]["cols"].GetArraySize() == 0)
    {
        snprintf(m_pErrBuff, sizeof(m_pErrBuff), "the db table \"%s\" was not define!", oDbOperate.table_name().c_str());
        LOG4_ERROR("error %d: %s!",ERR_DB_TABLE_NOT_DEFINE, m_pErrBuff);
        return(false);
    }
    if (DataMem::MemOperate::DbOperate::INSERT == oDbOperate.query_type()
                    || DataMem::MemOperate::DbOperate::INSERT_IGNORE == oDbOperate.query_type()
                    || DataMem::MemOperate::DbOperate::REPLACE == oDbOperate.query_type())
    {
        if (m_oNosqlDbRelative["tables"][oDbOperate.table_name()]["cols"].GetArraySize() != oDbOperate.fields_size())
        {
            snprintf(m_pErrBuff, sizeof(m_pErrBuff), "the db table \"%s\" field num %d, but request field num %d!",oDbOperate.table_name().c_str(),
                            m_oNosqlDbRelative["tables"][oDbOperate.table_name()]["cols"].GetArraySize(), oDbOperate.fields_size());
            LOG4_ERROR("error %d: %s!",ERR_LACK_TABLE_FIELD, m_pErrBuff);
            return(false);
        }
    }
    auto table_iter = m_mapTableFields.find(oDbOperate.table_name());
    if (table_iter == m_mapTableFields.end())
    {
        std::set<std::string> setTableFields;
        for (int i = 0; i < m_oNosqlDbRelative["tables"][oDbOperate.table_name()]["cols"].GetArraySize(); ++i)
        {
            setTableFields.insert(m_oNosqlDbRelative["tables"][oDbOperate.table_name()]["cols"](i));
        }
        m_mapTableFields.insert(std::pair<std::string, std::set<std::string> >(oDbOperate.table_name(), setTableFields));
        table_iter = m_mapTableFields.find(oDbOperate.table_name());
    }

    for (int i = 0; i < oDbOperate.fields_size(); ++i)
    {
        if (oDbOperate.fields(i).col_name().size() == 0)
        {
            snprintf(m_pErrBuff, sizeof(m_pErrBuff), "the db table \"%s\" field name can not be empty!",oDbOperate.table_name().c_str());
            LOG4_ERROR("error %d: %s!",ERR_TABLE_FIELD_NAME_EMPTY,m_pErrBuff);
            return(false);
        }
        if (table_iter->second.end() == table_iter->second.find(oDbOperate.fields(i).col_name())
			&& (oDbOperate.fields(i).col_as().size() > 0 && table_iter->second.end() == table_iter->second.find(oDbOperate.fields(i).col_as())))
        {
            snprintf(m_pErrBuff, sizeof(m_pErrBuff), "the query table \"%s\" field name \"%s\" is not match the table dict!",
                            oDbOperate.table_name().c_str(), oDbOperate.fields(i).col_name().c_str());
            LOG4_ERROR("error %d: %s!",ERR_DB_FIELD_ORDER_OR_FIELD_NAME, m_pErrBuff);
            return(false);
        }
    }
    return(true);
}

bool DataProxySession::CheckDataSet(const DataMem::MemOperate& oMemOperate,const std::string& strRedisDataPurpose)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (m_oNosqlDbRelative["tables"][oMemOperate.db_operate().table_name()]["cols"].GetArraySize() != oMemOperate.db_operate().fields_size())
		// && (DataMem::MemOperate::RedisOperate::T_WRITE == oMemOperate.redis_operate().op_type()))
    {
        snprintf(m_pErrBuff, gc_iErrBuffSize, "the query table \"%s\" field num is not match the table dict field num!",oMemOperate.db_operate().table_name().c_str());
        LOG4_ERROR("error %d: the query table \"%s\" field num is not match the table dict field num!",ERR_DB_FIELD_NUM, oMemOperate.db_operate().table_name().c_str());
        return(false);
    }
    bool bFoundKeyField = false;
    if (m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field").size() == 0)  // 不需要key_field字段，则认为已找到
    {
        bFoundKeyField = true;
    }
    for (int i = 0; i < oMemOperate.db_operate().fields_size(); ++i)
    {
//        if (DataMem::MemOperate::RedisOperate::T_WRITE == oMemOperate.redis_operate().op_type())
//        {
            if ((m_oNosqlDbRelative["tables"][oMemOperate.db_operate().table_name()]["cols"](i) != oMemOperate.db_operate().fields(i).col_name())
					&& (m_oNosqlDbRelative["tables"][oMemOperate.db_operate().table_name()]["cols"](i) != oMemOperate.db_operate().fields(i).col_as()))
            {
                snprintf(m_pErrBuff, gc_iErrBuffSize, "the query table \"%s\" query field name \"%s\" is not match the table dict field name \"%s\"!",
                                oMemOperate.db_operate().table_name().c_str(), oMemOperate.db_operate().fields(i).col_name().c_str(),
                                m_oNosqlDbRelative["tables"][oMemOperate.db_operate().table_name()]["cols"](i).c_str());
                LOG4_ERROR("error %d: %s",ERR_KEY_FIELD,m_pErrBuff);
                return(false);
            }
//        }
        if (!bFoundKeyField)
        {
            if (m_oNosqlDbRelative["tables"][oMemOperate.db_operate().table_name()]["cols"](i) == m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field"))
            {
                bFoundKeyField = true;
            }
        }
    }
    if (!bFoundKeyField)
    {
        snprintf(m_pErrBuff, sizeof(m_pErrBuff), "key field \"%s\" not found in the table dict!",m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field").c_str());
        LOG4_ERROR("error %d: %s",ERR_KEY_FIELD,m_pErrBuff);//ERR_KEY_FIELD
        return(false);
    }
    return(true);
}

//检查发送数据目的的合法性--JoinField类型
bool DataProxySession::CheckJoinField(const DataMem::MemOperate& oMemOperate, const std::string& strRedisDataPurpose)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::set<std::string> setReqDbFields;
    for (int i = 0; i < oMemOperate.db_operate().fields_size(); ++i)
    {
        if (oMemOperate.db_operate().fields(i).col_as().size() > 0)
        {
            setReqDbFields.insert(oMemOperate.db_operate().fields(i).col_as());
        }
        else if (oMemOperate.db_operate().fields(i).col_name().size() > 0)
        {
            setReqDbFields.insert(oMemOperate.db_operate().fields(i).col_name());
        }
    }
    if (m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field").size() != 0)  // 需要检查key_field字段
    {
        if (setReqDbFields.end() == setReqDbFields.find(m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field")))
        {
            snprintf(m_pErrBuff, sizeof(m_pErrBuff), "key field \"%s\" not found in the table dict!",m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field").c_str());
            LOG4_ERROR("error %d:%s",ERR_KEY_FIELD, m_pErrBuff);
            return(false);
        }
    }
    for (int j = 0; j < m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]["join_fields"].GetArraySize(); ++j)
    {
        if (setReqDbFields.end() == setReqDbFields.find(m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]["join_fields"](j)))
        {
            snprintf(m_pErrBuff, sizeof(m_pErrBuff), "the query table \"%s\" join field name is not match the db request field!",oMemOperate.db_operate().table_name().c_str());
            LOG4_ERROR("error %d:%s",ERR_JOIN_FIELDS, m_pErrBuff);
            return(false);
        }
    }
    return(true);
}

//检查发送数据目的的合法性--Dataset类型WriteBoth
bool DataProxySession::PrepareForWriteBothWithDataset(DataMem::MemOperate& oMemOperate, const std::string& strRedisDataPurpose)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (NOSQL_T_HASH == oMemOperate.redis_operate().redis_structure())
    {
        if (m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field").size() == 0)
        {
        	snprintf(m_pErrBuff, sizeof(m_pErrBuff),"miss key_field in CmdDataProxyTableRelative.json config!");
        	LOG4_ERROR("error %d:%s",ERR_KEY_FIELD, m_pErrBuff);
            return(false);
        }

        if ("HSET" == oMemOperate.redis_operate().redis_cmd_write())    // 命令如果是HSET，redis请求的field将被忽略，改用db请求中的field来填充
        {
            // 强行清空redis field并以db请求中的field来替换
            oMemOperate.mutable_redis_operate()->clear_fields();
            DataMem::Record oRecord;
            DataMem::Field* pMemRedisField = oMemOperate.mutable_redis_operate()->add_fields();//设置新的redis field
            for (int i = 0; i < oMemOperate.db_operate().fields_size(); ++i)
            {
                if ((m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field") == oMemOperate.db_operate().fields(i).col_name())
					|| (oMemOperate.db_operate().fields(i).col_as().size() > 0 && m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field")
                                                == oMemOperate.db_operate().fields(i).col_as()))
                {
                    //key
                    if (oMemOperate.db_operate().fields(i).col_value().size() == 0)
                    {
                        snprintf(m_pErrBuff, gc_iErrBuffSize, "the value of key field \"%s\" can not be empty!",
                                        m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field").c_str());
                        LOG4_ERROR("error %d:%s",ERR_KEY_FIELD_VALUE, m_pErrBuff);
                        return(false);
                    }
                    pMemRedisField->set_col_name(oMemOperate.db_operate().fields(i).col_value());
                }
                DataMem::Field* pRedisField = oRecord.add_field_info();
                pRedisField->set_col_name(oMemOperate.db_operate().fields(i).col_name());
                pRedisField->set_col_value(oMemOperate.db_operate().fields(i).col_value());
            }
            LOG4_DEBUG("%s() oRecord(%s)",__FUNCTION__,oRecord.DebugString().c_str());
            if (oMemOperate.redis_operate().fields_size() == 0)
            {
                snprintf(m_pErrBuff, sizeof(m_pErrBuff),"key field \"%s\" not found in the table dict!",
                                m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field").c_str());
                LOG4_ERROR("error %d:%s",ERR_KEY_FIELD, m_pErrBuff);
                return(false);
            }
            pMemRedisField->set_col_value(oRecord.SerializeAsString());
        }
        else if ("HDEL" == oMemOperate.redis_operate().redis_cmd_write())   // 命令如果是HDEL，redis后面的参数有且仅有一个，参数的名或值至少有一个不为空
        {
            if (oMemOperate.redis_operate().fields_size() != 1)
            {
            	snprintf(m_pErrBuff, sizeof(m_pErrBuff),"error %d: %s", ERR_KEY_FIELD, "hdel field num must be 1!");
                LOG4_ERROR("error %d:%s",ERR_KEY_FIELD, m_pErrBuff);
                return(false);
            }
            if (oMemOperate.redis_operate().fields(0).col_name().size() == 0)
            {
            	snprintf(m_pErrBuff, sizeof(m_pErrBuff),"hash field name is empty for hdel!");
                LOG4_ERROR("error %d:%s",ERR_KEY_FIELD, m_pErrBuff);
                return(false);
            }
            else if (oMemOperate.redis_operate().fields(0).col_value().size() > 0)
            {
            	snprintf(m_pErrBuff, sizeof(m_pErrBuff),"error %d: %s", ERR_KEY_FIELD, "hash field value is not empty for hdel!");
            	LOG4_ERROR("error %d:%s",ERR_KEY_FIELD, m_pErrBuff);
                return(false);
            }
        }
        else // 命令非法
        {
        	snprintf(m_pErrBuff, sizeof(m_pErrBuff),"error %d: %s", ERR_INVALID_CMD_FOR_HASH_DATASET, "hash with dataset cmd error!");
            LOG4_ERROR("error %d:%s",ERR_KEY_FIELD, m_pErrBuff);
            return(false);
        }
    }
    else    // NOSQL_T_STRING  NOSQL_T_LIST  NOSQL_T_SET  and so on
    {
        if ("DEL" != oMemOperate.redis_operate().redis_cmd_write())
        {
            DataMem::Record oRecord;
            for (int i = 0; i < oMemOperate.db_operate().fields_size(); ++i)
            {
                DataMem::Field* pField = oRecord.add_field_info();
                if (oMemOperate.db_operate().fields(i).col_value().size() > 0)
                {
                    pField->set_col_value(oMemOperate.db_operate().fields(i).col_value());
                }
            }
            oMemOperate.mutable_redis_operate()->clear_fields();
            DataMem::Field* pRedisField = oMemOperate.mutable_redis_operate()->add_fields();
            pRedisField->set_col_value(oRecord.SerializeAsString());
        }
    }
    return(true);
}

//检查发送数据目的的合法性--FieldJoin类型WriteBoth
bool DataProxySession::PrepareForWriteBothWithFieldJoin(DataMem::MemOperate& oMemOperate, const std::string& strRedisDataPurpose)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<std::string, std::string> mapJoinFields;
    std::map<std::string, std::string>::iterator join_field_iter;
    for (int j = 0; j < m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]["join_fields"].GetArraySize(); ++j)
    {
        mapJoinFields.insert(std::make_pair(m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]["join_fields"](j), std::string("")));
    }

    if (NOSQL_T_HASH == oMemOperate.redis_operate().redis_structure() || NOSQL_T_SORT_SET == oMemOperate.redis_operate().redis_structure())
    {
        if (m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field").size() == 0)
        {
        	snprintf(m_pErrBuff,sizeof(m_pErrBuff),"miss key_field in CmdDataProxyTableRelative.json config!");
            LOG4_ERROR("error %d: %s", ERR_KEY_FIELD, m_pErrBuff);
            return(false);
        }

        if ("HSET" == oMemOperate.redis_operate().redis_cmd_write() || "ZADD" == oMemOperate.redis_operate().redis_cmd_write())// 命令如果是HSET，redis请求的field将被忽略，改用db请求中的field来填充
        {
            if (oMemOperate.redis_operate().fields_size() == 0) // 当redis field为0时，使用db field生成redis field，否则，使用原请求的redis field
            {
                for (int i = 0; i < oMemOperate.db_operate().fields_size(); ++i)
                {
                    if ((m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field") == oMemOperate.db_operate().fields(i).col_name())
							|| (oMemOperate.db_operate().fields(i).col_as().size() > 0
							&& m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field") == oMemOperate.db_operate().fields(i).col_as()))
                    {
                        if (oMemOperate.db_operate().fields(i).col_value().size() == 0)
                        {
                            snprintf(m_pErrBuff, gc_iErrBuffSize, "the value of key field \"%s\" can not be empty!",
                                            m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field").c_str());
                            LOG4_ERROR("error %d: %s", ERR_KEY_FIELD_VALUE,m_pErrBuff);
                            return(false);
                        }
                        DataMem::Field* pRedisKeyField = oMemOperate.mutable_redis_operate()->add_fields();
                        pRedisKeyField->set_col_name(oMemOperate.db_operate().fields(i).col_value());
                    }
                    join_field_iter = mapJoinFields.find(oMemOperate.db_operate().fields(i).col_name());
                    if (join_field_iter == mapJoinFields.end() && oMemOperate.db_operate().fields(i).col_as().size() > 0)
                    {
                        join_field_iter = mapJoinFields.find(oMemOperate.db_operate().fields(i).col_as());
                    }
                    if (join_field_iter != mapJoinFields.end())
                    {
                        if (oMemOperate.db_operate().fields(i).col_value().size() == 0)
                        {
                            snprintf(m_pErrBuff, gc_iErrBuffSize, "the value of join field \"%s\" can not be empty!",
                                            oMemOperate.db_operate().fields(i).col_name().c_str());
                            LOG4_ERROR("error %d: %s", ERR_KEY_FIELD_VALUE, m_pErrBuff);
                            return(false);
                        }
                        join_field_iter->second = oMemOperate.db_operate().fields(i).col_value();
                    }
                }
                if (oMemOperate.redis_operate().fields_size() == 0)
                {
                    snprintf(m_pErrBuff, gc_iErrBuffSize, "key field \"%s\" not found in the request redis operator!",
                                    m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field").c_str());
                    LOG4_ERROR("error %d: %s",m_pErrBuff);
                    return(false);
                }
                std::string strRedisFieldValue;
                for (int j = 0; j < m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]["join_fields"].GetArraySize(); ++j)
                {
                    join_field_iter = mapJoinFields.find(m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]["join_fields"](j));
                    if (strRedisFieldValue.size() == 0)
                    {
                        strRedisFieldValue = join_field_iter->second;
                    }
                    else
                    {
                        strRedisFieldValue += std::string(":") + join_field_iter->second;
                    }
                }
                DataMem::Field* pRedisField = oMemOperate.mutable_redis_operate()->mutable_fields(0);
                pRedisField->set_col_value(strRedisFieldValue);
            }
        }
        else if ("HDEL" == oMemOperate.redis_operate().redis_cmd_write() || "ZREM" == oMemOperate.redis_operate().redis_cmd_write())   // 命令如果是HDEL，redis后面的参数有且仅有一个，参数的名或值至少有一个不为空
        {
            if (oMemOperate.redis_operate().fields(0).col_name().size() == 0)
            {
            	snprintf(m_pErrBuff, gc_iErrBuffSize,"hash field name is empty for hdel!");
                LOG4_ERROR("error %d: %s", ERR_KEY_FIELD, m_pErrBuff);
                return(false);
            }
            else if (oMemOperate.redis_operate().fields(0).col_value().size() > 0)
            {
            	snprintf(m_pErrBuff, gc_iErrBuffSize,"hash field value is not empty for hdel!");
                LOG4_ERROR("error %d: %s", ERR_KEY_FIELD, m_pErrBuff);
                return(false);
            }
        }
        else // 命令非法
        {
        	snprintf(m_pErrBuff, gc_iErrBuffSize,"hash with field join cmd error!");
            LOG4_ERROR("error %d: %s", ERR_INVALID_CMD_FOR_HASH_DATASET,m_pErrBuff);
            return(false);
        }
    }
    else    // NOSQL_T_STRING  NOSQL_T_LIST  NOSQL_T_SET  and so on
    {
        if ("DEL" != oMemOperate.redis_operate().redis_cmd_write())
        {
            for (int i = 0; i < oMemOperate.db_operate().fields_size(); ++i)
            {
                join_field_iter = mapJoinFields.find(oMemOperate.db_operate().fields(i).col_name());
                if (join_field_iter == mapJoinFields.end() && oMemOperate.db_operate().fields(i).col_as().size())
                {
                    join_field_iter = mapJoinFields.find(oMemOperate.db_operate().fields(i).col_as());
                }
                if (join_field_iter != mapJoinFields.end())
                {
                    if (oMemOperate.db_operate().fields(i).col_value().size() == 0)
                    {
                        snprintf(m_pErrBuff, gc_iErrBuffSize, "the value of key field \"%s\" can not be empty!",
                                        m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field").c_str());
                        LOG4_ERROR("error %d: %s",ERR_KEY_FIELD_VALUE,m_pErrBuff);
                        return(false);
                    }
                    join_field_iter->second = oMemOperate.db_operate().fields(i).col_value();
                }
            }
            std::string strRedisFieldValue;
            for (int j = 0; j < m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]["join_fields"].GetArraySize(); ++j)
            {
                join_field_iter = mapJoinFields.find(m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]["join_fields"](j));
                if (strRedisFieldValue.size() == 0)
                {
                    strRedisFieldValue = join_field_iter->second;
                }
                else
                {
                    strRedisFieldValue += std::string(":") + join_field_iter->second;
                }
            }
            oMemOperate.mutable_redis_operate()->clear_fields();
            DataMem::Field* pRedisField = oMemOperate.mutable_redis_operate()->add_fields();
            pRedisField->set_col_value(strRedisFieldValue);
        }
    }
    return(true);
}

bool DataProxySession::ScanSyncData()
{
    m_ScanSyncDataTime = ::time(NULL);
    if (m_ScanSyncDataTime <= m_ScanSyncDataLastTime + 60)//一分钟内最多搜索文件目录一次
    {
        return true;
    }
    m_ScanSyncDataLastTime = m_ScanSyncDataTime;
    LOG4_DEBUG("CmdDataProxy ScanSyncData time(%llu)",m_ScanSyncDataTime);
    DIR* dir;
    struct dirent* dirent_ptr;
    dir = opendir(std::string(net::GetWorkPath() + SYNC_DATA_DIR).c_str());
    if (dir != NULL)
    {
        size_t uiCurrentPos = 0;
        size_t uiNextPos = 0;
        std::string strFileName;
        std::string strWorkerIndex;
        std::string strTableName;
        std::string strTime;
        std::set<std::string> setTables;
        std::set<std::string>::iterator tb_iter;
        while ((dirent_ptr = readdir(dir)) != NULL)
        {
            strFileName = dirent_ptr->d_name;
            if (strFileName.size() > 4 && std::string(".dat") == strFileName.substr(strFileName.size() - 4, 4))
            {
                uiCurrentPos = 0;
                uiNextPos = 0;
                //0.tb_blog.201610191341.dat
                //strWorkerIndex 0
                uiNextPos = strFileName.find('.', uiCurrentPos);
                strWorkerIndex = strFileName.substr(uiCurrentPos, uiNextPos - uiCurrentPos);
                uiCurrentPos = uiNextPos + 1;
                if (strtoul(strWorkerIndex.c_str(), NULL, 10) != net::GetWorkerIndex())
                {
                    continue;
                }
                //strTableName tb_blog
                uiNextPos = strFileName.find('.', uiCurrentPos);
                strTableName = strFileName.substr(uiCurrentPos, uiNextPos - uiCurrentPos);//tb_blog
                uiCurrentPos = uiNextPos + 1;
                //strTime 201610191341
                uiNextPos = strFileName.find('.', uiCurrentPos);
                strTime = strFileName.substr(uiCurrentPos, uiNextPos - uiCurrentPos);
                uiCurrentPos = uiNextPos + 1;

                tb_iter = setTables.find(strTableName);
                if (tb_iter == setTables.end())
                {
                	setTables.insert(strTableName);
                    tb_iter = setTables.begin();
                }
            }
        }
        closedir(dir);
        if (setTables.size() > 0)
        {
        	for (const auto& tb_iter:setTables)
			{
				SessionSyncDbData* pSessionSyncDbData = GetSessionSyncDbData(tb_iter,net::GetWorkPath() + SYNC_DATA_DIR);
				if (pSessionSyncDbData)
				{
					pSessionSyncDbData->ScanSyncData();//检查同步文件
				}
				else
				{
					LOG4_ERROR("failed to get pSessionSyncDbData");
				}
			}
        	LOG4_INFO("data file to sync done");
        }
        else
        {
        	LOG4_INFO("no data file to sync");
        }
        return(true);
    }
    else
    {
        LOG4_INFO("dir %s not exist or failed to open", std::string(net::GetWorkPath() + SYNC_DATA_DIR).c_str());
        return(false);
    }
}

}
;
//namespace core
