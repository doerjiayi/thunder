/*******************************************************************************
 * Project:  Net
 * @file     RedisStep.cpp
 * @brief 
 * @author   cjy
 * @date:    2019年8月15日
 * @note
 * Modify history:
 ******************************************************************************/
#include "RedisStep.hpp"

namespace net
{

RedisStep::RedisStep(Step* pNextStep)
    : Step(pNextStep), m_pRedisCmd(NULL)
{
    m_pRedisCmd = new util::RedisCmd();
}

RedisStep::RedisStep(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, Step* pNextStep)
    : Step(stReqMsgShell, oReqMsgHead, oReqMsgBody, pNextStep), m_pRedisCmd(NULL)
{
    m_pRedisCmd = new util::RedisCmd();
}

RedisStep::~RedisStep()
{
    if (m_pRedisCmd != NULL)
    {
        delete m_pRedisCmd;
        m_pRedisCmd = NULL;
    }
}

} /* namespace net */
