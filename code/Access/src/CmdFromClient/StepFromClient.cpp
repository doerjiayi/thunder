/*******************************************************************************
 * Project:  AccessServer
 * @file     StepFromClient.cpp
 * @brief 
 * @author   lbh
 * @date:    2019年10月21日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepFromClient.hpp"

namespace im
{

StepFromClient::StepFromClient(
                const net::tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
    : net::Step(stMsgShell, oInMsgHead, oInMsgBody)
{
}

StepFromClient::~StepFromClient()
{
}

net::E_CMD_STATUS StepFromClient::Emit(int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
    MsgHead oOutMsgHead = m_oReqMsgHead;
    oOutMsgHead.set_seq(GetSequence());     // 更换消息头的seq后直接转发
    net::SendToAuto("LOGIC", oOutMsgHead, m_oReqMsgBody);
    return(net::STATUS_CMD_RUNNING);
}

net::E_CMD_STATUS StepFromClient::Callback(
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

net::E_CMD_STATUS StepFromClient::Timeout()
{
//    MsgHead oOutMsgHead = m_oMsgHead;
//    MsgBody oOutMsgBody;
//    OrdinaryResponse oRes;
//    oRes.set_err_no(ERR_LOGIC_SERVER_TIMEOUT);
//    oRes.set_err_msg("logic timeout!");
//    oOutMsgBody.set_body(oRes.SerializeAsString());
//    oOutMsgHead.set_cmd(m_oMsgHead.cmd() + 1);
//    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
//    net::SendTo(m_stMsgShell, oOutMsgHead, oOutMsgBody);
    LOG4_WARN( "cmd %u, seq %lu, logic timeout!", m_oReqMsgHead.cmd(), m_oReqMsgHead.seq());
    return(net::STATUS_CMD_FAULT);
}

} /* namespace im */
