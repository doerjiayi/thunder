/*
 * NodeSession.cpp
 *
 *  Created on: 2015年11月6日
 *      Author: chen
 */
#include "cmd/CW.hpp"
#include "Comm.hpp"
#include "NodeSession.h"

namespace core
{

auto defaultCallback = [](const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data,net::Session*pSession)
{
	LOG4_DEBUG("send succ oInMsgBody:%s!", oInMsgBody.DebugString().c_str());
};

net::E_CMD_STATUS NodeSession::Timeout()
{//定时检查中心活跃状态
	CheckCenterActive();
	return net::STATUS_CMD_RUNNING;
}

bool NodeSession::ReadConfig()
{
	if (!net::GetConfig(m_oCurrentConf,net::GetConfigPath() + std::string("CenterCmd.json")))
	{
		return false;
	}
    return true;
}

bool NodeSession::Init(std::string &err,bool boReload)
{
    if(!boReload)
    {
        if (boInit)
        {
            return true;
        }
    }
    SetCurrentTime();
    if(m_uiInitSessionTime + m_nNodeTimeBeat >= m_uiCurrentTime)
    {
        LOG4_INFO("m_uiInitSessionTime(%llu),NodeTimeBeat(%d),currentTime(%llu),Init too often",m_uiInitSessionTime,m_nNodeTimeBeat,m_uiCurrentTime);
        return true;
    }
    m_uiInitSessionTime = m_uiCurrentTime;
    if(!ReadConfig())
    {
        LOG4_ERROR("Read conf error");
        err = "Read conf error";
        return false;
    }
    const util::CJsonObject& conf = m_oCurrentConf;
    {//gate
		int iGatewaySize(0);
		util::CJsonObject gatewayArray;
		if(conf.Get("gate", gatewayArray))
		{
			if (gatewayArray.IsArray())
			{
				iGatewaySize = gatewayArray.GetArraySize();
				for(int i = 0;i < iGatewaySize;++i)
				{
					std::string gatewayType = gatewayArray[i].ToString();
					RemoveFlag(gatewayType);
					LOG4_DEBUG("gatewayType (%s)",gatewayType.c_str());
					m_GatewayTypeList.push_back(gatewayType);
				}
			}
		}
		LOG4_INFO("gatewayArray size:%d",iGatewaySize);
	}
	{//center
		m_vecCenterServer.clear();
		util::CJsonObject center_servers;
		LOAD_CONFIG(conf,"center", center_servers);
		if(center_servers.IsEmpty())
		{
			err = "config_servers is empty";
			LOG4_ERROR("config_servers is empty");
			return false;
		}
		if (!center_servers.IsArray())
		{
			err = "center_servers is not array";
			LOG4_ERROR("center_servers is not array");
			return false;
		}
		std::string center_inner_host;
		int center_inner_port(0);
		int s = center_servers.GetArraySize();
		for(int i = 0;i < s;++i)
		{
			CenterServer centerServer;
			util::CJsonObject serverObj = center_servers[i];
			LOAD_CONFIG(serverObj,"center_inner_host",center_inner_host);
			LOAD_CONFIG(serverObj,"center_inner_port",center_inner_port);

			centerServer.center_inner_host = center_inner_host;
			centerServer.center_inner_port = center_inner_port;
			char identify[64];
			snprintf(identify,sizeof(identify),"%s:%d",center_inner_host.c_str(),center_inner_port);
			centerServer.server_identify = identify;
			LOG4_DEBUG("load center_inner_host(%s),center_inner_port(%d),server_identify(%s)",
							center_inner_host.c_str(),center_inner_port,centerServer.server_identify.c_str());
			m_vecCenterServer.push_back(centerServer);
		}
	}
	//Route配置
	LOAD_CONFIG(conf,"route", m_objRoute);
	{//中心服务器本身配置
		LOG4_INFO("gc_iBeatInterval:%d,NODE_BEAT:%f",net::gc_iBeatInterval,NODE_BEAT);
		m_centerInnerPort = g_pLabor->GetPortForServer();
		m_centerInnerHost = g_pLabor->GetHostForServer();
		m_centerNodeType = g_pLabor->GetNodeType();
		m_centerProcessNum = 1;//中心服务器工作进程数只有一个

		snprintf(m_CenterActive.inner_ip,sizeof(m_CenterActive.inner_ip),"%s",m_centerInnerHost.c_str());
		m_CenterActive.inner_port = m_centerInnerPort;
		m_CenterActive.status = eOfflineStatus;
		if(!LoadNodeRoute())
		{
			LOG4_ERROR("failed to LoadNodeRoute");
			return false;
		}
		LOG4_INFO("center InnerPort:%d InnerHost:%s NodeType:%s ProcessNum:%d",
				m_centerInnerPort,m_centerInnerHost.c_str(),m_centerNodeType.c_str(),m_centerProcessNum);
	}
	{
		util::CJsonObject redlock;
		LOAD_CONFIG(conf,"redlock", redlock);
		m_RedLock.Load(redlock);
	}
    CheckCenterActive();
    boInit = true;
    return true;
}

bool NodeSession::CheckCenterActive()
{
	m_CenterActive.status = eMasterStatus;//本节点默认为主节点
	if (m_RedLock.ServerSize())
	{
		if (m_RedLock.ContinueLock())
		{
			m_CenterActive.status = eMasterStatus;

		}
		else
		{
//			for(auto s:m_RedLock.Servers())
//			{
//				LOG4_INFO("server error(%d,%s)",s->err,s->errstr);
//			}
			m_CenterActive.status = eSlaveStatus;
		}
	}
	else
	{
		LOG4_INFO("RedLock.ServerSize zero");
	}
	LOG4_INFO("Center is %s",m_CenterActive.status == eMasterStatus ? "eMasterStatus":"eSlaveStatus");
	return true;
}

bool NodeSession::LoadNodeRoute()
{
	if (!m_objRoute.IsEmpty())
	{
		if (m_vecWhiteNode.size() == 0)
		{
			LOG4_INFO("route:%s",m_objRoute.ToString().c_str());
			//"ipwhite"
			util::CJsonObject objIpwhite;
			if (m_objRoute.Get("ipwhite",objIpwhite))
			{
				WhiteNode whiteNode;
				int s = objIpwhite.GetArraySize();
				for(int i = 0;i < s;++i)
				{
					snprintf(whiteNode.inner_ip,sizeof(whiteNode.inner_ip),RemoveFlagString(objIpwhite[i].ToString()).c_str());
					LOG4_INFO("whiteNode:%s",whiteNode.inner_ip);
					m_vecWhiteNode.push_back(whiteNode);
				}
			}
		}
		if (m_vecNodeTypes.size() == 0)
		{
			util::CJsonObject objAutoNodetype;
			if (m_objRoute.Get("auto",objAutoNodetype))//"auto" ,优先自动路由
			{
				std::vector<std::string> vecNode;
				int s = objAutoNodetype.GetArraySize();
				for(int i = 0;i < s;++i)
				{
					vecNode.push_back(RemoveFlagString(objAutoNodetype[i].ToString()));
				}
				for(auto node:vecNode)
				{
					NodeType nodeType;
					nodeType.nodetype = node;
					for(auto n:vecNode)
					{
						nodeType.neededServers.push_back(n);
					}
					LOG4_INFO("nodeType:%s",nodeType.nodetype.c_str());
					for(auto neededServer:nodeType.neededServers)LOG4_INFO("neededServer:%s",neededServer.c_str());
					m_vecNodeTypes.push_back(nodeType);
				}
			}
			else//"node"
			{
				util::CJsonObject objNodetype;
				if (m_objRoute.Get("node",objNodetype))
				{
					std::vector<std::string> vecNode;
					objNodetype.GetKeys(vecNode);
					for(auto node:vecNode)
					{
						util::CJsonObject neededservers;
						NodeType nodeType;
						nodeType.nodetype = node;
						objNodetype.Get(node,neededservers);
						int s = neededservers.GetArraySize();
						for(int i = 0;i < s;++i)
						{
							nodeType.neededServers.push_back(RemoveFlagString(neededservers[i].ToString()));
						}
						LOG4_INFO("nodeType:%s",nodeType.nodetype.c_str());
						for(auto neededServer:nodeType.neededServers)LOG4_INFO("neededServer:%s",neededServer.c_str());
						m_vecNodeTypes.push_back(nodeType);
					}
				}
			}
		}
	}
    return true;
}

bool NodeSession::CheckNodeType(const std::string& nodeType)
{//检查节点类型
    NodeTypesVec::const_iterator it = m_vecNodeTypes.begin();
    NodeTypesVec::const_iterator itEnd = m_vecNodeTypes.end();
    for (;it != itEnd; ++it)
    {
        if (it->nodetype == nodeType)
        {
            return true;
        }
    }
    LOG4_ERROR("nodeinfo type(%s) invalid",nodeType.c_str());
    return false;
}

bool NodeSession::CheckWhiteNode(const std::string& nodeInnerIp)
{
    for(auto it:m_vecWhiteNode)
    {
        if(it.inner_ip == nodeInnerIp)
        {
            return true;
        }
    }
    LOG4_ERROR("nodeinfo inner_ip(%s) invalid",nodeInnerIp.c_str());
    return false;
}

bool NodeSession::CheckNodeSuspend(NodeStatusInfo& nodeinfo)
{
	NodeStatusInfo *pNodeStatusInfo = GetNodeInfo(nodeinfo.getNodeKey());
	if (pNodeStatusInfo)
	{
		nodeinfo.suspend = pNodeStatusInfo->suspend;
	}
    return (true);
}

//检查节点状态
bool NodeSession::CheckNodeStatus(const NodeStatusInfo& nodeinfo)
{
    if(!LoadNodeRoute())
    {
        LOG4_ERROR("failed to LoadNodeRoute");
    }
    //检查节点类型
    if(!CheckNodeType(nodeinfo.nodeType))
    {
        LOG4_ERROR("nodeinfo type(%s) invalid",nodeinfo.nodeType.c_str());
        return false;
    }
    //检查白名单
    if(!CheckWhiteNode(nodeinfo.nodeInnerIp))
    {
        LOG4_ERROR("nodeinfo inner_ip(%s) invalid",nodeinfo.nodeInnerIp.c_str());
        return false;
    }
    return true;
}

net::uint32 NodeSession::GetNewNodeID()
{
    uint32 tmpID(0);
    for (const auto& it:m_mapNodesStatus)
    {
        if (it.second.nodeId > (int) tmpID)
        {
            tmpID = it.second.nodeId;
        }
    }
    ++tmpID;
    m_uiNodeId = tmpID;
    return m_uiNodeId;
}

void NodeSession::AddNodeInfo(const std::string& NodeKey,const NodeStatusInfo& Info)
{
    auto iter = m_mapNodesStatus.find(NodeKey);
    if (iter == m_mapNodesStatus.end())
    {
        m_mapNodesStatus.insert(make_pair(NodeKey, Info));
    }
}

bool NodeSession::DelNodeInfo(const std::string& strNodeKey)
{
	int n = m_mapNodesStatus.erase(strNodeKey);
    return (n > 0);
}

bool NodeSession::GetNodeInfo(const std::string &NodeKey, NodeStatusInfo &oInfo)
{
    auto iter = m_mapNodesStatus.find(NodeKey);
    if (iter != m_mapNodesStatus.end())
    {
        oInfo = iter->second;
        return true;
    }
    return false;
}
NodeStatusInfo *NodeSession::GetNodeInfo(const std::string &NodeKey)
{
    auto iter = m_mapNodesStatus.find(NodeKey);
    if (iter != m_mapNodesStatus.end())
    {
        return &iter->second;
    }
    return NULL;
}

uint32 NodeSession::GetNodeCountByType(const std::string &nodeType)
{
    uint32 count(0);
    for(const auto& iter:m_mapNodesStatus)
    {
        if(iter.second.nodeType == nodeType)
        {
            ++count;
        }
    }
    return count;
}


const NodeType* NodeSession::GetNodeTypeServerInfo(const std::string &nodeType)
{
    for (const auto& it:m_vecNodeTypes)
    {
        if (nodeType == it.nodetype)return (&it);
    }
    return (NULL);
}

bool NodeSession::GetNodeStatusByNodeType(const std::string & nodetype,std::vector<NodeLoadStatus>& vecNodeStatus)
{
    vecNodeStatus.clear();
    NodeLoadStatus nodeStatus;
    std::map<std::string, NodeStatusInfo>::iterator iter = m_mapNodesStatus.begin();
    for (; iter != m_mapNodesStatus.end(); ++iter)
    {
        if (nodetype == iter->second.nodeType)
        {
            nodeStatus.clear();
            nodeStatus = iter->second;
            vecNodeStatus.push_back(nodeStatus);
        }
    }
    if (vecNodeStatus.empty())
    {
        return (false);
    }
    return (true);
}

int NodeSession::RegNode(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, const NodeStatusInfo& nodeinfo)
{
    NodeStatusInfo tmpRegNodeStatus = nodeinfo;
    CheckNodeSuspend(tmpRegNodeStatus);
    if(eMasterStatus == m_CenterActive.status)
    {
        LOG4_DEBUG("Master(%s,%d) RegNodeRoute(%s)",m_CenterActive.inner_ip,m_CenterActive.inner_port,
                        tmpRegNodeStatus.getNodeKey().c_str());
        return RegNodeRoute(stMsgShell,oInMsgHead,oInMsgBody,tmpRegNodeStatus);
    }
    else if (eSlaveStatus == m_CenterActive.status)
    {//从节点更新内存，不更新db
        NodeStatusInfo *pNodeInfo = GetNodeInfo(tmpRegNodeStatus.getNodeKey());
        if (pNodeInfo) //注册过的只需要更新节点信息
        {
            if (!pNodeInfo->update(tmpRegNodeStatus))
            {
                LOG4_WARN("pNodeInfo(%d) update failed", tmpRegNodeStatus.nodeId);
            }
        }
        else
        {
            //加入到节点管理MAP
            LOG4_DEBUG("(%s):before AddNodeInfo(%s) size(%u)",__FUNCTION__,tmpRegNodeStatus.getNodeKey().c_str(),GetMapNodeInfoSize());
            AddNodeInfo(tmpRegNodeStatus.getNodeKey(), tmpRegNodeStatus);
            LOG4_DEBUG("(%s):after AddNodeInfo(%s) size(%u)",__FUNCTION__,tmpRegNodeStatus.getNodeKey().c_str(),GetMapNodeInfoSize());
        }
        //从中心节点只会发布自己的路由
        return SendCenterToReg(stMsgShell,oInMsgHead,oInMsgBody,tmpRegNodeStatus);
    }
    else
    {
        LOG4_WARN("m_CenterStatus(%d),wait for center",m_CenterActive.status);
        return ERR_OK;//不返回错误消息
    }
}

int NodeSession::UpdateNode(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,const NodeStatusInfo& nodeinfo)
{
    NodeStatusInfo *pNodeInfo = GetNodeInfo(nodeinfo.getNodeKey());
    if (!pNodeInfo) //没有该节点则注册
    {
        int iRet = RegNode(stMsgShell, oInMsgHead,oInMsgBody, nodeinfo);
        if (iRet)
        {
            LOG4_ERROR("CmdNodeReport RegNode msg jsonbuf[%s] is wrong,error code(%d)",
                            oInMsgBody.body().c_str(), iRet);
            return iRet;
        }
        return ERR_OK;
    }
    else //更新上报状态
    {
        if (!pNodeInfo->update(nodeinfo))
        {
            LOG4_WARN("pNodeInfo(%s,%d,%s,%d) update failed",pNodeInfo->nodeType.c_str(),pNodeInfo->nodeId,pNodeInfo->nodeInnerIp.c_str(),pNodeInfo->nodeInnerPort);
            return ERR_SERVERINFO;
        }
        return ERR_OK;
    }
}

bool NodeSession::DelNode(const std::string& delNodeIdentify)
{
	NodeStatusInfo delNodeInfo;//节点信息
	//获取对应的节点信息.delNodeIdentify(IP:端口)
	if (!GetNodeInfo(delNodeIdentify, delNodeInfo))
	{
		LOG4_WARN("No such node.del node identity(%s)!",delNodeIdentify.c_str());
		return false;
	}
	LOG4_DEBUG("(%s):before DelNodeInfo size(%u),disconnect server type(%s)",__FUNCTION__, GetMapNodeInfoSize(),delNodeInfo.nodeType.c_str());
	//从map中删除节点信息
	if(!DelNodeInfo(delNodeIdentify))
	{
		LOG4_WARN("failed to del delNodeInfo.nodeId(%d)!",delNodeInfo.nodeId);
		return false;
	}
	LOG4_DEBUG("(%s):after DelNodeInfo size(%u),disconnect server type(%s)",__FUNCTION__, GetMapNodeInfoSize(),delNodeInfo.nodeType.c_str());
	if(eMasterStatus == m_CenterActive.status)
	{//主节点才写数据库和下发消息
		SendUnregToOthers(delNodeInfo);//给其它模块发下线通知
	}
	return true;
}

int NodeSession::GetLoadMinNode(const std::string& serverType,NodeLoadStatus &nodeLoadStatus)
{//主从节点都允许分配服务器节点
    if(!CheckNodeType(serverType))
    {
        LOG4_WARN("no such nodetype(%s)",serverType.c_str());
        return ERR_SERVERINFO;
    }
    //获取指定类型的节点状态列表
    std::vector<NodeLoadStatus> vecNodeStatus;
    if(!GetNodeStatusByNodeType(serverType,vecNodeStatus))
    {
        LOG4_ERROR("get server node failed(%s)",serverType.c_str());
        return ERR_SERVERINFO;
    }
    if(vecNodeStatus.empty())
    {
        LOG4_ERROR("serverType vecNodeStatus empty!");
        return ERR_SERVERINFO;
    }
    //获取节点状态列表中负载最小的节点
	nodeLoadStatus = *vecNodeStatus.begin();
	for(auto itNode:vecNodeStatus)
	{
		if(nodeLoadStatus.serverload > itNode.serverload)//获取负载最小的节点
		{
			nodeLoadStatus = itNode;
		}
	}
    return ERR_OK;
}

int NodeSession::OfflineNode(const std::string& sOfflineIdentify)
{
    if (sOfflineIdentify == GetSelfNodeIdentify())
    {//下线的是自己
        LOG4_WARN("try to OfflineNode self");
        return ERR_SERVER_SELF_OFFLINE;
    }
	//获取对应的节点信息.delNodeIdentify(IP:端口)
	NodeStatusInfo *pOfflineNodeInfo = GetNodeInfo(sOfflineIdentify);//下线节点信息
	if (!pOfflineNodeInfo)
	{
		LOG4_WARN("OfflineNode No such sOfflineIdentify(%s)!",sOfflineIdentify.c_str());
		return ERR_SERVER_NODE_NO_EXIST;
	}
	if(eNodeStatusInfoSuspend == pOfflineNodeInfo->suspend)
	{
	    LOG4_WARN("OfflineNode already sOfflineIdentify(%s)!",sOfflineIdentify.c_str());
        return ERR_SERVER_NODE_ALREADY_OFFLINE;
	}
	uint32 nodeCount = GetNodeCountByType(pOfflineNodeInfo->nodeType);
    if(nodeCount <2)
    {
        LOG4_WARN("nodeCount(%u),need no less then 2 nodes");
        return ERR_SERVER_NODE_OFFLINE_NEED_MORE_NODES;
    }
	pOfflineNodeInfo->suspend = eNodeStatusInfoSuspend;//挂起的节点依然处理服务器内部消息，但是不处理新的业务请求
	//下线者给其它服务发通知
    int nRet = SendUnregToOthers(*pOfflineNodeInfo);
    if(nRet)
    {
        LOG4_WARN("failed to SendUnregToOthers:%s",pOfflineNodeInfo->nodeType.c_str());
        return nRet;
    }
    LOG4_INFO("%s() OfflineNode sOfflineIdentify(%s) ok",__FUNCTION__,sOfflineIdentify.c_str());
    return ERR_OK;
}

int NodeSession::OnlineNode(const std::string& sOnlineIdentify)
{
    if (sOnlineIdentify == GetSelfNodeIdentify())//192.168.18.78:27000
    {//上线的是自己
        return ERR_SERVER_CENTER_NO_ROUTES_RESTORE;
    }
    //获取对应的节点信息.sOnlineIdentify(IP:端口)
    NodeStatusInfo *pOnlineNodeInfo = GetNodeInfo(sOnlineIdentify);//上线节点信息
    if (!pOnlineNodeInfo)
    {
        LOG4_WARN("OnlineNode No such sOnlineIdentify(%s)!",sOnlineIdentify.c_str());
        return ERR_SERVER_NODE_NO_EXIST;
    }
    if(eNodeStatusInfoNormal == pOnlineNodeInfo->suspend)
    {
        LOG4_WARN("OnlineNode already online(%s)!",sOnlineIdentify.c_str());
        return ERR_SERVER_NODE_ALREADY_ONLINE;
    }
    pOnlineNodeInfo->suspend = eNodeStatusInfoNormal;
    //上线者给其它服务发通知
    int nRet = SendRegToOthers(*pOnlineNodeInfo);
    if(nRet)
    {
        LOG4_WARN("failed to SendRegToOthers:%s",pOnlineNodeInfo->nodeType.c_str());
        return nRet;
    }
    LOG4_INFO("%s() OnlineNode sOnlineIdentify(%s) ok",__FUNCTION__,sOnlineIdentify.c_str());
    return ERR_OK;
}

int NodeSession::CanOnlineNode(const std::string& sOnlineIdentify)
{
    if (sOnlineIdentify == GetSelfNodeIdentify())
    {//上线的是自己
        return ERR_SERVER_SELF_ONLINE;
    }
    //获取对应的节点信息.sOnlineIdentify(IP:端口)
    NodeStatusInfo *pOnlineNodeInfo = GetNodeInfo(sOnlineIdentify);//上线节点信息
    if (!pOnlineNodeInfo)
    {
        LOG4_WARN("OnlineNode No such sOnlineIdentify(%s)!",
                        sOnlineIdentify.c_str());
        return ERR_SERVER_NODE_NO_EXIST;
    }
    if(eNodeStatusInfoNormal == pOnlineNodeInfo->suspend)
    {
        LOG4_WARN("OnlineNode already online(%s)!",sOnlineIdentify.c_str());
        return ERR_SERVER_NODE_ALREADY_ONLINE;
    }
    return ERR_OK;
}

int NodeSession::RegNodeRoute(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,const NodeStatusInfo& nodeinfo)
{
    NodeStatusInfo *pNodeInfo = GetNodeInfo(nodeinfo.getNodeKey());
    if (pNodeInfo) //注册过的只需要更新节点信息
    {
        if (!pNodeInfo->update(nodeinfo))
        {
            LOG4_WARN("pNodeInfo(%d) update failed", nodeinfo.nodeId);
        }
        LOG4_DEBUG("Master(%s,%d) getNodeKey(%s)",m_CenterActive.inner_ip,m_CenterActive.inner_port,nodeinfo.getNodeKey().c_str());
        //返回注册响应
        ResToReg(stMsgShell, oInMsgHead, ERR_OK,*pNodeInfo);
        //给注册者发其他服务器通知
        SendOthersToReg(stMsgShell, *pNodeInfo);
        //注册者给其它服务发通知
        SendRegToOthers(*pNodeInfo);
        return (ERR_OK);
    }
    else //没有注册过的需要检查,然后分配节点并注册
    {
        NodeStatusInfo tmpRegNodeStatus = nodeinfo;
        tmpRegNodeStatus.nodeId = GetNewNodeID(); //需用节点id分配器来分配节点ID
        LOG4_DEBUG("Master(%s,%d) getNodeKey(%s),new nodeId(%d)",m_CenterActive.inner_ip,m_CenterActive.inner_port,
                                                nodeinfo.getNodeKey().c_str(),tmpRegNodeStatus.nodeId);
        LOG4_DEBUG("(%s):before AddNodeInfo size(%u)",__FUNCTION__, GetMapNodeInfoSize());
        //加入到节点管理MAP
        AddNodeInfo(tmpRegNodeStatus.getNodeKey(), tmpRegNodeStatus);
        LOG4_DEBUG("(%s):after AddNodeInfo(%s) size(%u)",__FUNCTION__,tmpRegNodeStatus.getNodeKey().c_str(),GetMapNodeInfoSize());
        //返回注册响应
        ResToReg(stMsgShell, oInMsgHead,ERR_OK,tmpRegNodeStatus);
        //给注册者发其他服务器通知
        SendOthersToReg(stMsgShell,tmpRegNodeStatus);
        //注册者给其它服务发通知
        SendRegToOthers(tmpRegNodeStatus);
        return (ERR_OK);
    }
}

bool NodeSession::ResToReg(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,int iRet,const NodeStatusInfo &regNodeStatus)
{
	LOG4_TRACE("%s()",__FUNCTION__);
    util::CJsonObject jObjReturn;
    jObjReturn.Add("errcode", iRet);
    jObjReturn.Add("node_id", iRet ? 0 : regNodeStatus.nodeId);
    return net::SendToClient(stMsgShell, oInMsgHead, jObjReturn.ToString());
}

//发送其他服务器给注册者
int NodeSession::SendOthersToReg(const net::tagMsgShell& stMsgShell,const NodeStatusInfo &regNodeStatus)
{
	LOG4_TRACE("%s()",__FUNCTION__);
    const std::string& strRegNodeType = regNodeStatus.nodeType;
    //发送其他服务器给注册者
    //发送格式{\"node_arry_reg\":[{\\"node_type\\":\"LOGIC\",\\"node_ip\\":\"192.168.18.22\",\\"node_port\\":40120,\\"worker_num\\":2}]}
	const NodeType* pNodeType = GetNodeTypeServerInfo(strRegNodeType);
	if (pNodeType)
	{
		util::CJsonObject objRegNodes;
		GetNeededNodesStatus(pNodeType->neededServers, objRegNodes);//获取需要的已注册的服务
		net::SendToCallback(this,net::CMD_REQ_NODE_REG_NOTICE,objRegNodes.ToString(),defaultCallback,stMsgShell);
	}
	else
	{
		LOG4_TRACE("node type(%s) don't need other server routes",strRegNodeType.c_str());
	}
    return (ERR_OK);
}

//发送注册者给其它服务
int NodeSession::SendRegToOthers(const NodeStatusInfo &regNodeStatus)
{
	LOG4_TRACE("%s()",__FUNCTION__);
    if(regNodeStatus.suspend)//正常状态的节点才把自己的路由发布出去
    {
    	LOG4_TRACE("regNodeStatus(%s,%s) is suspend",regNodeStatus.getNodeKey().c_str(),regNodeStatus.nodeType.c_str());
        return (ERR_OK);
    }
    const std::string& strRegNodeType = regNodeStatus.nodeType;
    //发送格式 {\"node_arry_reg\":[{\\"node_type\\":\"ACCESS\",\\"node_ip\\":\"192.168.18.22\",\\"node_port\\":40111,\\"worker_num\\":2}]}
	util::CJsonObject jRegNodeObj;
	jRegNodeObj.AddEmptySubArray("node_arry_reg");
	util::CJsonObject tmember;
	tmember.Add("node_type", regNodeStatus.nodeType);
	tmember.Add("node_ip", regNodeStatus.nodeInnerIp);
	tmember.Add("node_port", regNodeStatus.nodeInnerPort);
	tmember.Add("worker_num", regNodeStatus.workerNum);
	jRegNodeObj["node_arry_reg"].Add(tmember);
	const std::string& toNoticeMsgBody = jRegNodeObj.ToString();
	for (NodesStatusMapCIT it_iter = m_mapNodesStatus.begin(); it_iter != m_mapNodesStatus.end(); ++it_iter) //已注册服务器
	{
		const NodeStatusInfo& info = it_iter->second;
		//给其他服务发送通知(包括同一个节点的其他子进程)
		const NodeType* pNodeType = GetNodeTypeServerInfo(info.nodeType);
		if (pNodeType)    //已注册的节点需要的节点类型
		{
			if (pNodeType->neededServers.size() > 0)
			{
				const NodeType::ServersList& vecServersType = pNodeType->neededServers;
				for (NodeType::ServersListCIT it = vecServersType.begin();it != vecServersType.end(); ++it)
				{
					if (strRegNodeType == *it)  //刚注册的服务是已注册服务需要的节点类型，则发送通知
					{//通知已注册的服务
						net::SendToCallback(this,net::CMD_REQ_NODE_REG_NOTICE,toNoticeMsgBody,defaultCallback,info.getNodeKey());
					}
				}
			}
			else
			{
				LOG4_DEBUG("registed node type(%s) don't need other node routes",
								info.nodeType.c_str());
			}
		}
		else
		{
			LOG4_WARN("node type(%s) don't have such server,please check table tb_nodetype",
							info.nodeType.c_str());
		}
	}
    return (ERR_OK);
}

int NodeSession::SendCenterToReg(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,const NodeStatusInfo &regNodeStatus)
{
    //发送中心服务器给注册者
    //发送格式{\"node_arry_reg\":[{\\"node_type\\":\"CENTER\",\\"node_ip\\":\"192.168.18.22\",\\"node_port\\":40120,\\"worker_num\\":1}]}
	const NodeType* pNodeType = GetNodeTypeServerInfo(regNodeStatus.nodeType);
	if (pNodeType)
	{
		util::CJsonObject objRegNodes;
		GetCenterNodesStatus(objRegNodes);//获取本中心节点信息
		if (!net::SendToCallback(this,net::CMD_REQ_NODE_REG_NOTICE,objRegNodes.ToString(),defaultCallback,stMsgShell))
		{
			LOG4_WARN("send to failed,strRegNodeType:%s",regNodeStatus.nodeType.c_str());
			return (ERR_SERVER_ERROR);
		}
	}
	else
	{
		LOG4_DEBUG("node type(%s) don't need other server routes",regNodeStatus.nodeType.c_str());
	}
    return (ERR_OK);
}

//获取需要的已注册的服务器的节点状态(json格式)
bool NodeSession::GetNeededNodesStatus(const std::vector<std::string>& neededServers,util::CJsonObject &jObj)
{
    const NodeStatusInfo *pInfo = NULL;
    jObj.AddEmptySubArray("node_arry_reg");
    util::CJsonObject tmember;
    for (const auto& it_iter:m_mapNodesStatus) //已注册服务器
    {
        pInfo = &it_iter.second;
        LOG4_TRACE("online nodetype[%s] suspend(%d)",pInfo->nodeType.c_str(),pInfo->suspend);
        for (const auto& it:neededServers)
        {
        	LOG4_TRACE("need nodetype[%s] checking online nodetype[%s] suspend(%d)",it.c_str(),pInfo->nodeType.c_str(),pInfo->suspend);
            if (it == pInfo->nodeType) //判断是否是需要的服务器类型
            {
                if(eNodeStatusInfoNormal == pInfo->suspend)//正常节点才能下发路由
                {
                	LOG4_TRACE("add nodetype[%s]",pInfo->nodeType.c_str());
                    tmember.Clear();
                    tmember.Add("node_type", pInfo->nodeType);
                    tmember.Add("node_ip", pInfo->nodeInnerIp);
                    tmember.Add("node_port", pInfo->nodeInnerPort);
                    tmember.Add("worker_num", pInfo->workerNum);
                    jObj["node_arry_reg"].Add(tmember);
                }
                else
                {
                	LOG4_TRACE("need nodetype[%s] suspend(%d)",pInfo->nodeType.c_str(),pInfo->suspend);
                }
            }
            else
            {
            	LOG4_TRACE("need nodetype[%s] skip online nodetype[%s] suspend(%d)",it.c_str(),pInfo->nodeType.c_str(),pInfo->suspend);
            }
        }
    }
    //中心服务器信息
    tmember.Clear();
    tmember.Add("node_type", m_centerNodeType);
    tmember.Add("node_ip", m_centerInnerHost);
    tmember.Add("node_port", m_centerInnerPort);
    tmember.Add("worker_num", m_centerProcessNum);
    jObj["node_arry_reg"].Add(tmember);
    LOG4_DEBUG("%s() center_node_type(%s,%s)",__FUNCTION__,m_centerNodeType.c_str(),tmember.ToString().c_str());
    return (true);
}

bool NodeSession::GetCenterNodesStatus(util::CJsonObject &jObj)
{
    jObj.AddEmptySubArray("node_arry_reg");
    util::CJsonObject tmember;
    //中心服务器信息
    tmember.Add("node_type", m_centerNodeType);
    tmember.Add("node_ip", m_centerInnerHost);
    tmember.Add("node_port", m_centerInnerPort);
    tmember.Add("worker_num", m_centerProcessNum);
    jObj["node_arry_reg"].Add(tmember);
    LOG4_DEBUG("%s() center_node_type(%s,%s)",__FUNCTION__,m_centerNodeType.c_str(),tmember.ToString().c_str());
    return (true);
}

//发送连接断开通知到其它服务
int NodeSession::SendUnregToOthers(const NodeStatusInfo &delNodeInfo)
{
    util::CJsonObject oExitNode,tmember;
    oExitNode.AddEmptySubArray("node_arry_exit");
    tmember.Add("node_type", delNodeInfo.nodeType);
    tmember.Add("node_ip", delNodeInfo.nodeInnerIp);
    tmember.Add("node_port", delNodeInfo.nodeInnerPort);
    tmember.Add("worker_num", delNodeInfo.workerNum);
    oExitNode["node_arry_exit"].Add(tmember);
    const std::string& strDisConnectBody = oExitNode.ToString();
    LOG4_DEBUG("SendUnregToOthers!oExitNode[%s]",strDisConnectBody.c_str());
    bool boSendedNotice(false);
    //遍历管理器内存的node列表,如果断开连接的服务是它们需要的服务,则通知它们注销该断开连接的服务
    {
        //在线节点管理器（key为节点类型：IP：端口，value为节点信息）
        for (const auto& it_iter: m_mapNodesStatus)//已注册的服务器
        {
            const NodeStatusInfo& nodeInfo = it_iter.second;
            //获取其他服务的配置(同一个节点的其他子进程也通知)，如果是需要该退出服务路由的则通知
            const NodeType* pNodeType = GetNodeTypeServerInfo(nodeInfo.nodeType);
            if(pNodeType)
            {
                for(const auto& it:pNodeType->neededServers)//该类服务器需要的服务器类型
                {
                    if(it == delNodeInfo.nodeType)//注销的服务器是该类服务器需要的服务器,则通知该类服务器注销
                    {
                        LOG4_DEBUG("%s() nodeInfo(%s)!oExitNode[%s]",__FUNCTION__,nodeInfo.getNodeKey().c_str(),oExitNode.ToString().c_str());
						if (!net::SendToCallback(this,net::CMD_REQ_NODE_REG_NOTICE,strDisConnectBody,defaultCallback,nodeInfo.getNodeKey()))
						{
							LOG4_WARN("%s() send to %s failed",__FUNCTION__,nodeInfo.getNodeKey().c_str());
						}
						else
						{
							boSendedNotice = true;
						}
                    }
                }
            }
            else
            {
                LOG4_WARN("%s() nodeInfo(%s) don't have server config,please check config file",__FUNCTION__,nodeInfo.nodeType.c_str());
            }
        }
    }
    if (!boSendedNotice)
    {
        LOG4_TRACE("did not send any notices to unregister node.oExitNode(%s)",oExitNode.ToString().c_str());
    }
    return (net::ERR_OK);
}
//关闭网关服务器（INTREFACE、ACCESS、OSSI）到指定更新节点路由信息
int NodeSession::SendOfflineToGate(const NodeStatusInfo &offlineNodeInfo)
{
    util::CJsonObject oExitNode,tmember;
    oExitNode.AddEmptySubArray("node_arry_exit");
    tmember.Add("node_type", offlineNodeInfo.nodeType);
    tmember.Add("node_ip", offlineNodeInfo.nodeInnerIp);
    tmember.Add("node_port", offlineNodeInfo.nodeInnerPort);
    tmember.Add("worker_num", offlineNodeInfo.workerNum);
    oExitNode["node_arry_exit"].Add(tmember);
    const std::string& strOfflineBody = oExitNode.ToString();
    LOG4_DEBUG("SendOfflineToGate!oExitNode[%s]",strOfflineBody.c_str());
    bool boSendedNotice(false);
    //遍历管理器内存的node列表,如果断开连接的服务是它们需要的服务,则通知它们注销该断开连接的服务
    {
        //注销的服务类型
        const std::string& offlineNodeType = offlineNodeInfo.nodeType;
        //在线节点管理器（key为节点类型：IP：端口，value为节点信息）
        for (NodeSession::NodesStatusMapCIT it_iter =
        		m_mapNodesStatus.begin();
                        it_iter != m_mapNodesStatus.end(); ++it_iter)//已注册的服务器
        {
            const NodeStatusInfo& nodeInfo = it_iter->second;
            if(!IsGate(nodeInfo.nodeType))//如果不是网关类型服务则不发送
            {
                continue;
            }
            //获取其他服务的配置
            const NodeType* pNodeType = GetNodeTypeServerInfo(nodeInfo.nodeType);
            if(pNodeType)
            {
                //其他服务需要的服务
                const std::vector<std::string>& neededServers = pNodeType->neededServers;
                for(std::vector<std::string>::const_iterator it = neededServers.begin();
                                        it != neededServers.end();++it)//该类服务器需要的服务器类型
                {
                    if(offlineNodeType == *it)//下线的服务是该类服务器需要的服务器,则通知该类服务器注销路由
                    {
						if (!net::SendToCallback(this,net::CMD_REQ_NODE_REG_NOTICE,strOfflineBody,defaultCallback,nodeInfo.getNodeKey()))
						{
							LOG4_WARN("%s send to %s failed",__FUNCTION__,nodeInfo.getNodeKey().c_str());
						}
						else
						{
							boSendedNotice = true;
						}
                    }
                }
            }
            else
            {
                LOG4_WARN("SendOfflineToGate!notify node type(%s) don't have server config,please check table tb_nodetype",
                            nodeInfo.nodeType.c_str());
            }
        }
    }
    if (!boSendedNotice)
    {
        LOG4_DEBUG("did not send any notices to unregister node.oExitNode(%s)",oExitNode.ToString().c_str());
    }
    return (net::ERR_OK);
}

//发送注册者给网关服务
int NodeSession::SendOnlineToGate(const NodeStatusInfo& onlineNodeInfo)
{
    const std::string& strOnlineNodeType = onlineNodeInfo.nodeType;
    //发送格式 {\"node_arry_reg\":[{\\"node_type\\":\"ACCESS\",\\"node_ip\\":\"192.168.18.22\",\\"node_port\\":40111,\\"worker_num\\":2}]}
	util::CJsonObject jRegNodeObj;
	jRegNodeObj.AddEmptySubArray("node_arry_reg");
	util::CJsonObject tmember;
	tmember.Add("node_type", onlineNodeInfo.nodeType);
	tmember.Add("node_ip", onlineNodeInfo.nodeInnerIp);
	tmember.Add("node_port", onlineNodeInfo.nodeInnerPort);
	tmember.Add("worker_num", onlineNodeInfo.workerNum);
	jRegNodeObj["node_arry_reg"].Add(tmember);
	const std::string& strOnlineBody = jRegNodeObj.ToString();
	for (const auto& it_iter:m_mapNodesStatus) //已注册服务器
	{
		const NodeStatusInfo& nodeInfo = it_iter.second;
		if(!IsGate(nodeInfo.nodeType))//如果不是网关类型服务则不发送
		{
			continue;
		}
		//给其他服务发送通知
		const NodeType* pNodeType = GetNodeTypeServerInfo(nodeInfo.nodeType);
		if (pNodeType)    //已注册的节点需要的节点类型
		{
			const std::vector<std::string>& vecServersType = pNodeType->neededServers;
			if (vecServersType.size())
			{
				for (const auto& it:vecServersType)
				{
					if (strOnlineNodeType == it)  //上线服务是已注册服务需要的节点类型，则发送通知
					{//通知已注册的服务
						net::SendToCallback(this,net::CMD_REQ_NODE_REG_NOTICE,strOnlineBody,defaultCallback,nodeInfo.getNodeKey());
					}
				}
			}
			else
			{
				LOG4_DEBUG("online node type(%s) don't need other node routes",nodeInfo.nodeType.c_str());
			}
		}
		else
		{
			LOG4_WARN("notify node type(%s) don't have such server,please check table tb_nodetype",nodeInfo.nodeType.c_str());
		}
	}
    return (ERR_OK);
}


bool NodeSession::IsGate(const std::string& nodetype)
{
    int s = m_GatewayTypeList.size();
    for(int i = 0;i < s;++i)
    {
        if (nodetype == m_GatewayTypeList[i])
        {
            LOG4_DEBUG("SendOfflineToGate:%s is gateway type",nodetype.c_str());
            return true;
        }
    }
    return false;
}

void NodeSession::RemoveFlag(std::string &str, char flag)const
{
	std::string::iterator it = std::remove(str.begin(), str.end(), flag);
	str.erase(it, str.end());
}

std::string NodeSession::RemoveFlagString(std::string str, char flag)const
{
	std::string::iterator it = std::remove(str.begin(), str.end(), flag);
	str.erase(it, str.end());
	return str;
}

std::string NodeSession::GetSelfNodeIdentify()const
{
	char strSelfNodeKey[100] = { 0 };
	sprintf(strSelfNodeKey, "%s:%d", m_centerInnerHost.c_str(), m_centerInnerPort);
	return std::string(strSelfNodeKey);
}
bool NodeSession::IsSelfNodeIdentify(const std::string& identify)const
{
	char strSelfNodeKey[100] = { 0 };
	sprintf(strSelfNodeKey, "%s:%d", m_centerInnerHost.c_str(), m_centerInnerPort);
	if(identify == strSelfNodeKey)
	{
		return true;
	}
	return false;
}
bool NodeSession::IsCenterServer(const std::string& identify)
{
	std::vector<CenterServer>::const_iterator it = m_vecCenterServer.begin();
	std::vector<CenterServer>::const_iterator itEnd = m_vecCenterServer.end();
	for(;it != itEnd;++it)
	{
		if (identify == it->server_identify)
		{
			return true;
		}
		LOG4_DEBUG("server_identify(%s),identify(%s)",it->server_identify.c_str(),identify.c_str());
	}
	return false;
}
bool NodeSession::HasIdentifyAuthority(const std::string& sNodeIdentify)
{
	LOG4_DEBUG("HasAuthority sNodeIdentify(%s)",sNodeIdentify.c_str());
	if(IsCenterServer(sNodeIdentify))
	{//中心节点操作由中心节点自己来完成
		if(!IsSelfNodeIdentify(sNodeIdentify))
		{
			LOG4_DEBUG("IsSelfNodeIdentify failed(%s)",sNodeIdentify.c_str());
			return false;
		}
	}
	else//非中心节点操作，由中心主节点完成
	{
		if(!IsMaster())
		{
			LOG4_DEBUG("it 's not master(%s)",sNodeIdentify.c_str());
			return false;
		}
	}
	return true;
}


NodeSession* GetNodeSession(bool boReload)
{
    NodeSession* pSess = (NodeSession*) net::GetSession(1, "net::NodeSession");
    if (pSess)
    {
        if(boReload)
        {//重新加载的重新初始化session
            std::string err;
            if(!pSess->Init(err,boReload))
            {
                LOG4_ERROR("NodeSession init error!%s",err.c_str());
                g_pLabor->DeleteCallback(pSess);
                return NULL;
            }
        }
        return (pSess);
    }
    int nodeSessionTimeOut = (net::gc_iBeatInterval/2 -1) > 0  ? (net::gc_iBeatInterval/2 -1) :1;
    //注册节点会话
    pSess = new NodeSession(1,nodeSessionTimeOut);
    if (pSess == NULL)
    {
        LOG4_ERROR("error %d: new NodeSession() error!", ERR_NEW);
        return (NULL);
    }
    LOG4CPLUS_INFO_FMT(g_pLabor->GetLogger(),"new NodeSession(1,%d) NodeSession timeout:%d",nodeSessionTimeOut,nodeSessionTimeOut);
    std::string err;
    if(!pSess->Init(err))
    {
        LOG4_ERROR("NodeSession init error!%s",err.c_str());
        delete pSess;
        pSess = NULL;
        return (NULL);
    }
    if (net::RegisterCallback(pSess))
    {
        LOG4_DEBUG("register NodeSession ok!");
        return (pSess);
    }
    else
    {
        LOG4_ERROR("register NodeSession error!");
        delete pSess;
        pSess = NULL;
    }
    return (NULL);
}




}
;
//name space analysis
