/*******************************************************************************
 * Project:  AccessServer
 * @file     CmdToClient.cpp
 * @brief 
 * @author   lbh
 * @date:    2019年10月20日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdToClient.hpp"

MUDULE_CREATE(im::CmdToClient);

namespace im
{

bool CmdToClient::AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    net::ExecStep(new StepToClient(stMsgShell, oInMsgHead, oInMsgBody));
    return true;
}

} /* namespace im */
