/*******************************************************************************
 * Project:  Center
 * @file     CmdNodeReport.cpp
 * @brief 
 * @author   bwar
 * @date:    Feb 14, 2017
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdNodeReport.hpp"

MUDULE_CREATE(coor::CmdNodeReport);

namespace coor
{

bool CmdNodeReport::Init()
{
    return(true);
}

bool CmdNodeReport::AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oMsgHead,const MsgBody& oMsgBody)
{
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    util::CJsonObject oNodeInfo;
    util::CJsonObject oNodeId;
    if (nullptr == m_pSessionOnlineNodes)
    {
        m_pSessionOnlineNodes = (SessionOnlineNodes*)net::GetSession("coor::SessionOnlineNodes");
        if (nullptr == m_pSessionOnlineNodes)
        {
            LOG4_ERROR("no session node found!");
            return(false);
        }
    }
    if (oNodeInfo.Parse(oMsgBody.body()))
    {
        uint16 unNodeId = m_pSessionOnlineNodes->AddNode(oNodeInfo);
        if (0 == unNodeId)
        {
            LOG4_ERROR("there is no valid node_id in the system!");
        }
        else
        {
            oNodeId.Add("node_id", unNodeId);
            oOutMsgBody.set_body(oNodeId.ToString());
            LOG4_INFO("AddNode node_id(%u)!",unNodeId);
        }
    }
    else
    {
        LOG4_ERROR("failed to parse node info json from MsgBody.data()!");
    }
    return(GetLabor()->SendToClient(stMsgShell, oMsgHead, oNodeId.ToString()));
}

} /* namespace coor */
