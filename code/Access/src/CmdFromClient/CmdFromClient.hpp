/*******************************************************************************
 * Project:  AccessServer
 * @file     CmdFromClient.hpp
 * @brief 
 * @author   lbh
 * @date:    2019年10月21日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDFROMCLIENT_CMDFROMCLIENT_HPP_
#define SRC_CMDFROMCLIENT_CMDFROMCLIENT_HPP_

#include "cmd/Cmd.hpp"
#include "StepFromClient.hpp"

namespace im
{

class CmdFromClient: public net::Cmd
{
public:
    CmdFromClient(){}
    virtual ~CmdFromClient(){}
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace im */

#endif /* SRC_CMDFROMCLIENT_CMDFROMCLIENT_HPP_ */
