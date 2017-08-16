/*******************************************************************************
 * Project:  HelloThunder
 * @file     StepGetHelloName.cpp
 * @brief 
 * @author   chenjiayi
 * @date:    2017年10月23日
 * @note
 * Modify history:
 ******************************************************************************/
#include "../ModuleHello/StepGetHelloName.hpp"

namespace hello
{

StepGetHelloName::StepGetHelloName(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
    m_stMsgShell = stMsgShell;
}

StepGetHelloName::~StepGetHelloName()
{
}

thunder::E_CMD_STATUS StepGetHelloName::Callback(const redisAsyncContext *c, int status, redisReply* pReply)
{
    if (REDIS_OK == status && pReply != NULL)
    {
        if (REDIS_REPLY_STRING == pReply->type || REDIS_REPLY_ERROR == pReply->type)
        {
            MsgHead oOutMsgHead;
            MsgBody oOutMsgBody;
            oOutMsgHead.set_cmd(m_oMsgHead.cmd() + 1);
            oOutMsgHead.set_seq(m_oMsgHead.seq());
            oOutMsgBody.set_body(pReply->str);
            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
            LOG4CPLUS_DEBUG_FMT(GetLogger(), "%s", pReply->str);
            LOG4CPLUS_DEBUG_FMT(GetLogger(), "SendTo(%d, %llu)!", m_stMsgShell.iFd, m_stMsgShell.ulSeq);
            SendTo(m_stMsgShell, oOutMsgHead, oOutMsgBody);
            return thunder::STATUS_CMD_COMPLETED;
        }
        else
        {
            MsgHead oOutMsgHead;
            MsgBody oOutMsgBody;
            oOutMsgHead.set_cmd(m_oMsgHead.cmd() + 1);
            oOutMsgHead.set_seq(m_oMsgHead.seq());
            oOutMsgBody.set_body("OK");
            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
            LOG4CPLUS_DEBUG_FMT(GetLogger(), "SendTo(%d, %llu)!", m_stMsgShell.iFd, m_stMsgShell.ulSeq);
            SendTo(m_stMsgShell, oOutMsgHead, oOutMsgBody);
            return thunder::STATUS_CMD_COMPLETED;
        }
    }
    else
    {
        LOG4CPLUS_ERROR(GetLogger(), "pReply is null!");
        return thunder::STATUS_CMD_FAULT;
    }
}

} /* namespace hello */
