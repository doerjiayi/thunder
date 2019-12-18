/*******************************************************************************
 * Project:  Center
 * @file     CmdRegister.cpp
 * @brief 
 * @author   bwar
 * @date:    Sep 19, 2016
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdNodeRegister.hpp"

MUDULE_CREATE(coor::CmdNodeRegister);

namespace coor
{

bool CmdNodeRegister::Init()
{
    util::CJsonObject oCenterConf = net::GetCustomConf();

    if (std::string("db_config") == oCenterConf("config_choice"))
    {
        return(InitFromDb(oCenterConf["db_config"]));
    }
    else if (std::string("local_config") == oCenterConf("config_choice"))
    {
        return(InitFromLocal(oCenterConf["local_config"]));
    }
    else
    {
        LOG4_ERROR("invalid config!");
        return(false);
    }
}

bool CmdNodeRegister::InitFromDb(const util::CJsonObject& oDbConf)
{
    return(true);
}

bool CmdNodeRegister::InitFromLocal(const util::CJsonObject& oLocalConf)
{
    util::CJsonObject oCenter = oLocalConf;
    double dSessionTimeout = 3.0;
    oCenter.Get("beacon_beat", dSessionTimeout);
    if (m_pSessionOnlineNodes == nullptr)
    {
    	m_pSessionOnlineNodes = new coor::SessionOnlineNodes(dSessionTimeout);
    	GetLabor()->RegisterCallback(m_pSessionOnlineNodes);
    }
    m_pSessionOnlineNodes->InitElection(oCenter["centers"]);
    for (int i = 0; i < oCenter["ipwhite"].GetArraySize(); ++i)
    {
        m_pSessionOnlineNodes->AddIpwhite(oCenter["ipwhite"](i));
    }
    for (int i = 0; i < oCenter["node_type"].GetArraySize(); ++i)
    {
        for (int j = 0; j < oCenter["node_type"][i]["subscribe"].GetArraySize(); ++j)
        {
            m_pSessionOnlineNodes->AddSubscribe(oCenter["node_type"][i]("node_type"), oCenter["node_type"][i]["subscribe"](j));
        }
    }
    return(true);
}

/**
 * @brief 上报节点状态信息
 * @return 上报是否成功
 * @note 节点状态信息结构如：
 * {
 *     "node_type":"ACCESS",
 *     "node_ip":"192.168.11.12",
 *     "node_port":9988,
 *     "access_ip":"120.234.2.106",
 *     "access_port":10001,
 *     "worker_num":10,
 *     "active_time":16879561651.06,
 *     "node":{
 *         "load":1885792, "connect":495873, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":495870
 *     },
 *     "worker":
 *     [
 *          {"load":655666, "connect":495873, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}},
 *          {"load":655235, "connect":485872, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}},
 *          {"load":585696, "connect":415379, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}}
 *     ]
 * }
 */
bool CmdNodeRegister::AnyMessage(
                const net::tagMsgShell& stMsgShell,const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    util::CJsonObject oNodeInfo;
    util::CJsonObject oNodeId;
    if (oNodeInfo.Parse(oMsgBody.body()))
    {
        uint16 unNodeId = m_pSessionOnlineNodes->AddNode(oNodeInfo);
        if (0 == unNodeId)
        {
			LOG4_ERROR("failed to AddNode !");
        	oNodeId.Add("errcode",1);
        }
        else
        {
        	LOG4_INFO("AddNode node_id(%u)!",unNodeId);
        	oNodeId.Add("node_id", unNodeId);
            oNodeId.Add("errcode",0);
        }
    }
    else
    {
        LOG4_ERROR("failed to parse node info json from MsgBody.data()!");
        oNodeId.Add("errcode",1);
    }
    LOG4_INFO("oNodeId(%s)!",oNodeId.ToString().c_str());
    return GetLabor()->SendToClient(stMsgShell,oMsgHead,oNodeId.ToString());
}

} /* namespace coor */
