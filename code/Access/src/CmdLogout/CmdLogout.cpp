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
#include "RobotError.h"

MUDULE_CREATE(im::CmdLogout);

namespace im
{

bool CmdLogout::AnyMessage(
                const net::tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    bool bResult = false;
    util::CBuffer oBuff;
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    im_user::ulogout  oInAsk;
    im_user::ulogout_ack  oOutAck;
    bool errReturn =false;
    oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
    oOutMsgHead.set_seq(oInMsgHead.seq());

	if (oInAsk.ParseFromString(oInMsgBody.body()))
	{
		bResult = true;
		LOG4_DEBUG("im_user::ulogout ParseFromString uid[%d] ok!",oInAsk.imid());

		user_basic basicInfo;
		if (oInMsgBody.has_additional() && basicInfo.ParseFromString(oInMsgBody.additional()))
		{
			if (basicInfo.userid()!=oInAsk.imid())
			{
				errReturn = true;
			}
		}
		else
		{
			errReturn = true;
		}
		//直接返回，数据有问题不转发Logic
		if (errReturn)
		{
			oOutAck.mutable_error()->set_error_code(net::ERR_OK);
			oOutAck.mutable_error()->set_error_info("OK");
			oOutAck.mutable_error()->set_error_client_show("OK");
			oOutMsgBody.set_body(oOutAck.SerializeAsString());
			oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
			net::SendTo(stMsgShell, oOutMsgHead,oOutMsgBody);
			g_pLabor->Disconnect(stMsgShell,false);
			return(bResult);
		}
		oOutAck.mutable_error()->set_error_code(net::ERR_OK);
		oOutAck.mutable_error()->set_error_info("OK");
		oOutAck.mutable_error()->set_error_client_show("OK");
		oOutMsgBody.set_body(oOutAck.SerializeAsString());
		oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
		net::SendTo(stMsgShell, oOutMsgHead,oOutMsgBody);//直接返回

		net::ExecStep(new StepLogoutToLogic(stMsgShell, oInMsgHead, oInMsgBody,oInAsk));
		return(bResult);
	}
	else
	{
		bResult = false;
		oOutAck.mutable_error()->set_error_code(im::ERR_INVALID_PROTOCOL);
		oOutAck.mutable_error()->set_error_info("invalid protocol err");
		oOutAck.mutable_error()->set_error_client_show("解析Protobuf出错");
		LOG4_ERROR("error %d: StepLogoutToLogic ParseFromString error!", ERR_INVALID_PROTOCOL);
	}

    oOutMsgBody.set_body(oOutAck.SerializeAsString());
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
    net::SendTo(stMsgShell, oOutMsgHead,oOutMsgBody);
    return(bResult);
}

} /* namespace im */
