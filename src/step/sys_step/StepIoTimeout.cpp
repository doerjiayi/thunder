/*******************************************************************************
 * Project:  Thunder
 * @file     StepIoTimeout.cpp
 * @brief 
 * @author   cjy
 * @date:    2015年10月31日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepIoTimeout.hpp"

namespace thunder
{

StepIoTimeout::StepIoTimeout(const MsgShell& stMsgShell, struct ev_timer* pWatcher)
    : m_stMsgShell(stMsgShell), watcher(pWatcher)
{
}

StepIoTimeout::~StepIoTimeout()
{
}

E_CMD_STATUS StepIoTimeout::Emit(
                int iErrno,
                const std::string& strErrMsg,
                const std::string& strErrShow)
{
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    oOutMsgHead.set_cmd(CMD_REQ_BEAT);
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_msgbody_len(0);
    LOG4_TRACE("StepIoTimeout::Emit stMsgShell(%d,%u) ClientAddr(%s) GetConnectIdentify(%s)",
                    m_stMsgShell.iFd,m_stMsgShell.ulSeq,GetLabor()->GetClientAddr(m_stMsgShell).c_str(),
                    GetLabor()->GetConnectIdentify(m_stMsgShell).c_str());
    if (SendTo(m_stMsgShell, oOutMsgHead, oOutMsgBody))
    {
        return(STATUS_CMD_RUNNING);
    }
    else        // SendTo错误会触发断开连接和回收资源
    {
        return(STATUS_CMD_FAULT);
    }
}

E_CMD_STATUS StepIoTimeout::Callback(
                const MsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody,
                void* data)
{
    LOG4_TRACE("StepIoTimeout::Callback stMsgShell(%d,%u) ClientAddr(%s) GetConnectIdentify(%s)",
                        m_stMsgShell.iFd,m_stMsgShell.ulSeq,GetLabor()->GetClientAddr(stMsgShell).c_str(),
                        GetLabor()->GetConnectIdentify(stMsgShell).c_str());
    GetLabor()->IoTimeout(watcher, true);
    return(STATUS_CMD_COMPLETED);
}

E_CMD_STATUS StepIoTimeout::Timeout()
{
    LOG4_TRACE("StepIoTimeout::Timeout stMsgShell(%d,%u) ClientAddr(%s) ConnectIdentify(%s)",
                            m_stMsgShell.iFd,m_stMsgShell.ulSeq,GetLabor()->GetClientAddr(m_stMsgShell).c_str(),
                            GetLabor()->GetConnectIdentify(m_stMsgShell).c_str());
    GetLabor()->IoTimeout(watcher, false);
    return(STATUS_CMD_FAULT);
}

} /* namespace thunder */
