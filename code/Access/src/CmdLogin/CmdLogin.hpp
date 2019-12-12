/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdLogin.hpp
 * @brief    被告知Worker信息
 * @author   lbh
 * @date:    2019年8月24日
 * @note     用户登录进行验证码校验，校验成功后登录成功，
 * Modify history:
 ******************************************************************************/
#ifndef CMD_LOGIN_HPP_
#define CMD_LOGIN_HPP_

#include "cmd/Cmd.hpp"
#include "StepToLogic.hpp"

namespace im
{

/**
 * @brief   被告知Worker信息
 * @author  ty
 * @date    2019年8月9日
 * @note    A连接B成功后，A将己方的Worker信息A_attr（节点类型和Worker唯一标记）告知B，
 * B收到A后将A_attr登记起来，并将己方的Worker信息B_attr回复A。  因为这是B对A的响应，
 * 所以无需开启Step等待A的接收结果，若A为收到B的响应，则应再告知一遍。
 */
class CmdLogin : public net::Cmd
{
public:
    CmdLogin();
    virtual ~CmdLogin();
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace im */

#endif
