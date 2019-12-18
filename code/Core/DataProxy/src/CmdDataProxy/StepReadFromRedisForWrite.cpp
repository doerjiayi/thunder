/*******************************************************************************
 * Project:  DataProxyServer
 * @file     StepReadFromRedisForWrite.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年4月6日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepReadFromRedisForWrite.hpp"

namespace core
{

StepReadFromRedisForWrite::StepReadFromRedisForWrite(
                const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,
                const DataMem::MemOperate& oMemOperate,
                SessionRedisNode* pNodeSession,
                const util::CJsonObject& oTableFields,
                const std::string& strKeyField)
    : m_stMsgShell(stMsgShell), m_oReqMsgHead(oInMsgHead), m_oMemOperate(oMemOperate),
      m_oTableFields(oTableFields), m_strKeyField(strKeyField),
      m_pRedisNodeSession(pNodeSession), pStepSendToDbAgent(NULL), pStepWriteToRedis(NULL)
{
}

StepReadFromRedisForWrite::~StepReadFromRedisForWrite()
{
}

net::E_CMD_STATUS StepReadFromRedisForWrite::Emit(int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (!m_pRedisNodeSession)return(net::STATUS_CMD_FAULT);

    util::RedisCmd* pRedisCmd = RedisCmd();
    if (pRedisCmd)
    {
        const std::string& hash_key = m_oMemOperate.redis_operate().hash_key().size() ? m_oMemOperate.redis_operate().hash_key() :m_oMemOperate.redis_operate().key_name();
        if (!m_pRedisNodeSession->GetRedisNode(hash_key, m_strMasterNode, m_strSlaveNode))
        {
            Response(m_stMsgShell, m_oReqMsgHead, ERR_REDIS_NODE_NOT_FOUND, "redis node not found!");
            return(net::STATUS_CMD_FAULT);
        }
        pRedisCmd->SetCmd(m_oMemOperate.redis_operate().redis_cmd_read());
        pRedisCmd->Append(m_oMemOperate.redis_operate().key_name());
        for (int i = 0; i < m_oMemOperate.redis_operate().fields_size(); ++i)
        {
            if (m_oMemOperate.redis_operate().fields(i).col_name().size() > 0)
            {
                pRedisCmd->Append(m_oMemOperate.redis_operate().fields(i).col_name());
            }
            if (m_oMemOperate.redis_operate().fields(i).col_value().size() > 0)//m_oMemOperate.redis_operate().fields(i).col_name().size() > 0 ||
            {
                pRedisCmd->Append(m_oMemOperate.redis_operate().fields(i).col_value());
            }
        }

        if (net::RegisterCallback(m_strMasterNode, this))
        {
            return(net::STATUS_CMD_RUNNING);
        }
        Response(m_stMsgShell, m_oReqMsgHead, ERR_REGISTERCALLBACK_REDIS, "RegisterCallback(net::RedisStep) error!");
        LOG4_ERROR("RegisterCallback(%s, StepReadFromRedisForWrite) error!", m_strMasterNode.c_str());
        return(net::STATUS_CMD_FAULT);
    }
    Response(m_stMsgShell, m_oReqMsgHead, ERR_NEW, "malloc space for util::RedisCmd error!");
    return(net::STATUS_CMD_FAULT);
}

net::E_CMD_STATUS StepReadFromRedisForWrite::Callback(const redisAsyncContext *c, int status, redisReply* pReply)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    char szErrMsg[256] = {0};
    if (REDIS_OK != status)
    {
        snprintf(szErrMsg, sizeof(szErrMsg), "redis %s cmd status %d!", m_strMasterNode.c_str(), status);
        Response(m_stMsgShell, m_oReqMsgHead, ERR_REDIS_CMD, szErrMsg);
        return(net::STATUS_CMD_FAULT);
    }
    if (NULL == pReply)
    {
        snprintf(szErrMsg, sizeof(szErrMsg), "redis %s error %d: %s!", m_strMasterNode.c_str(), c->err, c->errstr);
        Response(m_stMsgShell, m_oReqMsgHead, ERR_REDIS_CMD, szErrMsg);
        return(net::STATUS_CMD_FAULT);
    }
    LOG4_TRACE("redis reply->type = %d", pReply->type);
    if (REDIS_REPLY_ERROR == pReply->type)
    {
        snprintf(szErrMsg, sizeof(szErrMsg), "redis %s error %d: %s!", m_strSlaveNode.c_str(), pReply->type, pReply->str);
        Response(m_stMsgShell, m_oReqMsgHead, ERR_REDIS_CMD, szErrMsg);
        return(net::STATUS_CMD_FAULT);
    }
    if (REDIS_REPLY_NIL == pReply->type) // redis中数据为空，只需update db中的数据
    {
        return(ExecUpdate(true));
    }
    // 从redis中读到数据
    if (REDIS_REPLY_STRING == pReply->type) // dataset返回的结果仅有 REDIS_REPLY_STRING 一种可能
    {
    	//更新dataset的数据
        DataMem::Field* pDatasetField = NULL;
        DataMem::Record oRecord;
        int iRecoredFieldSize = 0;
        int iDictFieldSize = 0;
        if (0 == m_oMemOperate.redis_operate().fields_size())   // field_size()只会有0和1两种情况，在CmdDataProxy中已做验证
        {
            pDatasetField = m_oMemOperate.mutable_redis_operate()->add_fields();
        }
        else
        {
            pDatasetField = m_oMemOperate.mutable_redis_operate()->mutable_fields(0);
        }
        if (!oRecord.ParseFromArray(pReply->str, pReply->len))
        {
            snprintf(szErrMsg, sizeof(szErrMsg), "failed to parse redis dataset record \"%s\"", pReply->str);
            LOG4_ERROR("%d: %s!", ERR_UNEXPECTED_REDIS_REPLY, szErrMsg);
            Response(m_stMsgShell, m_oReqMsgHead, ERR_UNEXPECTED_REDIS_REPLY, szErrMsg);
            return(net::STATUS_CMD_FAULT);
        }
        if (oRecord.field_info_size() == 0)
        {
            LOG4_ERROR("%d: can not update empty redis dataset record!", ERR_UNEXPECTED_REDIS_REPLY);
            Response(m_stMsgShell, m_oReqMsgHead, ERR_UNEXPECTED_REDIS_REPLY, "can not update empty redis dataset record");
            return(net::STATUS_CMD_FAULT);
        }
        iRecoredFieldSize = oRecord.field_info_size();
        iDictFieldSize = m_oTableFields.GetArraySize();
        if (iRecoredFieldSize < iDictFieldSize)//原来多出的字段设置值为空
        {
            LOG4_TRACE("iRecoredFieldSize = %d, iDictFieldSize = %d", iRecoredFieldSize, iDictFieldSize);
            DataMem::Field* pField = NULL;
            for (int i = 0; i < iDictFieldSize - iRecoredFieldSize; ++i)
            {
                pField = oRecord.add_field_info();
                pField->set_col_value("");
            }
        }
        for (int j = 0; j < m_oMemOperate.db_operate().fields_size(); ++j)
        {
            for (int k = 0; k < m_oTableFields.GetArraySize(); ++k)
            {
                if (m_oMemOperate.db_operate().fields(j).col_name() == m_oTableFields(k) || m_oMemOperate.db_operate().fields(j).col_as() == m_oTableFields(k))
                {
                    oRecord.mutable_field_info(k)->set_col_value(m_oMemOperate.db_operate().fields(j).col_value());
                }
            }
        }
        pDatasetField->set_col_value(oRecord.SerializeAsString());
        return(ExecUpdate());
    }
    else
    {
        snprintf(szErrMsg, sizeof(szErrMsg), "unexprected redis reply type %d for update redis dataset!", pReply->type);
        Response(m_stMsgShell, m_oReqMsgHead, ERR_UNEXPECTED_REDIS_REPLY, szErrMsg);
        return(net::STATUS_CMD_FAULT);
    }
    return(net::STATUS_CMD_FAULT);
}

net::E_CMD_STATUS StepReadFromRedisForWrite::ExecUpdate(bool bDbOnly)
{
    if (bDbOnly)
    {
        m_oMemOperate.clear_redis_operate();
        pStepSendToDbAgent = new StepSendToDbAgent(m_stMsgShell, m_oReqMsgHead, m_oMemOperate,m_pRedisNodeSession, RELATIVE_DATASET, &m_oTableFields, m_strKeyField);
        GetLabor()->ExecStep(pStepSendToDbAgent);
        return(net::STATUS_CMD_COMPLETED);
    }
    else
    {
        pStepSendToDbAgent = new StepSendToDbAgent(m_stMsgShell, m_oReqMsgHead, m_oMemOperate,m_pRedisNodeSession, RELATIVE_DATASET, &m_oTableFields, m_strKeyField);
        pStepWriteToRedis = new StepWriteToRedis(m_stMsgShell, m_oReqMsgHead, m_oMemOperate.redis_operate(), m_pRedisNodeSession, pStepSendToDbAgent);
        GetLabor()->ExecStep(pStepWriteToRedis);
		return(net::STATUS_CMD_COMPLETED);
    }
}

} /* namespace core */
