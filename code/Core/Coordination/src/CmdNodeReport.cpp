/*******************************************************************************
 * Project:  Beacon
 * @file     CmdNodeReport.cpp
 * @brief 
 * @author   bwar
 * @date:    Feb 14, 2017
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdNodeReport.hpp"

namespace coor
{

CmdNodeReport::CmdNodeReport(int32 iCmd)
    :    m_pSessionOnlineNodes(nullptr)
{
}

CmdNodeReport::~CmdNodeReport()
{
}

bool CmdNodeReport::Init()
{
    return(true);
}

bool CmdNodeReport::AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oMsgHead,const MsgBody& oMsgBody)
{
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    util::CJsonObject oNodeInfo;
    if (nullptr == m_pSessionOnlineNodes)
    {
        m_pSessionOnlineNodes = std::dynamic_pointer_cast<SessionOnlineNodes>(GetSession("coor::SessionOnlineNodes"));
        if (nullptr == m_pSessionOnlineNodes)
        {
            LOG4_ERROR("no session node found!");
            return(false);
        }
    }
    if (oNodeInfo.Parse(oMsgBody.data()))
    {
        uint16 unNodeId = m_pSessionOnlineNodes->AddNode(oNodeInfo);
        if (0 == unNodeId)
        {
            oOutMsgBody.mutable_rsp_result()->set_code(net::ERR_NODE_NUM);
            oOutMsgBody.mutable_rsp_result()->set_msg("there is no valid node_id in the system!");
        }
        else
        {
            util::CJsonObject oNodeId;
            oNodeId.Add("node_id", unNodeId);
            oOutMsgBody.set_data(oNodeId.ToString());
            oOutMsgBody.mutable_rsp_result()->set_code(net::ERR_OK);
            oOutMsgBody.mutable_rsp_result()->set_msg("OK");
        }
    }
    else
    {
        LOG4_ERROR("failed to parse node info json from MsgBody.data()!");
        oOutMsgBody.mutable_rsp_result()->set_code(net::ERR_BODY_JSON);
        oOutMsgBody.mutable_rsp_result()->set_msg("failed to parse node info json from MsgBody.data()!");
    }
    return(SendTo(stMsgShell, oMsgHead.cmd() + 1, oMsgHead.seq(), oOutMsgBody));
}

} /* namespace coor */
