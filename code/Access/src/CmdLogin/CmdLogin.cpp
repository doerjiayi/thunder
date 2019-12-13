/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdLogin.cpp
 * @brief 
 * @author   lbh
 * @date:    2019年8月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdLogin.hpp"
#include "user.pb.h"
#include "common.pb.h"
#include "ImError.h"

MUDULE_CREATE(im::CmdLogin);

namespace im
{

CmdLogin::CmdLogin()
{
}

CmdLogin::~CmdLogin()
{
}

bool CmdLogin::AnyMessage(
                const net::tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    im_user::ulogin  oInAsk;
    im_user::ulogin_ack  oOutAck;
    oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
    oOutMsgHead.set_seq(oInMsgHead.seq());
    do
    {
    	if (!oInAsk.ParseFromString(oInMsgBody.body()))
		{
			oOutAck.mutable_error()->set_error_code(im::ERR_INVALID_PROTOCOL);
			oOutAck.mutable_error()->set_error_info("invalid protocol err");
			oOutAck.mutable_error()->set_error_client_show("解析Protobuf出错");
			LOG4_ERROR("error %d: WorkerLoad ParseFromString error!", im::ERR_INVALID_PROTOCOL);
			break;
		}
		else
		{
			LOG4_DEBUG("im_user::ulogin ParseFromString [%s] oInMsgBody[%s] ok!",oInAsk.DebugString().c_str(),oInMsgBody.DebugString().c_str());
			//判断是否重复登录
			if (g_pLabor->HadClientData(stMsgShell))
			{
				oOutAck.mutable_error()->set_error_code(net::ERR_OK);
				oOutAck.mutable_error()->set_error_info("ok");
				oOutAck.mutable_error()->set_error_client_show("成功");
				break;
			}
			if (!net::ExecStep(new StepToLogic(stMsgShell, oInMsgHead, oInMsgBody)))
			{
				oOutAck.mutable_error()->set_error_code(im::ERR_SERVER_ERROR);
				oOutAck.mutable_error()->set_error_info("server err");
				oOutAck.mutable_error()->set_error_client_show("解析Protobuf出错");
				LOG4_ERROR("error: new StepToLogic() error!");
				break;
			}
			return(true);
		}
    }while(0);
    oOutMsgBody.set_body(oOutAck.SerializeAsString());
    oOutMsgBody.set_additional(oInMsgBody.additional());
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
    net::SendTo(stMsgShell, oOutMsgHead,oOutMsgBody);
    return(true);
}


} /* namespace im */
