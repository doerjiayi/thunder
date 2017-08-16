/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdUpdateNodeId.cpp
 * @brief 
 * @author   cjy
 * @date:    2017年9月18日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdUpdateNodeId.hpp"

namespace thunder
{

CmdUpdateNodeId::CmdUpdateNodeId()
{
}

CmdUpdateNodeId::~CmdUpdateNodeId()
{
}

bool CmdUpdateNodeId::AnyMessage(
                const MsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    thunder::CJsonObject oNode;
    if (oNode.Parse(oInMsgBody.body()))
    {
        int iNodeId = 0;
        oNode.Get("node_id", iNodeId);
        GetLabor()->SetNodeId(iNodeId);
        return(true);
    }
    return(false);
}

} /* namespace thunder */
