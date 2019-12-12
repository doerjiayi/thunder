/*******************************************************************************
 * Project:  Net
 * @file     CmdUpdateNodeId.cpp
 * @brief 
 * @author   cjy
 * @date:    2019年9月18日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdUpdateNodeId.hpp"

namespace net
{

CmdUpdateNodeId::CmdUpdateNodeId()
{
}

CmdUpdateNodeId::~CmdUpdateNodeId()
{
}

bool CmdUpdateNodeId::AnyMessage(
                const tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    util::CJsonObject oNode;
    if (oNode.Parse(oInMsgBody.body()))
    {
        int iNodeId = 0;
        oNode.Get("node_id", iNodeId);
        g_pLabor->SetNodeId(iNodeId);
        return(true);
    }
    return(false);
}

} /* namespace net */
