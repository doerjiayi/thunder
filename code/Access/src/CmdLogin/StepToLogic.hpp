/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepToLogic.hpp
 * @brief    告知对端己方Worker进程信息
 * @author   lbh
 * @date:    2019年8月13日
 * @note     在作为客户端发起对一个服务端的连接，当对端返回连接成功信息时，启动
 * StepToLogic向对端发送己方Worker信息，并等待对端回复对方Worker信息回调。
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_TO_LOGIC_HPP_
#define SRC_STEP_TO_LOGIC_HPP_

#include "user.pb.h"
#include "step/Step.hpp"
#include "StepGetOfflineMsg.hpp"

namespace im
{

class StepToLogic : public net::Step
{
public:
    StepToLogic(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody);
    virtual ~StepToLogic();

    virtual net::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    virtual net::E_CMD_STATUS Callback(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data = NULL);
    virtual net::E_CMD_STATUS Timeout();

private:
    int m_iTimeoutNum;          ///< 超时次数
    net::tagMsgShell m_stMsgShell;
    MsgHead m_oInMsgHead;
    MsgBody m_oInMsgBody;

    MsgHead m_oOutMsgHead;
    MsgBody m_oOutMsgBody;
	bool    m_bLoginOk;//登录是否OK
	im_user::ulogin m_oInAsk;

};

} /* namespace im */

#endif 
