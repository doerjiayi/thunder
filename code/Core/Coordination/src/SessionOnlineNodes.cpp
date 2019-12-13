/*******************************************************************************
 * Project:  Beacon
 * @file     SessionOnlineNodes.cpp
 * @brief 
 * @author   bwar
 * @date:    Sep 20, 2016
 * @note
 * Modify history:
 ******************************************************************************/
#include "CW.hpp"
#include "SessionOnlineNodes.hpp"

namespace coor
{

const uint32 SessionOnlineNodes::mc_uiLeader = 0x80000000;
const uint32 SessionOnlineNodes::mc_uiAlive = 0x00000007;   ///< 最近三次心跳任意一次成功则认为在线

SessionOnlineNodes::SessionOnlineNodes(double dSessionTimeout)
    : net::Session("coor::SessionOnlineNodes", dSessionTimeout),
      m_unLastNodeId(0), m_bIsLeader(false)
{
}

SessionOnlineNodes::~SessionOnlineNodes()
{
}

net::E_CMD_STATUS SessionOnlineNodes::Timeout()
{
    CheckLeader();
    SendBeaconBeat();
    return(net::STATUS_CMD_RUNNING);
}

void SessionOnlineNodes::AddIpwhite(const std::string& strIpwhite)
{
    m_setIpwhite.insert(strIpwhite);
}

void SessionOnlineNodes::AddSubscribe(const std::string& strNodeType, const std::string& strSubscribeNodeType)
{
    auto pub_iter = m_mapPublisher.find(strSubscribeNodeType);
    if (pub_iter == m_mapPublisher.end())
    {
        std::unordered_set<std::string> setSubscriber;
        setSubscriber.insert(strNodeType);
        m_mapPublisher.insert(std::make_pair(strSubscribeNodeType, setSubscriber));
    }
    else
    {
        pub_iter->second.insert(strNodeType);
    }
}

uint16 SessionOnlineNodes::AddNode(const util::CJsonObject& oNodeInfo)
{
    LOG4_TRACE("(%s)", oNodeInfo.ToString().c_str());
    uint32 uiNodeId = 0;
    std::unordered_set<uint16>::iterator node_id_iter;
    std::string strNodeIdentify = oNodeInfo("node_ip") + std::string(":") + oNodeInfo("node_port");
    util::CJsonObject oNodeInfoWithNodeId = oNodeInfo;
    oNodeInfo.Get("node_id", uiNodeId);

    // find node_id or create node_id
    auto identify_node_iter = m_mapIdentifyNodeId.find(strNodeIdentify);
    if (identify_node_iter == m_mapIdentifyNodeId.end())
    {
        while (0 == uiNodeId)
        {
            uiNodeId = ++m_unLastNodeId;
            node_id_iter = m_setNodeId.find(uiNodeId);
            if (node_id_iter != m_setNodeId.end())
            {
                uiNodeId = 0;
            }
            if (m_unLastNodeId >= 65535)
            {
                m_unLastNodeId = 0;
            }
            if (m_setNodeId.size() >= 65535)
            {
                LOG4_ERROR("there is no valid node_id in the system!");
                return(0);
            }
        }
        m_mapIdentifyNodeId.insert(std::make_pair(strNodeIdentify, uiNodeId));
    }
    else
    {
        uiNodeId = identify_node_iter->second;
    }
    oNodeInfoWithNodeId.Replace("node_id", uiNodeId);


    auto node_type_iter = m_mapNode.find(oNodeInfoWithNodeId("node_type"));
    if (node_type_iter == m_mapNode.end())
    {
        std::unordered_map<std::string, util::CJsonObject> mapNodeInfo;
        mapNodeInfo.insert(std::make_pair(strNodeIdentify, oNodeInfoWithNodeId));
        m_mapNode.insert(std::make_pair(oNodeInfoWithNodeId("node_type"), mapNodeInfo));
        m_mapIdentifyNodeType.insert(std::make_pair(strNodeIdentify, oNodeInfoWithNodeId("node_type")));
        m_setNodeId.insert(uiNodeId);
        m_setAddedNodeId.insert(uiNodeId);
        AddNodeBroadcast(oNodeInfoWithNodeId);
        SendBeaconBeat();
        return(uiNodeId);
    }
    else
    {
        auto node_iter = node_type_iter->second.find(strNodeIdentify);
        if (node_iter == node_type_iter->second.end())
        {
            node_type_iter->second.insert(std::make_pair(strNodeIdentify, oNodeInfoWithNodeId));
            m_setNodeId.insert(uiNodeId);
            m_setAddedNodeId.insert(uiNodeId);
            m_mapIdentifyNodeType.insert(std::make_pair(strNodeIdentify, oNodeInfoWithNodeId("node_type")));
            AddNodeBroadcast(oNodeInfoWithNodeId);
            SendBeaconBeat();
            return(uiNodeId);
        }
        else
        {
            node_iter->second = oNodeInfoWithNodeId;
            m_setNodeId.insert(uiNodeId);
            m_setAddedNodeId.insert(uiNodeId);
            SendBeaconBeat();
            return(uiNodeId);
        }
    }
}

void SessionOnlineNodes::RemoveNode(const std::string& strNodeIdentify)
{
    LOG4_TRACE("%s", __FUNCTION__);
    auto identity_node_iter = m_mapIdentifyNodeType.find(strNodeIdentify);
    if (identity_node_iter != m_mapIdentifyNodeType.end())
    {
        auto node_type_iter = m_mapNode.find(identity_node_iter->second);
        if (node_type_iter != m_mapNode.end())
        {
            auto node_iter = node_type_iter->second.find(strNodeIdentify);
            if (node_iter != node_type_iter->second.end())
            {
                uint32 uiNodeId = 0;
                node_iter->second.Get("node_id", uiNodeId);
                m_setNodeId.erase(uiNodeId);
                m_setRemovedNodeId.insert(uiNodeId);
                RemoveNodeBroadcast(node_iter->second);
                SendBeaconBeat();
                m_mapIdentifyNodeType.erase(strNodeIdentify);
                node_type_iter->second.erase(node_iter);
            }
        }
    }
}

void SessionOnlineNodes::AddBeaconBeat(const std::string& strNodeIdentify, const Election& oElection)
{
    if (!m_bIsLeader)
    {
        if (oElection.last_node_id() > 0)
        {
            m_unLastNodeId = oElection.last_node_id();
        }
        for (int32 i = 0; i < oElection.added_node_id_size(); ++i)
        {
            m_setNodeId.insert(oElection.added_node_id(i));
        }
        for (int32 j = 0; j < oElection.removed_node_id_size(); ++j)
        {
            m_setNodeId.erase(m_setNodeId.find(oElection.removed_node_id(j)));
        }
    }

    auto iter = m_mapBeacon.find(strNodeIdentify);
    if (iter == m_mapBeacon.end())
    {
        uint32 uiBeaconAttr = 1;
        if (oElection.is_leader() != 0)
        {
            uiBeaconAttr |= mc_uiLeader;
        }
        m_mapBeacon.insert(std::make_pair(strNodeIdentify, uiBeaconAttr));
    }
    else
    {
        iter->second |= 1;
        if (oElection.is_leader() != 0)
        {
            iter->second |= mc_uiLeader;
        }
    }
}

void SessionOnlineNodes::GetIpWhite(util::CJsonObject& oIpWhite) const
{
    /**
     * oIpWhite like this: 
     * [
     *     "192.168.157.138", "192.168.157.139", "192.168.157.175"
     * ]
     */
    for (auto it :m_setIpwhite)
    {
        oIpWhite.Add(it);
    }
}

void SessionOnlineNodes::GetBeacon(util::CJsonObject& oBeacon) const
{
    /**
     * oBeacon like this:
     * [
     *     {"identify":"192.168.157.176:16000.1", "leader":true, "online":true},
     *     {"identify":"192.168.157.177:16000.1", "leader":false, "online":true}
     * ]
     */
    for (auto it :m_mapBeacon)
    {
        util::CJsonObject oNode;
        oNode.Add("identify", it.first);
        if (it.second & mc_uiLeader)
        {
            oNode.Add("leader", "yes");
        }
        else
        {
            oNode.Add("leader", "no");
        }
        if (it.second & mc_uiAlive)
        {
            oNode.Add("online", "yes");
        }
        else
        {
            oNode.Add("online", "no");
        }
        oBeacon.Add(oNode);
    }
}

void SessionOnlineNodes::GetSubscription(util::CJsonObject& oSubcription) const
{
    /**
     * oSubcription like this: 
     * [
     *     {"node_type":"INTERFACE", "subcriber":["LOGIC", "LOGGER"]},
     *     {"node_type":"LOGIC", "subcriber":["LOGIC", "MYDIS", "LOGGER"]}
     * ]
     */
    for (auto pub_iter = m_mapPublisher.begin();
            pub_iter != m_mapPublisher.end(); ++pub_iter)
    {
        oSubcription.AddAsFirst(util::CJsonObject("{}"));
        oSubcription[0].Add("node_type", pub_iter->first);
        oSubcription[0].AddEmptySubArray("subcriber");
        for (auto it = pub_iter->second.begin(); it != pub_iter->second.end(); ++it)
        {
            oSubcription[0]["subcriber"].Add(*it);
        }
    }
}

void SessionOnlineNodes::GetSubscription(const std::string& strNodeType, util::CJsonObject& oSubcription) const
{
    /**
     * oSubcription like this: 
     * [
     *     "INTERFACE", "ACCESS"
     * ]
     */
    auto pub_iter = m_mapPublisher.find(strNodeType);
    if (pub_iter != m_mapPublisher.end())
    {
        for (auto it = pub_iter->second.begin(); it != pub_iter->second.end(); ++it)
        {
            oSubcription.Add(*it);
        }
    }
}

void SessionOnlineNodes::GetOnlineNode(util::CJsonObject& oOnlineNode) const
{
    /**
     * oOnlineNode like this: 
     * [
     *     {"node_type":"INTERFACE", "node":["192.168.157.131:16004", "192.168.157.132:16004"]},
     *     {"node_type":"LOGIC", "node":["192.168.157.131:16005", "192.168.157.132:16005"]},
     *     {"node_type":"DBAGENT", "node":["192.168.157.131:16007", "192.168.157.132:16007"]}
     * ]
     */
    for (auto node_iter = m_mapNode.begin(); node_iter != m_mapNode.end(); ++node_iter)
    {
        oOnlineNode.AddAsFirst(util::CJsonObject("{}"));
        oOnlineNode[0].Add("node_type", node_iter->first);
        oOnlineNode[0].AddEmptySubArray("node");
        for (auto it = node_iter->second.begin(); it != node_iter->second.end(); ++it)
        {
            oOnlineNode[0]["node"].Add(it->second("node_ip") + ":" + it->second("node_port"));
        }
    }
}

void SessionOnlineNodes::GetOnlineNode(
        const std::string& strNodeType, util::CJsonObject& oOnlineNode) const
{
    /**
     * oOnlineNode like this: 
     * [
     *     "192.168.157.131:16005", "192.168.157.132:16005"
     * ]
     */
    auto node_iter = m_mapNode.find(strNodeType);
    if (node_iter != m_mapNode.end())
    {
        for (auto it = node_iter->second.begin(); it != node_iter->second.end(); ++it)
        {
            oOnlineNode.Add(it->second("node_ip") + ":" + it->second("node_port"));
        }
    }
}

bool SessionOnlineNodes::GetNodeReport(
        const std::string& strNodeType, util::CJsonObject& oNodeReport) const
{
    auto node_iter = m_mapNode.find(strNodeType);
    if (node_iter == m_mapNode.end())
    {
        return(false);
    }
    else
    {
        for (auto it = node_iter->second.begin(); it != node_iter->second.end(); ++it)
        {
            oNodeReport.AddAsFirst(it->second);
            oNodeReport[0].Delete("worker");
        }
        return(true);
    }
}

bool SessionOnlineNodes::GetNodeReport(
        const std::string& strNodeType,
        const std::string& strIdentify,
        util::CJsonObject& oNodeReport) const
{
    auto node_iter = m_mapNode.find(strNodeType);
    if (node_iter == m_mapNode.end())
    {
        return(false);
    }
    else
    {
        auto it = node_iter->second.find(strIdentify);
        if (it == node_iter->second.end())
        {
            return(false);
        }
        else
        {
            oNodeReport.Add(it->second);
            return(true);
        }
    }
}

bool SessionOnlineNodes::GetOnlineNode(const std::string& strNodeType, std::vector<std::string>& vecNodes)
{
    auto node_iter = m_mapNode.find(strNodeType);
    if (node_iter != m_mapNode.end())
    {
        for (auto it = node_iter->second.begin(); it != node_iter->second.end(); ++it)
        {
            vecNodes.push_back(it->second("node_ip") + ":" + it->second("node_port"));
        }
        return(true);
    }
    return(false);
}

void SessionOnlineNodes::AddNodeBroadcast(const util::CJsonObject& oNodeInfo)
{
    LOG4_TRACE("(%s)", oNodeInfo.ToString().c_str());
    util::CJsonObject oSubcribeNodeInfo;
    util::CJsonObject oAddNodes;
    util::CJsonObject oAddedNodeInfo = oNodeInfo;
    oAddedNodeInfo.Delete("node");
    oAddedNodeInfo.Delete("worker");
    oAddNodes.Add("add_nodes", util::CJsonObject("[]"));
    oAddNodes["add_nodes"].Add(oAddedNodeInfo);
    oSubcribeNodeInfo.Add("add_nodes", util::CJsonObject("[]"));
    for (auto sub_iter = m_mapPublisher.begin();
                    sub_iter != m_mapPublisher.end(); ++sub_iter)
    {
        for (auto node_type_iter = sub_iter->second.begin(); node_type_iter != sub_iter->second.end(); ++node_type_iter)
        {
            /* send this node info to subscriber */
            if (sub_iter->first == oNodeInfo("node_type"))
            {
                auto node_list_iter = m_mapNode.find(*node_type_iter);
                if (node_list_iter != m_mapNode.end())
                {
                    LOG4_TRACE("m_mapNode[%s].size() = %u", node_type_iter->c_str(), node_list_iter->second.size());
                    try
                    {
                        for (auto node_iter = node_list_iter->second.begin(); node_iter != node_list_iter->second.end(); ++node_iter)
                        {
                            if (node_iter->second("node_id") == oNodeInfo("node_id"))
                            {
                                continue;
                            }

                            MsgBody oMsgBody;
                            oMsgBody.set_body(oAddNodes.ToString());
                            net::ExecStep(new coor::StepNodeBroadcast(node_iter->first, (int32)net::CMD_REQ_NODE_NOTICE, oMsgBody))
                        }
                    }
                    catch(std::bad_alloc& e)
                    {
                        LOG4_ERROR("new StepNodeBroadcast error: %s", e.what());
                    }
                }
            }

            /* make subscribe node info */
            if ((*node_type_iter) == oNodeInfo("node_type"))
            {
                auto node_list_iter = m_mapNode.find(sub_iter->first);
                if (node_list_iter != m_mapNode.end())
                {
                    for (auto node_iter = node_list_iter->second.begin(); node_iter != node_list_iter->second.end(); ++node_iter)
                    {
                        util::CJsonObject oExistNodeInfo = node_iter->second;
                        oExistNodeInfo.Delete("node");
                        oExistNodeInfo.Delete("worker");
                        oSubcribeNodeInfo["add_nodes"].Add(oExistNodeInfo);
                    }
                }
            }

        }
    }

    /* send subscribe node info to this node */
    if (oSubcribeNodeInfo["add_nodes"].GetArraySize() > 0)
    {
		MsgBody oMsgBody;
		char szThisNodeIdentity[32];
		oMsgBody.set_body(oSubcribeNodeInfo.ToString());
		snprintf(szThisNodeIdentity, sizeof(szThisNodeIdentity),
						"%s:%s", oNodeInfo("node_ip").c_str(), oNodeInfo("node_port").c_str());
		net::ExecStep(new coor::StepNodeBroadcast(szThisNodeIdentity,net::CMD_REQ_NODE_REG_NOTICE,oMsgBody));
    }
}

void SessionOnlineNodes::RemoveNodeBroadcast(const util::CJsonObject& oNodeInfo)
{
    LOG4_TRACE("(%s)", oNodeInfo.ToString().c_str());
    std::unordered_map<std::string, std::unordered_map<std::string, util::CJsonObject> >::iterator node_list_iter;
    std::unordered_map<std::string, util::CJsonObject>::iterator node_iter;
    std::unordered_set<std::string>::iterator node_type_iter;
    util::CJsonObject oDelNodes;
    util::CJsonObject oDeletedNodeInfo = oNodeInfo;
    oDeletedNodeInfo.Delete("node");
    oDeletedNodeInfo.Delete("worker");
    oDelNodes.Add("del_nodes", util::CJsonObject("[]"));
    oDelNodes["del_nodes"].Add(oDeletedNodeInfo);
    for (auto sub_iter = m_mapPublisher.begin();
                    sub_iter != m_mapPublisher.end(); ++sub_iter)
    {
        for (node_type_iter = sub_iter->second.begin(); node_type_iter != sub_iter->second.end(); ++node_type_iter)
        {
            /* send this node info to subscriber */
            if (sub_iter->first == oNodeInfo("node_type"))
            {
                node_list_iter = m_mapNode.find(*node_type_iter);
                if (node_list_iter != m_mapNode.end())
                {
                    for (node_iter = node_list_iter->second.begin(); node_iter != node_list_iter->second.end(); ++node_iter)
                    {
                        MsgBody oMsgBody;
                        oMsgBody.set_body(oDelNodes.ToString());
                        net::ExecStep(new coor::StepNodeBroadcast(node_iter->first,net::CMD_REQ_NODE_REG_NOTICE,oMsgBody));
                    }
                }
            }
        }
    }
}

void SessionOnlineNodes::InitElection(const util::CJsonObject& oBeacon)
{
    util::CJsonObject oBeaconList = oBeacon;
    for (int i = 0; i < oBeaconList.GetArraySize(); ++i)
    {
        m_mapBeacon.insert(std::make_pair(oBeaconList(i) + ".1", 0));
    }
    if (m_mapBeacon.size() == 0)
    {
        m_bIsLeader = true;
    }
    else if (m_mapBeacon.size() == 1
            && GetNodeIdentify() == m_mapBeacon.begin()->first)
    {
        m_bIsLeader = true;
    }
    else
    {
        SendBeaconBeat();
    }
}

void SessionOnlineNodes::CheckLeader()
{
    LOG4_TRACE("");
    std::string strLeader;
    for (auto iter = m_mapBeacon.begin(); iter != m_mapBeacon.end(); ++iter)
    {
        if (mc_uiAlive & iter->second)
        {
            if (mc_uiLeader & iter->second)
            {
                strLeader = iter->first;
            }
            else if (strLeader.size() == 0)
            {
                strLeader = iter->first;
            }
        }
        else
        {
            iter->second &= (~mc_uiLeader);
        }
        uint32 uiLeaderBit = mc_uiLeader & iter->second;
        iter->second = ((iter->second << 1) & mc_uiAlive) | uiLeaderBit;
        if (iter->first == GetNodeIdentify())
        {
            iter->second |= 1;
        }
    }

    if (strLeader == GetNodeIdentify())
    {
        m_bIsLeader = true;
    }
}

void SessionOnlineNodes::SendBeaconBeat()
{
    LOG4_TRACE("");
    MsgBody oMsgBody;
    Election oElection;
    if (m_bIsLeader)
    {
        oElection.set_is_leader(1);
        oElection.set_last_node_id(m_unLastNodeId);
        for (auto it = m_setAddedNodeId.begin(); it != m_setAddedNodeId.end(); ++it)
        {
            oElection.add_added_node_id(*it);
        }
        for (auto it = m_setRemovedNodeId.begin(); it != m_setRemovedNodeId.end(); ++it)
        {
            oElection.add_removed_node_id(*it);
        }
    }
    else
    {
        oElection.set_is_leader(0);
    }
    m_setAddedNodeId.clear();
    m_setRemovedNodeId.clear();
    oMsgBody.set_data(oElection.SerializeAsString());

    for (auto iter = m_mapBeacon.begin(); iter != m_mapBeacon.end(); ++iter)
    {
        if (GetNodeIdentify() != iter->first)
        {
            net::SendTo(iter->first, net::CMD_REQ_LEADER_ELECTION, GetSequence(), oMsgBody);
        }
    }
}

}
