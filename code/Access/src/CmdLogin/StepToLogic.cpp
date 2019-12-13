/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepToLogic.cpp
 * @brief    把数据发到逻辑服务器
 * @author   lbh
 * @date:    2019年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepToLogic.hpp"
#include "user.pb.h"
#include "common.pb.h"
#include "user_basic.pb.h"
#include "util/CBuffer.hpp"

namespace im
{

StepToLogic::StepToLogic(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody)
    : m_iTimeoutNum(0),m_stMsgShell(stMsgShell),m_oInMsgHead(oInMsgHead),m_oInMsgBody(oInMsgBody)
{
	m_bLoginOk = false;
}

StepToLogic::~StepToLogic()
{
}

net::E_CMD_STATUS StepToLogic::Emit(int iErrno, const std::string& strErrMsg,const std::string& strErrShow)
{
    m_oOutMsgHead = m_oInMsgHead;
    m_oOutMsgBody = m_oInMsgBody;
    if (m_oInAsk.ParseFromString(m_oInMsgBody.body()))
    {
        std::string identify = net::GetWorkerIdentify();
        m_oInAsk.set_serv_nodeidentify(identify);
        m_oOutMsgBody.set_body(m_oInAsk.SerializeAsString());
        m_oOutMsgHead.set_msgbody_len(m_oOutMsgBody.ByteSize());
    }
    else
    {
       LOG4_ERROR("seq[%llu] oInAsk.ParseFromString() fail !", m_oInMsgHead.seq());
       return(net::STATUS_CMD_FAULT);
    }

	net::tagMsgShell tstMsgShell;
	char szIdentify[32] = {0};
	snprintf(szIdentify, sizeof(szIdentify), "%u", m_oInAsk.imid());
	g_pLabor->GetMsgShell(szIdentify,tstMsgShell);
	//已经登录过
	if (m_stMsgShell.iFd == tstMsgShell.iFd && m_stMsgShell.ulSeq == tstMsgShell.ulSeq)
	{
		MsgHead oOutMsgHead;
		MsgBody oOutMsgBody;
		im_user::ulogin_ack  oOutAck;
		oOutMsgHead.set_cmd(m_oInMsgHead.cmd() + 1);
		oOutMsgHead.set_seq(m_oInMsgHead.seq());
	    oOutMsgBody.set_session_id(m_oInAsk.imid());
		common::errorinfo *pInfo = oOutAck.mutable_error();
		pInfo->set_error_code(net::ERR_OK);
		pInfo->set_error_info("OK");
		pInfo->set_error_client_show("OK");
		oOutMsgBody.set_body(oOutAck.SerializeAsString());
		oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
		SendTo(m_stMsgShell,oOutMsgHead,oOutMsgBody);

		LOG4_ERROR("imid[%u] have login in !",m_oInAsk.imid());
		return(net::STATUS_CMD_FAULT);
	}
    m_oOutMsgHead.set_seq(GetSequence());//往后面发数据需要另设Sequence
    LOG4_DEBUG("seq[%llu] StepToLogic::Emit!", m_oInMsgHead.seq());
    net::SendToSession("LOGIC",m_oOutMsgHead, m_oOutMsgBody);
    return(net::STATUS_CMD_RUNNING);
}

net::E_CMD_STATUS StepToLogic::Callback(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data)
{
    LOG4_DEBUG("seq[%llu] StepToLogic::Callback ok!", oInMsgHead.seq());
    util::CBuffer oBuff;
    MsgHead oOutMsgHead = oInMsgHead;
    oOutMsgHead.set_seq(m_oInMsgHead.seq());
    im_user::ulogin_ack  oOutAck;
    if (!oOutAck.ParseFromString(oInMsgBody.body()))//解析数据包
    {
        LOG4_ERROR("cmd[%llu]   im_user::ulogin_ack ParseFromString error", oInMsgHead.cmd());
        return net::STATUS_CMD_COMPLETED;
    }
	LOG4_DEBUG("seq[%llu] StepToLogic::Callback imid[%u] oOutAck[%s]!", oInMsgHead.seq(),m_oInAsk.imid(),oOutAck.DebugString().c_str());
    net::SendTo(m_stMsgShell,oOutMsgHead,oInMsgBody);//先
    if (oOutAck.error().error_code()==0)//登录成功
    {
        char strUid[50] = {0};
        snprintf(strUid,sizeof(strUid),"%u",oInMsgBody.session_id());
        net::AddMsgShell(strUid,m_stMsgShell);//对外的被动链接记录
        user_basic basicinfo;//角色基础数据包
        if (!basicinfo.ParseFromString(oInMsgBody.additional()))
        {
            LOG4_ERROR("cmd[%llu]  user_basic ParseFromString error", oInMsgHead.cmd());
            return net::STATUS_CMD_COMPLETED;
        }
        //填写客户端IP
        basicinfo.set_login_ip(g_pLabor->GetClientAddr(m_stMsgShell));
        util::CBuffer oBuff;
        oBuff.Write(basicinfo.SerializeAsString().c_str(),basicinfo.ByteSize());
        //设置客户端数据
        g_pLabor->SetClientData(m_stMsgShell,&oBuff);
		//添加新增数据
		m_oInMsgBody.set_additional(basicinfo.SerializeAsString());
		m_bLoginOk= true;
		Timeout();//超时才会调用
	    return(net::STATUS_CMD_COMPLETED);
    }
    else //如果验证失败，断掉连接
    {
		g_pLabor->Disconnect(m_stMsgShell,false);
        LOG4_ERROR("cmd[%u]  Login  error Disconnect oOutAck[%s]", oInMsgHead.cmd(),oOutAck.DebugString().c_str());
    }
    return(net::STATUS_CMD_COMPLETED);
}

net::E_CMD_STATUS StepToLogic::Timeout()
{
	if (m_bLoginOk)
	{
		net::ExecStep(new StepGetOfflineMsg(m_oInAsk,m_stMsgShell, m_oInMsgHead, m_oInMsgBody));//S2S离线推送协议
        return(net::STATUS_CMD_COMPLETED);
	}
    ++m_iTimeoutNum;
    if (m_iTimeoutNum <= 3)//超时重发
    {
        m_oOutMsgHead.set_seq(GetSequence());//往后面发数据需要另设Sequence
        LOG4_DEBUG("seq[%llu]  StepToLogic::Timeout Send to!", m_oOutMsgHead.seq());
        net::SendToSession("LOGIC",m_oOutMsgHead, m_oOutMsgBody);
        return(net::STATUS_CMD_RUNNING);
    }
    else
    {
        LOG4_ERROR("error timeout %d times!", m_iTimeoutNum);
        return(net::STATUS_CMD_FAULT);
    }
}

} /* namespace im */
