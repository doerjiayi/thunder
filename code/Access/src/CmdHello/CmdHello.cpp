/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdHello.cpp
 * @brief 
 * @author   lbh
 * @date:    2019年8月24日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdHello.hpp"

MUDULE_CREATE(im::CmdHello);

namespace im
{

bool CmdHello::AnyMessage(const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody)
{
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
    oOutMsgHead.set_seq(oInMsgHead.seq());

	LOG4_DEBUG("CmdHello::AnyMessage:%s", oOutMsgBody.body().c_str());
	oOutMsgBody.set_body(oInMsgBody.body());
	oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
	GetLabor()->SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
    return(true);
}

} /* namespace im */
