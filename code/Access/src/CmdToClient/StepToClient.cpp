/*******************************************************************************
 * Project:  AccessServer
 * @file     StepToClient.cpp
 * @brief 
 * @author   lbh
 * @date:    2019年10月21日
 * @note
 * Modify history:
 ******************************************************************************/
#include "RobotError.h"
#include "StepToClient.hpp"

namespace im
{

StepToClient::StepToClient(
                const net::tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
    : net::Step(stMsgShell, oInMsgHead, oInMsgBody)
{
}

StepToClient::~StepToClient()
{
}

net::E_CMD_STATUS StepToClient::Emit(int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
    bool bSendResult = false;
    MsgHead oOutMsgHead = m_oReqMsgHead;
    oOutMsgHead.set_seq(GetSequence());     // 更换消息头的seq后直接转发
    if (m_oReqMsgBody.has_session_id())
    {
        char szIdentify[32] = {0};
        snprintf(szIdentify, sizeof(szIdentify), "%u", m_oReqMsgBody.session_id());
        bSendResult = net::SendTo(szIdentify, oOutMsgHead, m_oReqMsgBody);
    }
    else if (m_oReqMsgBody.has_session())
    {
        bSendResult = net::SendTo(m_oReqMsgBody.session(), oOutMsgHead, m_oReqMsgBody);
    }
    else
    {
        MsgBody oOutMsgBody;
        OrdinaryResponse oRes;
        oRes.set_err_no(robot::ERR_NO_SESSION_ID_IN_MSGBODY);
        oRes.set_err_msg("no session id!");
        oOutMsgBody.set_body(oRes.SerializeAsString());
        oOutMsgHead.set_cmd(net::CMD_RSP_SYS_ERROR);
        oOutMsgHead.set_seq(m_oReqMsgHead.seq());
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        net::SendTo(m_stReqMsgShell, oOutMsgHead, oOutMsgBody);
        return(net::STATUS_CMD_COMPLETED);
    }

    if (bSendResult)
    {
        return(net::STATUS_CMD_RUNNING);
    }
    else
    {
        MsgBody oOutMsgBody;
        OrdinaryResponse oRes;
        oRes.set_err_no(robot::ERR_USER_OFFLINE);
        oRes.set_err_msg("user offline!");
        oOutMsgBody.set_body(oRes.SerializeAsString());
        oOutMsgHead.set_cmd(net::CMD_RSP_SYS_ERROR);
        oOutMsgHead.set_seq(m_oReqMsgHead.seq());
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        net::SendTo(m_stReqMsgShell, oOutMsgHead, oOutMsgBody);
        return(net::STATUS_CMD_COMPLETED);
    }
}

net::E_CMD_STATUS StepToClient::Callback(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data)
{
    m_oReqMsgHead.set_cmd(oInMsgHead.cmd());
    m_oReqMsgHead.set_msgbody_len(oInMsgBody.ByteSize());
    net::SendTo(m_stReqMsgShell, m_oReqMsgHead, oInMsgBody);
    return(net::STATUS_CMD_COMPLETED);
}

net::E_CMD_STATUS StepToClient::Timeout()
{
    return(net::STATUS_CMD_FAULT);
}


} /* namespace im */
