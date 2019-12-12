/*******************************************************************************
 * Project:  Beacon
 * @file     CmdNodeDisconnect.cpp
 * @brief 
 * @author   bwar
 * @date:    Feb 14, 2017
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdNodeDisconnect.hpp"

namespace coor
{

CmdNodeDisconnect::CmdNodeDisconnect(int32 iCmd)
    :    m_pSessionOnlineNodes(nullptr)
{
}

CmdNodeDisconnect::~CmdNodeDisconnect()
{
}

bool CmdNodeDisconnect::Init()
{
    return(true);
}

bool CmdNodeDisconnect::AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oMsgHead,const MsgBody& oMsgBody)
{
    util::CJsonObject oNodeInfo;
    if (nullptr == m_pSessionOnlineNodes)
    {
        m_pSessionOnlineNodes = std::dynamic_pointer_cast<SessionOnlineNodes>(GetSession("coor::SessionOnlineNodes"));
        if (nullptr == m_pSessionOnlineNodes)
        {
            LOG4_ERROR("no session node found!");
        }
    }
    if (oNodeInfo.Parse(oMsgBody.data()))
    {
        LOG4_DEBUG("%s disconnect, remove from node list.", oMsgBody.data().c_str());
        m_pSessionOnlineNodes->RemoveNode(oMsgBody.data());
    }
    else
    {
        LOG4_DEBUG("%s disconnected.", oMsgBody.data().c_str());
    }
    return(true);
}

} /* namespace coor */
