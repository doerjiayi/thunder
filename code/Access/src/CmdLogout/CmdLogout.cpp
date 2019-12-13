/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdLogout.cpp
 * @brief 
 * @author   lbh
 * @date:    2019年8月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdLogout.hpp"
#include "StepLogoutToLogic.hpp"
#include "user.pb.h"
#include "common.pb.h"
#include "user_basic.pb.h"
#include "ImError.h"

MUDULE_CREATE(im::CmdLogout);

namespace im
{

bool CmdLogout::AnyMessage(
                const net::tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    util::CBuffer oBuff;
    im_user::ulogout  oInAsk;
    im_user::ulogout_ack  oOutAck;
	if (oInAsk.ParseFromString(oInMsgBody.body()))
	{
		LOG4_DEBUG("im_user::ulogout ParseFromString uid[%d] ok!",oInAsk.imid());
		user_basic basicInfo;
		if (oInMsgBody.has_additional() && basicInfo.ParseFromString(oInMsgBody.additional()))
		{
			if (basicInfo.userid() == oInAsk.imid())
			{
				oOutAck.mutable_error()->set_error_code(net::ERR_OK);
				oOutAck.mutable_error()->set_error_info("OK");
				oOutAck.mutable_error()->set_error_client_show("OK");
				net::SendToClient(stMsgShell, oInMsgHead,oOutAck);
				net::ExecStep(new StepLogoutToLogic(stMsgShell, oInMsgHead, oInMsgBody,oInAsk));
				return(true);
			}
		}
	}
	oOutAck.mutable_error()->set_error_code(im::ERR_INVALID_PROTOCOL);
	oOutAck.mutable_error()->set_error_info("invalid protocol err");
	oOutAck.mutable_error()->set_error_client_show("解析Protobuf出错");
	LOG4_ERROR("error %d: StepLogoutToLogic ParseFromString error!", ERR_INVALID_PROTOCOL);
	g_pLabor->Disconnect(stMsgShell,false);
	net::SendToClient(stMsgShell, oInMsgHead,oOutAck);
	return(false);
}

} /* namespace im */
