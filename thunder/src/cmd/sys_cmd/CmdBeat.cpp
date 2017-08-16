/*******************************************************************************
 * Project:  Thunder
 * @file     CmdBeat.cpp
 * @brief 
 * @author   cjy
 * @date:    2017年11月5日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdBeat.hpp"

namespace thunder
{

CmdBeat::CmdBeat()
{
}

CmdBeat::~CmdBeat()
{
}

bool CmdBeat::AnyMessage(
                const MsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    bool bResult = false;
    MsgHead oOutMsgHead = oInMsgHead;
    MsgBody oOutMsgBody = oInMsgBody;
    oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
    GetLabor()->SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
    return(bResult);
}

} /* namespace thunder */
