/*******************************************************************************
 * Project:  CenterServer
 * @file     CmdNodeDisconnect.cpp
 * @brief 
 * @author   cjy
 * @date:    2015年8月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdNodeDisconnect.hpp"

MUDULE_CREATE(core::CmdNodeDisconnect);

namespace core
{

bool CmdNodeDisconnect::Init()
{
    if (boInit)
    {
        return true;
    }
    pSess = GetNodeSession(true);
    if(!pSess)
    {
        LOG4_ERROR("failed to get GetNodeSession");
        return false;
    }
    boInit = true;
    return true;
}

bool CmdNodeDisconnect::AnyMessage(
                const net::tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    return pSess->DelNode(oInMsgBody.body());//delNodeIdentify
}




} /* namespace core */
