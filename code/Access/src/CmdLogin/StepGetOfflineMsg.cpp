/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepGetOfflineMsg.cpp
 * @brief    获取离线消息请求
 * @author   ty
 * @date:    2019年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepGetOfflineMsg.hpp"
#include "user.pb.h"
#include "common.pb.h"
#include "user_basic.pb.h"
#include "util/CBuffer.hpp"

namespace im
{

StepGetOfflineMsg::StepGetOfflineMsg(const im_user::ulogin &oInAsk,const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody)
    : m_iTimeoutNum(0),m_oInAsk(oInAsk),m_ulimid(oInAsk.imid()),m_stMsgShell(stMsgShell),m_oInMsgHead(oInMsgHead),m_oInMsgBody(oInMsgBody)
{
	m_oOutMsgHead = m_oInMsgHead;
	m_oOutMsgBody = m_oInMsgBody;
}

StepGetOfflineMsg::~StepGetOfflineMsg()
{
}

net::E_CMD_STATUS StepGetOfflineMsg::Emit(int iErrno, const std::string& strErrMsg,const std::string& strErrShow)
{
	LOG4_DEBUG("%s()", __FUNCTION__);

	Emit_Begin:
	if (m_iTimeoutNum==0)
	{
		//个人通知离线
		Emit_personal_notify();
	}
	else if(m_iTimeoutNum==1)
	{
		//push_chat_msg字段为0则不推送
		if (m_oInAsk.has_push_chat_msg()&&m_oInAsk.push_chat_msg()==0)
		{
			m_iTimeoutNum++;
			goto Emit_Begin;
		}
		
		//单聊离线
		Emit_offline_msg_p2p_req();
	}
	else if(m_iTimeoutNum==2)
	{
		//群通知离线
		Emit_group_notify();
	}
	else if(m_iTimeoutNum==3)
	{
		//push_chat_msg字段为0则不推送
		if (m_oInAsk.has_push_chat_msg()&&m_oInAsk.push_chat_msg()==0)
		{
			m_iTimeoutNum++;
			goto Emit_Begin;
		}
		//群聊离线
		Emit_offline_msg_group_req();
	}
	else if(m_iTimeoutNum==4)
	{
		//指定公众消息离线
		Emit_offical_specified_notify();
	}
	else if(m_iTimeoutNum==5)
	{
		//分组公众消息离线
		Emit_offical_userset_notify();
	}
	else 
	{
		return(net::STATUS_CMD_COMPLETED);
	}
  
    if (m_oOutMsgBody.has_session_id())
    {
        net::SendToWithMod("LOGIC", m_oOutMsgBody.session_id(), m_oOutMsgHead, m_oOutMsgBody);
    }
    else if (m_oOutMsgBody.has_session())
    {
        unsigned int uiSessionFactor = 0;
        for (unsigned int i = 0; i < m_oOutMsgBody.session().size(); ++i)
        {
            uiSessionFactor += m_oOutMsgBody.session()[i];
        }
        net::SendToWithMod("LOGIC", uiSessionFactor, m_oOutMsgHead, m_oOutMsgBody);
    }
    else
    {
        net::SendToNext("LOGIC", m_oOutMsgHead, m_oOutMsgBody);
    }

    return(net::STATUS_CMD_RUNNING);
}

//个人通知离线
net::E_CMD_STATUS StepGetOfflineMsg::Emit_personal_notify()
{
	LOG4_DEBUG("%s()", __FUNCTION__);

	chat_msg::personal_notify oOutAsk;
	oOutAsk.set_imid(m_ulimid);
	m_oOutMsgBody.set_body(oOutAsk.SerializeAsString());
	m_oOutMsgHead.set_msgbody_len(m_oOutMsgBody.ByteSize());
	//设置命令字
	m_oOutMsgHead.set_cmd(64001);
	//往后面发数据需要另设Sequence
	m_oOutMsgHead.set_seq(GetSequence());
	LOG4_DEBUG("seq[%llu] offical_userset_notify oOutAsk[%s]", m_oOutMsgHead.seq(),oOutAsk.DebugString().c_str());

	return(net::STATUS_CMD_COMPLETED);
}

//单聊离线
net::E_CMD_STATUS StepGetOfflineMsg::Emit_offline_msg_p2p_req()
{
	LOG4_DEBUG("%s()", __FUNCTION__);

	chat_msg::offline_msg_p2p_req oOutAsk;
	oOutAsk.set_send_id(m_ulimid);
	oOutAsk.set_msg_count(0);
	m_oOutMsgBody.set_body(oOutAsk.SerializeAsString());
	m_oOutMsgHead.set_msgbody_len(m_oOutMsgBody.ByteSize());
	//设置命令字
	m_oOutMsgHead.set_cmd(4009);
	//往后面发数据需要另设Sequence
	m_oOutMsgHead.set_seq(GetSequence());
	LOG4_DEBUG("seq[%llu] Emit_offline_msg_p2p_req[%s]", m_oOutMsgHead.seq(),oOutAsk.DebugString().c_str());
	
	return(net::STATUS_CMD_COMPLETED);
}

//群通知离线
net::E_CMD_STATUS StepGetOfflineMsg::Emit_group_notify()
{
	LOG4_DEBUG("%s()", __FUNCTION__);

	chat_msg::group_notify oOutAsk;
	oOutAsk.set_imid(m_ulimid);
	m_oOutMsgBody.set_body(oOutAsk.SerializeAsString());
	m_oOutMsgHead.set_msgbody_len(m_oOutMsgBody.ByteSize());
	//设置命令字
	m_oOutMsgHead.set_cmd(64003);
	//往后面发数据需要另设Sequence
	m_oOutMsgHead.set_seq(GetSequence());
	LOG4_DEBUG("seq[%llu] chat_msg::group_notify oOutAsk[%s]", m_oOutMsgHead.seq(),oOutAsk.DebugString().c_str());

	return(net::STATUS_CMD_COMPLETED);
}

//群聊离线
net::E_CMD_STATUS StepGetOfflineMsg::Emit_offline_msg_group_req()
{
	LOG4_DEBUG("%s()", __FUNCTION__);

	chat_msg::offline_msg_group_req oOutAsk;
	oOutAsk.Clear();
	oOutAsk.set_group_id(0);
	oOutAsk.set_msg_count(0);
	m_oOutMsgBody.set_body(oOutAsk.SerializeAsString());
	m_oOutMsgHead.set_msgbody_len(m_oOutMsgBody.ByteSize());
	//设置命令字
	m_oOutMsgHead.set_cmd(4011);
	//往后面发数据需要另设Sequence
	m_oOutMsgHead.set_seq(GetSequence());
	LOG4_DEBUG("seq[%llu] chat_msg::offline_msg_group_req oOutAsk[%s]", m_oOutMsgHead.seq(),oOutAsk.DebugString().c_str());

	return(net::STATUS_CMD_COMPLETED);
}

//指定公众消息离线
net::E_CMD_STATUS StepGetOfflineMsg::Emit_offical_specified_notify()
{
	LOG4_DEBUG("%s()", __FUNCTION__);

	offical_specified_notify oOutAsk;
	oOutAsk.set_imid(m_ulimid);
	m_oOutMsgBody.set_body(oOutAsk.SerializeAsString());
	m_oOutMsgHead.set_msgbody_len(m_oOutMsgBody.ByteSize());
	//设置命令字
	m_oOutMsgHead.set_cmd(64007);
	//往后面发数据需要另设Sequence
	m_oOutMsgHead.set_seq(GetSequence());
	LOG4_DEBUG("seq[%llu] offical_specified_notify oOutAsk[%s]", m_oOutMsgHead.seq(),oOutAsk.DebugString().c_str());

	return(net::STATUS_CMD_COMPLETED);
}

//分组公众消息离线
net::E_CMD_STATUS StepGetOfflineMsg::Emit_offical_userset_notify()
{
	LOG4_DEBUG("%s()", __FUNCTION__);

	offical_userset_notify oOutAsk;
	oOutAsk.set_imid(m_ulimid);
	m_oOutMsgBody.set_body(oOutAsk.SerializeAsString());
	m_oOutMsgHead.set_msgbody_len(m_oOutMsgBody.ByteSize());
	//设置命令字
	m_oOutMsgHead.set_cmd(64005);
	//往后面发数据需要另设Sequence
	m_oOutMsgHead.set_seq(GetSequence());
	LOG4_DEBUG("seq[%llu] offical_userset_notify oOutAsk[%s]", m_oOutMsgHead.seq(),oOutAsk.DebugString().c_str());

	return(net::STATUS_CMD_COMPLETED);
}

net::E_CMD_STATUS StepGetOfflineMsg::Callback(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data)
{
    LOG4_DEBUG("seq[%llu] StepGetOfflineMsg::Callback ok!", oInMsgHead.seq());
	return(net::STATUS_CMD_RUNNING);
}

net::E_CMD_STATUS StepGetOfflineMsg::Timeout()
{
	m_iTimeoutNum++;
	LOG4_DEBUG("%s()", __FUNCTION__);
	return(Emit(net::ERR_OK));
}

} /* namespace im */
