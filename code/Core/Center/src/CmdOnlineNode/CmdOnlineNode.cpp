/*******************************************************************************
 * Project:  CenterServer
 * @file     CmdOnlineNode.cpp
 * @brief 
 * @author   chenjiayi
 * @date:    2016年8月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include <iostream>
#include "util/json/CJsonObject.hpp"
#include "step/StepNode.hpp"
#include "CmdOnlineNode.hpp"

MUDULE_CREATE(core::CmdOnlineNode);

namespace core
{

CmdOnlineNode::CmdOnlineNode():pSess(NULL),boInit(false)
{
}
CmdOnlineNode::~CmdOnlineNode()
{
}

bool CmdOnlineNode::Init()
{
    if (boInit)
    {
        return true;
    }
    pSess = GetNodeSession(true);
    if(!pSess)
    {
        LOG4_ERROR("failed to get GetNodeSession");
        return false;
    }
    boInit = true;
    return true;
}

bool CmdOnlineNode::AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
	LOG4_TRACE("%s()", __FUNCTION__);
	m_stMsgShell = stMsgShell;
    m_oInMsgHead = oInMsgHead;
    if(!pSess->IsMaster())
	{
		LOG4_DEBUG("is not master");
		return false;
	}
    if(!parseMsg(oInMsgBody))
	{
		LOG4_ERROR("parseHttpMsg failed:%d:%s", server_err_code(ERR_MSG_BODY_DECODE), server_err_msg(ERR_MSG_BODY_DECODE));
		return Response(ERR_REQ_MISS_PARAM);
	}
    //对应的节点信息.sOnlineIdentify(IP:端口)
    char sOnlineIdentify[32];
    snprintf(sOnlineIdentify,sizeof(sOnlineIdentify),"%s:%u",m_oOnlineNodeReq.inner_ip().c_str(),m_oOnlineNodeReq.inner_port());
    if(!pSess->HasIdentifyAuthority(sOnlineIdentify))
    {
        LOG4_DEBUG("HasAuthority none(%s)",sOnlineIdentify);
        return false;
    }
    if (eOnlineFlag_restore_routes == m_oOnlineNodeReq.online())//恢复节点路由:0
    {
        int nRet = pSess->OnlineNode(sOnlineIdentify);
        if(nRet)
        {
            LOG4_WARN("failed to sOnlineIdentify(%s)",sOnlineIdentify);
            return Response(nRet);
        }
        return Response(nRet);
    }
    else if(eOnlineFlag_restart_workers == m_oOnlineNodeReq.online())//重启工作者:1
    {
        int nRet = pSess->CanOnlineNode(sOnlineIdentify);//是否正常节点，是有挂起节点才能被重启工作者
        if (ERR_SERVER_SELF_ONLINE == nRet)
        {
            LOG4_INFO("center worker exit");
            Response(ERR_OK);
            exit(0);
            return true;
        }
        else if (nRet)
        {
            LOG4_WARN("can't OnlineNode sOnlineIdentify(%s)!",sOnlineIdentify);
            return Response(nRet);
        }
        //重启目标节点工作者
        if(!SendRestartWorkersToTarget(sOnlineIdentify))
        {
            LOG4_WARN("failed to SendRestartWorkersToTarget(%s)",sOnlineIdentify);
            return Response(ERR_SERVER_LOGIC_ERROR);
        }//成功发送出去后异步回应
        return true;
    }
    else
    {
        LOG4_DEBUG("CmdOnlineNode online(%d) error",m_oOnlineNodeReq.online());
        return Response(ERR_REQ_MISS_PARAM);
    }
}

//发送重启工作者通知到目标服务
bool CmdOnlineNode::SendRestartWorkersToTarget(const std::string& sOnlineIdentify)
{
    struct Param:net::StepParam
	{
    	Param(server::online_node_ack &oAck):oOnlineNodeAck(oAck){}
    	server::online_node_ack oOnlineNodeAck;
	};
    auto callback = [](const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data,net::Step*pStep)
	{
    	net::DataStep* pDataStep = (net::DataStep*)pStep;
    	Param* pParam = (Param*)pDataStep->GetData();
    	OrdinaryResponse oRes;
    	int iCode(ERR_OK);
		if (!oRes.ParseFromString(oInMsgBody.body()))
		{
			LOG4_ERROR("failed to parse OrdinaryResponse oRes!");
			iCode = ERR_SERVERINFO;
		}
		else if(oRes.err_no())
		{
			LOG4_ERROR("error %d: %s!", oRes.err_no(), oRes.err_msg().c_str());
			iCode = ERR_SERVERINFO;
		}
		pParam->oOnlineNodeAck.mutable_error()->set_error_code(server_err_code(iCode));
		pParam->oOnlineNodeAck.mutable_error()->set_error_info(server_err_msg(iCode));
		pParam->oOnlineNodeAck.mutable_error()->set_error_client_show(server_err_msg(iCode));
		pDataStep->SendToClient(pParam->oOnlineNodeAck.SerializeAsString());
	};
    MsgBody oOutMsgBody;
	return net::SendToCallback(new net::DataStep(m_stMsgShell,m_oInMsgHead,new Param(m_oOnlineNodeAck)),net::CMD_REQ_NODE_RESTART_WORKERS,oOutMsgBody.SerializeAsString(),callback,sOnlineIdentify);
}
/*
message online_node_req
{
   string inner_ip = 1;//指定修改节点ip
   uint32 inner_port = 2;//指定修改节点端口
   uint32 online= 3;//恢复节点路由:0,重启工作者:1(非中心节点只有挂起的节点才能重启工作者)
}
* */
bool CmdOnlineNode::parseMsg(const MsgBody& oInMsgBody)
{
    m_oOnlineNodeReq.Clear();
    if (!net::ParseMsgBody(oInMsgBody,m_oOnlineNodeReq))
    {
        LOG4_ERROR("%s() ParseMsgBody(oInMsgBody,m_oOnlineNodeReq) failed!",__FUNCTION__);
        return(false);
    }
    m_oOnlineNodeAck.set_inner_ip(m_oOnlineNodeReq.inner_ip());
    m_oOnlineNodeAck.set_inner_port(m_oOnlineNodeReq.inner_port());
    m_oOnlineNodeAck.set_online(m_oOnlineNodeReq.online());
    LOG4_DEBUG("%s() m_oOnlineNodeReq(%s)",m_oOnlineNodeReq.DebugString().c_str());
    return(true);
}

/*
服务器节点上线响应
message online_node_ack
{
	common.errorinfo error = 1;//错误码以及错误描述信息
	string inner_ip = 2;//指定修改节点ip
	uint32 inner_port = 3;//指定修改节点端口
	uint32 offline = 4;//恢复节点路由:0,重启工作者:1
}
 * */
bool CmdOnlineNode::Response(int iErrno)
{
    LOG4_DEBUG("CmdOfflineNode::Response iErrno(%d)",iErrno);
    m_oOnlineNodeAck.mutable_error()->set_error_code(server_err_code(iErrno));
    m_oOnlineNodeAck.mutable_error()->set_error_info(server_err_msg(iErrno));
    m_oOnlineNodeAck.mutable_error()->set_error_client_show(server_err_msg(iErrno));
    return net::SendToClient(m_stMsgShell,m_oInMsgHead, m_oOnlineNodeAck);
}


} /* namespace core */
