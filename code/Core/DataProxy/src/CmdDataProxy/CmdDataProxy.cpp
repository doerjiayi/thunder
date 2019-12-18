/*******************************************************************************
 * Project:  DataProxy
 * @file     CmdDataProxy.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年12月8日
 * @note
 * Modify history:
 ******************************************************************************/
#include "util/HashCalc.hpp"
#include "CmdDataProxy.hpp"

MUDULE_CREATE(core::CmdDataProxy);

namespace core
{

CmdDataProxy::CmdDataProxy()
: m_pProxySess(NULL),m_pRedisNodeSession(NULL),pStepSendToDbAgent(NULL), pStepReadFromRedis(NULL), pStepWriteToRedis(NULL), pStepReadFromRedisForWrite(NULL)
{
}

CmdDataProxy::~CmdDataProxy()
{
}

bool CmdDataProxy::Init()
{
	m_pProxySess = GetDataProxySession();
    if (m_pProxySess)
    {
    	m_pProxySess->ScanSyncData();
        return(true);
    }
    return(false);
}

bool CmdDataProxy::AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody)
{
    DataMem::MemOperate oMemOperate;
    if (oMemOperate.ParseFromString(oInMsgBody.body()))
    {
        LOG4_TRACE("oMemOperate: %s", oMemOperate.DebugString().c_str());
        m_pProxySess->PreprocessRedis(oMemOperate);

        if (!CheckRequest(stMsgShell, oInMsgHead, oMemOperate))
        {
            LOG4_DEBUG("%s", oMemOperate.DebugString().c_str());
            return(false);
        }
        if (!oMemOperate.has_db_operate())
        {
            return(RedisOnly(stMsgShell, oInMsgHead, oMemOperate));
        }
        if (!oMemOperate.has_redis_operate())
        {
            return(DbOnly(stMsgShell, oInMsgHead, oMemOperate));
        }
        if (oMemOperate.has_db_operate() && oMemOperate.has_redis_operate())
        {
            std::string strRedisDataPurpose = std::to_string(oMemOperate.redis_operate().data_purpose());
            std::string strRedisRelative = m_pProxySess->m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("relative");//是否有特殊数据类型,有才需要检查，否则不需要检查
            LOG4_TRACE("strRedisDataPurpose:%s strRedisRelative:%s", strRedisDataPurpose.c_str(),strRedisRelative.c_str());
            if (DataMem::MemOperate::RedisOperate::T_READ == oMemOperate.redis_operate().op_type())
            {
            	if (strRedisRelative.size())
            	{
            		if (m_pProxySess->m_oNosqlDbRelative["relative"]("dataset") == strRedisRelative)
					{
						if (!m_pProxySess->CheckDataSet(oMemOperate,strRedisDataPurpose))
						{
							LOG4_ERROR("error %d: %s",ERR_DB_FIELD_NUM,m_pProxySess->m_pErrBuff);
							Response(stMsgShell, oInMsgHead, ERR_DB_FIELD_NUM, m_pProxySess->m_pErrBuff);
							return(false);
						}
					}
					else if (m_pProxySess->m_oNosqlDbRelative["relative"]("join") == strRedisRelative)
					{
						if (!m_pProxySess->CheckJoinField(oMemOperate,strRedisDataPurpose))
						{
							LOG4_ERROR("error %d: %s",ERR_JOIN_FIELDS,m_pProxySess->m_pErrBuff);
							Response(stMsgShell, oInMsgHead, ERR_JOIN_FIELDS,m_pProxySess->m_pErrBuff);
							return false;
						}
					}
            	}
                return(ReadEither(stMsgShell, oInMsgHead, oMemOperate));
            }
            else
            {
                if (DataMem::MemOperate::DbOperate::UPDATE == oMemOperate.db_operate().query_type())
                {
                	if (strRedisRelative.size())
                	{
                		if (m_pProxySess->m_oNosqlDbRelative["relative"]("dataset") == strRedisRelative)
						{
							return(UpdateBothWithDataset(stMsgShell, oInMsgHead, oMemOperate));
						}
                	}
					return(WriteBoth(stMsgShell, oInMsgHead, oMemOperate));
                }
                else if (DataMem::MemOperate::DbOperate::DELETE == oMemOperate.db_operate().query_type())
                {
                    return(WriteBoth(stMsgShell, oInMsgHead, oMemOperate));
                }
                else
                {
                	if (strRedisRelative.size())
                	{
                		if (m_pProxySess->m_oNosqlDbRelative["relative"]("dataset") == strRedisRelative)
						{
							if (!m_pProxySess->CheckDataSet(oMemOperate,strRedisDataPurpose))
							{
								LOG4_ERROR("error %d: %s",ERR_DB_FIELD_NUM,m_pProxySess->m_pErrBuff);
								Response(stMsgShell, oInMsgHead, ERR_DB_FIELD_NUM, m_pProxySess->m_pErrBuff);
								return(false);
							}
							if (!m_pProxySess->PrepareForWriteBothWithDataset(oMemOperate,strRedisDataPurpose))
							{
								LOG4_ERROR("error %d: %s", ERR_KEY_FIELD,m_pProxySess->m_pErrBuff);
								Response(stMsgShell, oInMsgHead, ERR_KEY_FIELD,m_pProxySess->m_pErrBuff);
								return false;
							}
						}
						else if (m_pProxySess->m_oNosqlDbRelative["relative"]("join") == strRedisRelative)
						{
							if (!m_pProxySess->CheckJoinField(oMemOperate,strRedisDataPurpose))
							{
								LOG4_ERROR("error %d: %s",ERR_JOIN_FIELDS,m_pProxySess->m_pErrBuff);
								Response(stMsgShell, oInMsgHead, ERR_JOIN_FIELDS,m_pProxySess->m_pErrBuff);
								return false;
							}
							if (!m_pProxySess->PrepareForWriteBothWithFieldJoin(oMemOperate,strRedisDataPurpose))
							{
								LOG4_ERROR("error %d: %s", ERR_KEY_FIELD,m_pProxySess->m_pErrBuff);
								Response(stMsgShell, oInMsgHead, ERR_KEY_FIELD,m_pProxySess->m_pErrBuff);
								return(false);
							}
						}
                	}
                    return(WriteBoth(stMsgShell, oInMsgHead, oMemOperate));
                }
            }
        }
        return(true);
    }
    else
    {
        Response(stMsgShell, oInMsgHead, ERR_PARASE_PROTOBUF, "failed to parse DataMem::MemOperate from oInMsgBody.body()!");
        return(false);
    }
}

bool CmdDataProxy::CheckRequest(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const DataMem::MemOperate& oMemOperate)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (!oMemOperate.has_redis_operate() && !oMemOperate.has_db_operate())
    {
        Response(stMsgShell, oInMsgHead, ERR_INCOMPLET_DATAPROXY_DATA, "neighter redis_operate nor db_operate was exist!");
        return(false);
    }
    int32 iDataType;
    int32 iSectionFactorType;
    if (oMemOperate.has_redis_operate())
    {
        if (!m_pProxySess->CheckRedisOperate(oMemOperate.redis_operate()))
		{
			LOG4_ERROR("error %d: %s",ERR_REDIS_READ_WRITE_CMD_NOT_MATCH,m_pProxySess->m_pErrBuff);
			Response(stMsgShell, oInMsgHead, ERR_REDIS_READ_WRITE_CMD_NOT_MATCH, m_pProxySess->m_pErrBuff);
			return(false);
		}
        if (!GetNodeSession(oMemOperate))
        {
            LOG4_ERROR("error %d: %s!",ERR_INVALID_REDIS_ROUTE, m_pProxySess->m_pErrBuff);
            Response(stMsgShell, oInMsgHead, ERR_INVALID_REDIS_ROUTE, m_pProxySess->m_pErrBuff);
            return(false);
        }
    }
    if (oMemOperate.has_db_operate())
    {
        if (!m_pProxySess->CheckDbOperate(oMemOperate.db_operate()))
		{
			LOG4_ERROR("error %d: %s",ERR_DB_TABLE_NOT_DEFINE,m_pProxySess->m_pErrBuff);
			Response(stMsgShell, oInMsgHead, ERR_DB_TABLE_NOT_DEFINE, m_pProxySess->m_pErrBuff);
			return(false);
		}
        if (!m_pProxySess->m_oNosqlDbRelative["tables"][oMemOperate.db_operate().table_name()].Get("data_type", iDataType)
                        || !m_pProxySess->m_oNosqlDbRelative["tables"][oMemOperate.db_operate().table_name()].Get("section_factor", iSectionFactorType))
        {
            snprintf(m_pErrBuff, gc_iErrBuffSize, "no \"data_type\" or \"section_factor\" config in "
                            "m_pProxySess->m_oNosqlDbRelative[\"tables\"][\"%s\"]", oMemOperate.db_operate().table_name().c_str());
            LOG4_ERROR("error %d: %s", ERR_INVALID_REDIS_ROUTE, m_pErrBuff);
            Response(stMsgShell, oInMsgHead, ERR_INVALID_REDIS_ROUTE, m_pErrBuff);
            return(false);
        }
    }
    if (oMemOperate.has_redis_operate() && oMemOperate.has_db_operate())
	{
		if ((DataMem::MemOperate::RedisOperate::T_WRITE == oMemOperate.redis_operate().op_type()
				&& DataMem::MemOperate::DbOperate::SELECT == oMemOperate.db_operate().query_type())
			||(DataMem::MemOperate::RedisOperate::T_READ == oMemOperate.redis_operate().op_type()
				&& DataMem::MemOperate::DbOperate::SELECT != oMemOperate.db_operate().query_type()))
		{
			Response(stMsgShell, oInMsgHead, ERR_REDIS_AND_DB_CMD_NOT_MATCH,"redis_operate.op_type() and db_operate.query_type() was not match!");
			return(false);
		}
		std::string strTableName;
		if (!m_pProxySess->GetNosqlTableName(oMemOperate.redis_operate().data_purpose(),strTableName))
		{
			LOG4_ERROR("error %d: %s!",ERR_INVALID_REDIS_ROUTE,m_pProxySess->m_pErrBuff);
			Response(stMsgShell, oInMsgHead, ERR_INVALID_REDIS_ROUTE,m_pProxySess->m_pErrBuff);
			return(false);
		}
	}
    return(true);
}

bool CmdDataProxy::RedisOnly(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemOperate& oMemOperate)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    if (DataMem::MemOperate::RedisOperate::T_WRITE == oMemOperate.redis_operate().op_type())
    {
        return GetLabor()->ExecStep(new StepWriteToRedis(stMsgShell, oInMsgHead, oMemOperate.redis_operate(), m_pRedisNodeSession));
    }
    else
    {
        std::string strRedisDataPurpose = std::to_string(oMemOperate.redis_operate().data_purpose());
        if (m_pProxySess->m_oNosqlDbRelative["relative"]("dataset") == m_pProxySess->m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("relative"))
        {
            pStepReadFromRedis = new StepReadFromRedis(stMsgShell, oInMsgHead,oMemOperate.redis_operate(), m_pRedisNodeSession, true,
                            &m_pProxySess->m_oNosqlDbRelative["tables"][oMemOperate.db_operate().table_name()]["cols"],
                            m_pProxySess->m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field"));
        }
        else
        {
            pStepReadFromRedis = new StepReadFromRedis(stMsgShell, oInMsgHead,oMemOperate.redis_operate(), m_pRedisNodeSession, false);
        }
        return GetLabor()->ExecStep(pStepReadFromRedis);
    }
}

bool CmdDataProxy::DbOnly(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemOperate& oMemOperate)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    return GetLabor()->ExecStep(new StepSendToDbAgent(stMsgShell, oInMsgHead, oMemOperate, m_pRedisNodeSession));
}

bool CmdDataProxy::ReadEither(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemOperate& oMemOperate)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::string strRedisDataPurpose = std::to_string(oMemOperate.redis_operate().data_purpose());
    std::string strTableName;
    m_pProxySess->GetNosqlTableName(oMemOperate.redis_operate().data_purpose(),strTableName);
    pStepSendToDbAgent = new StepSendToDbAgent(stMsgShell, oInMsgHead, oMemOperate, m_pRedisNodeSession,
                    atoi(m_pProxySess->m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("relative").c_str()),
                    &m_pProxySess->m_oNosqlDbRelative["tables"][strTableName]["cols"],
                    m_pProxySess->m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field"),
                    &m_pProxySess->m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]["join_fields"]);
    if (m_pProxySess->m_oNosqlDbRelative["relative"]("dataset") == m_pProxySess->m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("relative"))
    {
        pStepReadFromRedis = new StepReadFromRedis(stMsgShell, oInMsgHead,oMemOperate.redis_operate(), m_pRedisNodeSession, true,
                        &m_pProxySess->m_oNosqlDbRelative["tables"][oMemOperate.db_operate().table_name()]["cols"],
                        m_pProxySess->m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field"),
                        pStepSendToDbAgent);
    }
    else
    {
        pStepReadFromRedis = new StepReadFromRedis(stMsgShell, oInMsgHead,oMemOperate.redis_operate(), m_pRedisNodeSession, false, NULL, "", pStepSendToDbAgent);
    }
    return GetLabor()->ExecStep(pStepReadFromRedis);
}

bool CmdDataProxy::WriteBoth(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,DataMem::MemOperate& oMemOperate)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    pStepSendToDbAgent = new StepSendToDbAgent(stMsgShell, oInMsgHead, oMemOperate, m_pRedisNodeSession);
    pStepWriteToRedis = new StepWriteToRedis(stMsgShell, oInMsgHead, oMemOperate.redis_operate(), m_pRedisNodeSession, pStepSendToDbAgent);
    return GetLabor()->ExecStep(pStepWriteToRedis);
}

bool CmdDataProxy::UpdateBothWithDataset(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,DataMem::MemOperate& oMemOperate)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    if (NOSQL_T_HASH == oMemOperate.redis_operate().redis_structure())
    {
    	std::string strRedisDataPurpose = std::to_string(oMemOperate.redis_operate().data_purpose());
        if (m_pProxySess->m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field").size() == 0)
        {
            LOG4_ERROR("error %d: %s", ERR_KEY_FIELD, "miss key_field in CmdDataProxyTableRelative.json config!");
            Response(stMsgShell, oInMsgHead, ERR_KEY_FIELD,"miss key_field in CmdDataProxyTableRelative.json config!");
            return(false);
        }
        if (("HSET" != oMemOperate.redis_operate().redis_cmd_write()
                        && "HDEL" != oMemOperate.redis_operate().redis_cmd_write())
                        || oMemOperate.redis_operate().fields_size() != 1)
        {
            LOG4_ERROR("error %d: %s", ERR_INVALID_CMD_FOR_HASH_DATASET, "hash set cmd error, or invalid hash field num!");
            Response(stMsgShell, oInMsgHead, ERR_INVALID_CMD_FOR_HASH_DATASET, "hash set cmd error, or invalid hash field num!");
            return(false);
        }
        else if (oMemOperate.redis_operate().fields(0).col_name().size() == 0)
        {
            LOG4_ERROR("error %d: %s", ERR_KEY_FIELD, "hash field name is empty!");
            Response(stMsgShell, oInMsgHead, ERR_KEY_FIELD, "hash field name is empty!");
            return(false);
        }
        else if (oMemOperate.redis_operate().fields(0).col_value().size() > 0)
        {
            LOG4_ERROR("error %d: %s", ERR_KEY_FIELD, "hash field value is not empty!");
            Response(stMsgShell, oInMsgHead, ERR_KEY_FIELD, "hash field value is not empty!");
            return(false);
        }
        oMemOperate.mutable_redis_operate()->set_redis_cmd_read("HGET");
        pStepReadFromRedisForWrite = new StepReadFromRedisForWrite(stMsgShell, oInMsgHead, oMemOperate, m_pRedisNodeSession,
                        m_pProxySess->m_oNosqlDbRelative["tables"][oMemOperate.db_operate().table_name()]["cols"],
						m_pProxySess->m_oNosqlDbRelative["redis_struct"][strRedisDataPurpose]("key_field"));
        return GetLabor()->ExecStep(pStepReadFromRedisForWrite);
    }
    else if (NOSQL_T_STRING == oMemOperate.redis_operate().redis_structure())
    {
        oMemOperate.mutable_redis_operate()->set_redis_cmd_read("GET");
        oMemOperate.mutable_redis_operate()->clear_fields();
        pStepReadFromRedisForWrite = new StepReadFromRedisForWrite(stMsgShell, oInMsgHead, oMemOperate, m_pRedisNodeSession,
                        m_pProxySess->m_oNosqlDbRelative["tables"][oMemOperate.db_operate().table_name()]["cols"]);
        return GetLabor()->ExecStep(pStepReadFromRedisForWrite);
    }
    else
    {
        LOG4_ERROR("error %d: %s", ERR_UNDEFINE_REDIS_OPERATE, "only hash an string had update dataset supported!");
        Response(stMsgShell, oInMsgHead, ERR_UNDEFINE_REDIS_OPERATE,"only hash an string had update dataset supported!");
        return(false);
    }
}

bool CmdDataProxy::Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,int iErrno, const std::string& strErrMsg)
{
    LOG4_TRACE("%d: %s", iErrno, strErrMsg.c_str());
    DataMem::MemRsp oRsp;
    oRsp.set_err_no(iErrno);
    oRsp.set_err_msg(strErrMsg);
    return GetLabor()->SendToClient(stMsgShell, oInMsgHead, oRsp);
}

bool CmdDataProxy::GetNodeSession(const DataMem::MemOperate& oMemOperate)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::string strSectionFactor;
    if (!m_pProxySess->GetSectionFactor(oMemOperate.redis_operate().data_purpose(),oMemOperate.section_factor(),strSectionFactor))
    {
    	LOG4_ERROR("GetSectionFactor error(%s)",m_pProxySess->m_pErrBuff);
		return(false);
    }
	m_pRedisNodeSession = (SessionRedisNode*)net::GetSession(strSectionFactor, "net::SessionRedisNode");
    return (NULL != m_pRedisNodeSession);
}

} /* namespace core */
