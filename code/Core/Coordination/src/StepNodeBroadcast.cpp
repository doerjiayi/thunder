/*******************************************************************************
 * Project:  Beacon
 * @file     StepNodeBroadcast.cpp
 * @brief 
 * @author   bwar
 * @date:    Dec 28, 2016
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepNodeBroadcast.hpp"

namespace coor
{

StepNodeBroadcast::StepNodeBroadcast(const std::string& strNodeIdentity, int32 iCmd, const MsgBody& oMsgBody)
    : m_strTargetNodeIdentity(strNodeIdentity), m_iCmd(iCmd), m_strBody(oMsgBody)
{
}

StepNodeBroadcast::~StepNodeBroadcast()
{
}

net::E_CMD_STATUS StepNodeBroadcast::Emit(int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
    if (net::SendTo(m_strTargetNodeIdentity, m_iCmd, GetSequence(), m_strBody))
    {
        return(net::STATUS_CMD_RUNNING);
    }
    else
    {
        return(net::STATUS_CMD_FAULT);
    }
}

net::E_CMD_STATUS StepNodeBroadcast::Callback(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, void* data)
{
    if (net::ERR_OK == oInMsgBody.rsp_result().code())
    {
        return(net::STATUS_CMD_COMPLETED);
    }
    else
    {
        LOG4_ERROR("error %d: %s!", oInMsgBody.DebugString().c_str());
        return(net::STATUS_CMD_FAULT);
    }
}

net::E_CMD_STATUS StepNodeBroadcast::Timeout()
{
    LOG4_ERROR("timeout!");
    return(net::STATUS_CMD_FAULT);
}

} /* namespace coor */
