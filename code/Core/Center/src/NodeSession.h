/*
 * NodeSession.h
 *
 *  Created on: 2015年10月21日
 *      Author: chen
 */
#ifndef CODE_CENTERSERVER_SRC_NODESESSION_H_
#define CODE_CENTERSERVER_SRC_NODESESSION_H_
#include <string>
#include <map>
#include <list>
#include <time.h>
#include <utility>
#include "server.pb.h"
#include "user_basic.pb.h"
#include "session/Timer.hpp"
#include "step/StepState.hpp"
#include "Comm.hpp"
#include <stdlib.h>
#include <unistd.h>
#include "redlock/redlock-cpp/redlock.h"

enum NodeStatus
{
    eNodeStatus_Online = 1,
    eNodeStatus_Offline = 2,
};

//中心主节点处理服务器内部消息，中心从节点作为热备份。两个节点都处理外部请求业务
enum CenterStatus
{
    eOfflineStatus = 0,
    eMasterStatus = 1,
    eSlaveStatus = 2,
};

namespace core
{

struct CustomRedLock
{
	CustomRedLock(){m_redlock = new CRedLock();}
	~CustomRedLock(){delete m_redlock;}
	bool Load(util::CJsonObject& redlock)
	{
		int s = redlock.GetArraySize();
		for(int i = 0;i < s;++i)
		{
			std::string host;int port(0);
			util::CJsonObject obj = redlock[i];
			LOAD_CONFIG(obj,"host",host);
			LOAD_CONFIG(obj,"port",port);

			m_redlock->AddServerUrl(host.c_str(), port);
			LOG4_INFO("redlock port:%d host:%s",port,host.c_str());
		}
		return true;
	}
	bool ContinueLock()
	{
		CLock lock;
		return  m_redlock->ContinueLock("center_master", 21000, lock,true);
	}
	unsigned int ServerSize() {return m_redlock->RedisServerSize();}
	const vector<redisContext *>& Servers() {return m_redlock->RedisServers();}
private:
	CRedLock * m_redlock;
};

class NodeSession: public net::Timer
{
public:
    NodeSession(uint32 ulSessionId, ev_tstamp dSessionTimeout,const std::string& strSessionName="net::NodeSession"):
        Timer(ulSessionId, dSessionTimeout, strSessionName),
            boInit(false),m_nCheckActiveCounter(0),m_nNodeTimeBeat(0),
			m_uiNodeId(0),m_uiCurrentTime(0),m_uiInitSessionTime(0),
			m_centerInnerPort(0), m_centerProcessNum(0)
    {
        SetCurrentTime();
        m_CenterActive.activetime = m_uiCurrentTime;
        m_CenterActive.status = eOfflineStatus;//根据仲裁来判断
        m_nNodeTimeBeat = dSessionTimeout;//中心活跃上报时间

    }
    virtual ~NodeSession(){}
    net::E_CMD_STATUS Timeout();
    //读取配置
    bool ReadConfig();
    //初始化
    bool Init(std::string &err,bool boReload=false);
public:
    /* ******************* 服务器路由功能     * */
    //加载服务路由配置
	bool LoadNodeRoute();
    //注册节点
    int RegNode(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const MsgBody& oInMsgBody, const NodeStatusInfo& nodeinfo);
    //更新节点
    int UpdateNode(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,const NodeStatusInfo& nodeinfo);
    //删除节点
    bool DelNode(const std::string& delNodeIdentify);
	//注册节点路由(已有节点会更新).注册失败返回错误码
	//注册成功会发送以下消息：	//(1)发送返回注册响应,会分配节点id	//(2)发送注册服务器的配置信息(注册成功后才调用)	//(3)发送其他服务器给注册者、发送注册者给其它服务
	//注册返回信息：（1）注册响应,会分配节点id（2）发送注册服务器的配置信息(注册成功后才调用)（3）给注册者发其他服务器通知、注册者的给其它服务发通知    //返回注册响应,会分配节点id
	int RegNodeRoute(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,const NodeStatusInfo& nodeinfo);
	//检查节点类型
	bool CheckNodeType(const std::string& nodeType);
	//发送返回注册响应,会分配节点id
	bool ResToReg(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,int iRet,const NodeStatusInfo &regNodeStatus);
	//发送其他服务器给注册者
	int SendOthersToReg(const net::tagMsgShell& stMsgShell,const NodeStatusInfo &regNodeStatus);
	//发送注册者给其它服务
	int SendRegToOthers(const NodeStatusInfo &regNodeStatus);
	//发送中心服务器给注册者
	int SendCenterToReg(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,const NodeStatusInfo &regNodeStatus);
	//发送连接断开通知到其它服务
	int SendUnregToOthers(const NodeStatusInfo &delNodeInfo);
	//获取新的节点id
	uint32 GetNewNodeID();
	// 添加标识的节点信息到在线节点管理器中去
	void AddNodeInfo(const std::string& NodeKey, const NodeStatusInfo& Info);
	//从在线节点管理器删除节点信息
	bool DelNodeInfo(const std::string& NodeKey);
	//从在线节点管理器中获取节点信息
	bool GetNodeInfo(const std::string &NodeKey, NodeStatusInfo &nInfo);
	//从在线节点管理器中获取节点信息
	NodeStatusInfo *GetNodeInfo(const std::string &NodeKey);
	//获取指定类型节点的在线数量
	uint32 GetNodeCountByType(const std::string &nodeType);
	//获取节点数量
	uint32 GetMapNodeInfoSize(){return m_mapNodesStatus.size();}
	//检查白名单
	bool CheckWhiteNode(const std::string& nodeInnerIp);
	//根据节点类型获取所有节点状态
	bool GetNodeStatusByNodeType(const std::string & nodetype,std::vector<NodeLoadStatus>& vecNodeStatus);
	//获取服务器类型的配置信息
	const NodeType* GetNodeTypeServerInfo(const std::string &nodeType);
	//获取需要的已注册的服务器的节点状态(json格式)
	bool GetNeededNodesStatus(const std::vector<std::string>& neededServers,util::CJsonObject &jObj);
	//获取中心服务器的节点状态(json格式)
	bool GetCenterNodesStatus(util::CJsonObject &jObj);
    //获取最小负载节点
	int GetLoadMinNode(const std::string& serverType,NodeLoadStatus &nodeLoadStatus);
    /* ********************灰度功能（以及热备份功能）* */
	//下线节点
	int OfflineNode(const std::string& sOfflineIdentify);
	//上线节点
	int OnlineNode(const std::string& sOnlineIdentify);
	//检查能否操作上线
	int CanOnlineNode(const std::string& sOnlineIdentify);
	//检查中心活跃信息
	bool CheckCenterActive();
	//检查正在运行并被挂起的节点
	bool CheckNodeSuspend(NodeStatusInfo& nodeinfo);
	//发送下线通知到网关服务
	int SendOfflineToGate(const NodeStatusInfo &offlineNodeInfo);
	//发送上线通知到网关服务
	int SendOnlineToGate(const NodeStatusInfo& onlineNodeInfo);
	//是否是网关类型
	bool IsGate(const std::string& nodetype);
	bool IsMaster()const {return (eMasterStatus == m_CenterActive.status);}
	bool IsSlave()const{return (eSlaveStatus == m_CenterActive.status);}
	//获取自身的标识符ip:port
	std::string GetSelfNodeIdentify()const;
	bool IsSelfNodeIdentify(const std::string& identify)const;
	bool IsCenterServer(const std::string& identify);
	bool HasIdentifyAuthority(const std::string& sNodeIdentify);
	/* ********************服务器配置管理功能* */
    //检查节点状态
    bool CheckNodeStatus(const NodeStatusInfo& nodeinfo);
	void SetCurrentTime(){m_uiCurrentTime = ::time(NULL);}
	//去掉符号
	void RemoveFlag(std::string &str, char flag = '\"')const;
	std::string RemoveFlagString(std::string str, char flag = '\"')const;

	bool SelectMaster();
private:
	CustomRedLock m_RedLock;
    //中心活跃状态
    CenterActive m_CenterActive;
    //节点类型配置
    NodeTypesVec m_vecNodeTypes;
    //服务器白名单
    std::vector<WhiteNode> m_vecWhiteNode;
    //中心服务器状态
    std::vector<CenterActive> m_vecCenterActive;
    //节点状态管理器（key为节点类型：IP：端口，value为节点状态信息）
    typedef std::map<std::string, NodeStatusInfo> NodesStatusMap;
    typedef NodesStatusMap::iterator NodesStatusMapIT;
    typedef NodesStatusMap::const_iterator NodesStatusMapCIT;
    NodesStatusMap m_mapNodesStatus;

    bool boInit;
    int m_nCheckActiveCounter;//检查活跃计数器
    int m_nNodeTimeBeat;//节点心跳时间间隔

    //中心服务器配置CenterCmd.json
    util::CJsonObject m_oCurrentConf;       ///< 当前加载的配置

    //config
    util::CJsonObject m_objRoute;

    //节点分配器
    uint32 m_uiNodeId;
    //当前时间
    uint64 m_uiCurrentTime;
    //初始化session时间
    uint64 m_uiInitSessionTime;
    /*路由配置，如：
    * "center_inner_host":"192.168.18.68",  "center_inner_port":15000,  "center_node_type":"CENTER",  "center_process_num":1
    * */
    std::string m_centerInnerHost;//中心服务器内网地址
    int m_centerInnerPort;//中心服务器内网端口
    std::string m_centerNodeType;//中心服务器节点类型
    int m_centerProcessNum;//中心服务器工作进程数

    std::vector<std::string> m_GatewayTypeList;//网关类型列表

    //节点配置文件
    std::vector<NodeConfigFile> m_vecNodeConfigFile;
    //中心服务器列表
    std::vector<CenterServer> m_vecCenterServer;
};

NodeSession* GetNodeSession(bool boReload=false);

}
;

#endif /* CODE_CENTERSERVER_SRC_NODESESSION_H_ */
