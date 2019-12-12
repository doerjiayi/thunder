/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepLogoutToLogic.hpp
 * @brief    用户退出的处理逻辑
 * @author   ty
 * @date:    2019年10月13日
 * @note     客户端正常退出所发送的请求
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_LOGOUT_TO_LOGIC_HPP_
#define SRC_STEP_LOGOUT_TO_LOGIC_HPP_

#include "step/Step.hpp"
#include "user.pb.h"
#include "common.pb.h"
#include "RobotError.h"
namespace im
{

class StepLogoutToLogic : public net::Step
{
public:
    StepLogoutToLogic(const net::tagMsgShell& stMsgShell,
    		const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,const im_user::ulogout &oInAsk);
    virtual ~StepLogoutToLogic();

    virtual net::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    virtual net::E_CMD_STATUS Callback(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data = NULL);
    virtual net::E_CMD_STATUS Timeout();

public:
    im_user::ulogout  m_oInAsk;
private:
    int m_iTimeoutNum;          ///< 超时次数
    net::tagMsgShell m_stMsgShell;
    MsgHead m_oInMsgHead;
    MsgBody m_oInMsgBody;

};

} /* namespace im */

#endif 
