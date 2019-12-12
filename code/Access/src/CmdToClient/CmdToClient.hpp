/*******************************************************************************
 * Project:  AccessServer
 * @file     CmdToClient.hpp
 * @brief 
 * @author   lbh
 * @date:    2019年10月20日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDTOCLIENT_CMDTOCLIENT_HPP_
#define SRC_CMDTOCLIENT_CMDTOCLIENT_HPP_

#include "cmd/Cmd.hpp"
#include "StepToClient.hpp"

namespace im
{

class CmdToClient: public net::Cmd
{
public:
    CmdToClient(){}
    virtual ~CmdToClient(){}

    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace im */

#endif /* SRC_CMDTOCLIENT_CMDTOCLIENT_HPP_ */
