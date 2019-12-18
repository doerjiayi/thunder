/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepLogoutToLogic.cpp
 * @brief    把数据发到逻辑服务器
 * @author   lbh
 * @date:    2019年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepLogoutToLogic.hpp"

#include "user_basic.pb.h"
#include "util/CBuffer.hpp"

namespace im
{

StepLogoutToLogic::StepLogoutToLogic(const net::tagMsgShell& stMsgShell,
		const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,const im_user::ulogout &oInAsk)
    :m_oInAsk(oInAsk), m_iTimeoutNum(0),m_stMsgShell(stMsgShell),m_oInMsgHead(oInMsgHead),m_oInMsgBody(oInMsgBody)
{
}

StepLogoutToLogic::~StepLogoutToLogic()
{
}

net::E_CMD_STATUS StepLogoutToLogic::Emit(int iErrno, const std::string& strErrMsg,const std::string& strErrShow)
{
    LOG4_DEBUG("seq[%llu] StepLogoutToLogic::Emit!", m_oInMsgHead.seq());
    MsgHead oOutMsgHead = m_oInMsgHead;
    oOutMsgHead.set_seq(GetSequence());     // 更换消息头的seq后直接转发
    GetLabor()->SendToSession("LOGIC",oOutMsgHead,m_oInMsgBody);
    LOG4_DEBUG("cmd[%llu]  Logout  Disconnect", m_oInMsgHead.cmd());
    return(net::STATUS_CMD_RUNNING);
}

net::E_CMD_STATUS StepLogoutToLogic::Callback(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data)
{
    LOG4_DEBUG("seq[%llu] StepLogoutToLogic::Callback ok!", oInMsgHead.seq());
	GetLabor()->Disconnect(m_stMsgShell,false);
    return(net::STATUS_CMD_COMPLETED);
}

net::E_CMD_STATUS StepLogoutToLogic::Timeout()
{
    ++m_iTimeoutNum;
    if (m_iTimeoutNum <= 3)
    {
        GetLabor()->Disconnect(m_stMsgShell);
        return(net::STATUS_CMD_COMPLETED);
    }
    else
    {
        LOG4_ERROR("error timeout %d times!", m_iTimeoutNum);
        return(net::STATUS_CMD_FAULT);
    }
}

} /* namespace im */
