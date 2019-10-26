#include "StepGeneralRedis.hpp"

namespace core
{


net::E_CMD_STATUS StepGeneralRedis::Emit(int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
//    LOG4_TRACE("%s()", __FUNCTION__);
    util::RedisCmd* pRedisCmd = RedisCmd();
    if (pRedisCmd && pRedisCmd->GetCmd().size() == 0)
    {
        pRedisCmd->SetCmd(m_oRedisOperate.redis_cmd_read().size() > 0 ?m_oRedisOperate.redis_cmd_read():m_oRedisOperate.redis_cmd_write());
        pRedisCmd->Append(m_oRedisOperate.key_name());
        for (int i = 0; i < m_oRedisOperate.fields_size(); ++i)
        {
            if (m_oRedisOperate.fields(i).col_name().size() > 0)
            {
                pRedisCmd->Append(m_oRedisOperate.fields(i).col_name());
            }
            if (m_oRedisOperate.fields(i).col_value().size() > 0)
            {
                pRedisCmd->Append(m_oRedisOperate.fields(i).col_value());
            }
        }
//        LOG4_DEBUG("pRedisCmd:%s",pRedisCmd->GetCmd().c_str());
        ++m_iEmitNum;
        UnsetRegistered();
        if (net::RegisterCallback(m_strNode, this))
        {
            return(net::STATUS_CMD_RUNNING);
        }
        LOG4_ERROR("RegisterCallback(%s, StepGeneralRedis) error!", m_strNode.c_str());
        return(net::STATUS_CMD_FAULT);
    }
    LOG4_ERROR("pRedisCmd error!", m_strNode.c_str());
    return(net::STATUS_CMD_FAULT);
}

net::E_CMD_STATUS StepGeneralRedis::Callback(const redisAsyncContext *c, int status, redisReply* pReply)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    char szErrMsg[256] = {0};
    if (REDIS_OK != status)
    {
        if (1 == m_iEmitNum)//再尝试一次
        {
            LOG4_WARN("redis %s cmd status %d!", m_strNode.c_str(), status);
            return(Emit(ERR_OK));
        }
        snprintf(szErrMsg, sizeof(szErrMsg), "redis %s cmd status %d!", m_strNode.c_str(), status);
        LOG4_WARN("%s() ok.szErrMsg:%s", __FUNCTION__,szErrMsg);
        m_uiPingNode = 0;
        return(net::STATUS_CMD_FAULT);
    }
    if (NULL == pReply)
    {
        snprintf(szErrMsg, sizeof(szErrMsg), "redis %s error %d: %s!", m_strNode.c_str(), c->err, c->errstr);
        LOG4_WARN("%s() failed.szErrMsg:%s", __FUNCTION__,szErrMsg);
        m_uiPingNode = 0;
        return(net::STATUS_CMD_FAULT);
    }
    LOG4_TRACE("redis reply->type = %d", pReply->type);
    if (REDIS_REPLY_ERROR == pReply->type)
    {
        snprintf(szErrMsg, sizeof(szErrMsg), "redis %s error %d: %s!", m_strNode.c_str(), pReply->type, pReply->str);
        LOG4_WARN("%s() failed.szErrMsg:%s", __FUNCTION__,szErrMsg);
        m_uiPingNode = 0;
        return(net::STATUS_CMD_FAULT);
    }
    if (REDIS_REPLY_NIL == pReply->type)
    {
        LOG4_TRACE("%s() ok.pReply:null.", __FUNCTION__);
        m_uiPingNode = 1;
        return(net::STATUS_CMD_COMPLETED);
    }
    if (REDIS_REPLY_ARRAY == pReply->type)
    {
        for(uint32 i = 0;i < pReply->elements;++i)
        {
            redisReply* pElememtReply = pReply->element[i];
            LOG4_TRACE("%s() ok.pElememtReply:%s", __FUNCTION__,pElememtReply->str);
        }
        m_uiPingNode = 1;
        return(net::STATUS_CMD_COMPLETED);
    }
    LOG4_TRACE("%s() ok.pReply:%s", __FUNCTION__,pReply->str);
    m_uiPingNode = 1;
    return(net::STATUS_CMD_COMPLETED);
}

}
