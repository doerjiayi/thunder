/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepGetOfflineMsg.hpp
 * @brief    主动获取离线消息
 * @author   ty
 * @date:    2019年8月13日
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_GET_P2P_OFFLINEMSG_HPP_
#define SRC_STEP_GET_P2P_OFFLINEMSG_HPP_


#include "chat_msg.pb.h"
#include "user.pb.h"
#include "common.pb.h"
//#include "msg.pb.h"
#include "user_basic.pb.h"
#include "server_internal.pb.h"
#include "ImError.h"

#include "step/Step.hpp"

namespace im
{
//拉取离线数据
class StepGetOfflineMsg : public net::Step
{
public:
    StepGetOfflineMsg(const im_user::ulogin &oInAsk,const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody);
    virtual ~StepGetOfflineMsg();

    virtual net::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    virtual net::E_CMD_STATUS Callback(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data = NULL);
    virtual net::E_CMD_STATUS Timeout();
	//个人通知离线
	void Emit_personal_notify();
	//单聊离线
	void Emit_offline_msg_p2p_req();
	//群通知离线
	void Emit_group_notify();
	//群聊离线
	void Emit_offline_msg_group_req();
	//指定公众消息离线
	void Emit_offical_specified_notify();
	//分组公众消息离线
	void Emit_offical_userset_notify();
private:
    int m_iTimeoutNum;          ///< 超时次数
	im_user::ulogin m_oInAsk;
	net::uint32 m_ulimid;
    net::tagMsgShell m_stMsgShell;
    MsgHead m_oInMsgHead;
    MsgBody m_oInMsgBody;

    MsgHead m_oOutMsgHead;
    MsgBody m_oOutMsgBody;
};

} /* namespace im */

#endif 
