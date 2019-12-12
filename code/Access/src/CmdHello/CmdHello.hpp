/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdHello.hpp
 * @brief    Server测试程序
 * @author   lbh
 * @date:    2019年8月24日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef PLUGINS_CMDHELLO_HPP_
#define PLUGINS_CMDHELLO_HPP_

#include "cmd/Cmd.hpp"

namespace im
{

class CmdHello: public net::Cmd
{
public:
    CmdHello(){}
    virtual ~CmdHello(){}
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace im */

#endif /* PLUGINS_CMDHELLO_HPP_ */
