/*******************************************************************************
 * Project:  CenterServer
 * @file     CmdOfflineNode.cpp
 * @brief 
 * @author   chenjiayi
 * @date:    2016年8月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include <iostream>
#include "util/json/CJsonObject.hpp"
#include "CmdOfflineNode.hpp"

MUDULE_CREATE(core::CmdOfflineNode);

namespace core
{

CmdOfflineNode::CmdOfflineNode()
                :pSess(NULL),boInit(false)
{
}
CmdOfflineNode::~CmdOfflineNode()
{
}

bool CmdOfflineNode::Init()
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

bool CmdOfflineNode::AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    m_stMsgShell = stMsgShell;
    m_oInMsgHead = oInMsgHead;
    LOG4_TRACE("%s()", __FUNCTION__);
    if(!pSess->IsMaster())
	{
		LOG4_DEBUG("is not master");
		return false;
	}
    if(!parseMsg(oInMsgBody))
    {
        LOG4_ERROR("parseHttpMsg failed");
		Response(ERR_REQ_MISS_PARAM);
        return(false);
    }
    char sOfflineIdentify[32];
    snprintf(sOfflineIdentify,sizeof(sOfflineIdentify),"%s:%u",m_oOfflineNodeReq.inner_ip().c_str(),m_oOfflineNodeReq.inner_port());
    if(!pSess->HasIdentifyAuthority(sOfflineIdentify))
    {
        LOG4_DEBUG("HasAuthority none(%s)",sOfflineIdentify);
        return false;
    }
    if(eofflineFlag_close_note != m_oOfflineNodeReq.offline() && eofflineFlag_suspend_routes != m_oOfflineNodeReq.offline())
    {
        LOG4_WARN("%s() invalid offline(%d)",__FUNCTION__,m_oOfflineNodeReq.offline());
        return Response(ERR_REQ_MISS_PARAM);
    }
    int nRet = pSess->OfflineNode(sOfflineIdentify);
    if(ERR_SERVER_SELF_OFFLINE == nRet)//下线的是自己
    {
        if(eofflineFlag_close_note == m_oOfflineNodeReq.offline())//关闭节点
        {
            LOG4_WARN("can't shutdown center using web command");
            return Response(ERR_SERVER_CENTER_RESTART_SCRIPT);
        }
        else if (eofflineFlag_suspend_routes == m_oOfflineNodeReq.offline())//挂起路由
        {
            LOG4_WARN("can't suspend center");
            return Response(ERR_SERVER_CENTER_NO_SUSPEND);
        }
    }
    else if(nRet && (ERR_SERVER_NODE_ALREADY_OFFLINE != nRet))//被挂起的依然可以被关闭
    {
        LOG4_WARN("failed to OfflineNode(%s)",sOfflineIdentify);
        return Response(nRet);
    }
    if(eofflineFlag_close_note == m_oOfflineNodeReq.offline())//关闭节点
    {
        if(!SendOfflineToTarget(sOfflineIdentify))
        {
            LOG4_WARN("failed to SendOfflineToTarget(%s)",sOfflineIdentify);
            return Response(nRet);
        }//成功发送出去后异步回应
        return true;
    }
    return Response(nRet);
}


//发送下线通知到目标服务

bool CmdOfflineNode::SendOfflineToTarget(const std::string& sOfflineIdentify)
{
    struct Param:net::StepParam
	{
    	Param(server::offline_node_ack &oAck):oOfflineNodeAck(oAck){}
    	server::offline_node_ack oOfflineNodeAck;
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
		pParam->oOfflineNodeAck.mutable_error()->set_error_code(server_err_code(iCode));
		pParam->oOfflineNodeAck.mutable_error()->set_error_info(server_err_msg(iCode));
		pParam->oOfflineNodeAck.mutable_error()->set_error_client_show(server_err_msg(iCode));
		pDataStep->SendToClient(pParam->oOfflineNodeAck.SerializeAsString());
	};
    MsgBody oOutMsgBody;
	return net::SendToCallback(new net::DataStep(m_stMsgShell,m_oInMsgHead,new Param(m_oOfflineNodeAck)),net::CMD_REQ_NODE_STOP,oOutMsgBody.SerializeAsString(),callback,sOfflineIdentify);
}

bool CmdOfflineNode::parseMsg(const MsgBody& oInMsgBody)
{
    /*
    message offline_node_req
    {
        string inner_ip = 1;//指定修改节点ip
        uint32 inner_port = 2;//指定修改节点端口
        uint32 offline = 3;//挂起节点路由:0，关闭节点:1
    }
     * */
    m_oOfflineNodeReq.Clear();
    if (!net::ParseMsgBody(oInMsgBody,m_oOfflineNodeReq))
    {
        LOG4_ERROR("%s() ParseMsgBody(oInMsgBody,m_oOfflineNodeReq) failed!",__FUNCTION__);
        return(false);
    }
    m_oOfflineNodeAck.set_inner_ip(m_oOfflineNodeReq.inner_ip());
    m_oOfflineNodeAck.set_inner_port(m_oOfflineNodeReq.inner_port());
    m_oOfflineNodeAck.set_offline(m_oOfflineNodeReq.offline());
    LOG4_DEBUG("%s() m_oOfflineNodeReq(%s)",m_oOfflineNodeReq.DebugString().c_str());
    return(true);
}

bool CmdOfflineNode::Response(int iErrno)
{
    LOG4_DEBUG("CmdOfflineNode::Response iErrno(%d)",iErrno);
    m_oOfflineNodeAck.mutable_error()->set_error_code(server_err_code(iErrno));
    m_oOfflineNodeAck.mutable_error()->set_error_info(server_err_msg(iErrno));
    m_oOfflineNodeAck.mutable_error()->set_error_client_show(server_err_msg(iErrno));
    return net::SendToClient(m_stMsgShell, m_oInMsgHead,m_oOfflineNodeAck);
}


} /* namespace core */
