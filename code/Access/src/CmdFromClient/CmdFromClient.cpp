/*******************************************************************************
 * Project:  AccessServer
 * @file     CmdFromClient.cpp
 * @brief 
 * @author   lbh
 * @date:    2019年10月21日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdFromClient.hpp"

MUDULE_CREATE(im::CmdFromClient);

namespace im
{

bool CmdFromClient::AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    net::ExecStep(new StepFromClient(stMsgShell, oInMsgHead, oInMsgBody));
    return true;
}

} /* namespace im */
