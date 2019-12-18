/*******************************************************************************
 * Project:  Center
 * @file     CmdNodeDisconnect.cpp
 * @brief 
 * @author   bwar
 * @date:    Feb 14, 2017
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdNodeDisconnect.hpp"

MUDULE_CREATE(coor::CmdNodeDisconnect);

namespace coor
{

bool CmdNodeDisconnect::Init()
{
    return(true);
}

bool CmdNodeDisconnect::AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oMsgHead,const MsgBody& oMsgBody)
{
    util::CJsonObject oNodeInfo;
    if (nullptr == m_pSessionOnlineNodes)
    {
        m_pSessionOnlineNodes = (SessionOnlineNodes*)net::GetSession("coor::SessionOnlineNodes");
        if (nullptr == m_pSessionOnlineNodes)
        {
            LOG4_ERROR("no session node found!");
            return false;
        }
    }
    if (oNodeInfo.Parse(oMsgBody.body()))
    {
        LOG4_DEBUG("%s disconnect, remove from node list.", oMsgBody.body().c_str());
        m_pSessionOnlineNodes->RemoveNode(oMsgBody.body());
    }
    else
    {
        LOG4_DEBUG("%s disconnected.", oMsgBody.body().c_str());
    }
    return(true);
}

} /* namespace coor */
