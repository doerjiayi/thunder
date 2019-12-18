/*******************************************************************************
 * Project:  DataProxyServer
 * @file     CmdLocateData.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年4月18日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdLocateData.hpp"

MUDULE_CREATE(core::CmdLocateData);

namespace core
{

CmdLocateData::CmdLocateData():m_pProxySess(NULL)
{
}

CmdLocateData::~CmdLocateData()
{
}

bool CmdLocateData::Init()
{
	m_pProxySess = GetDataProxySession();
	if (m_pProxySess)
	{
		return(true);
	}
    return(false);
}

bool CmdLocateData::AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody)
{
    DataMem::MemOperate oMemOperate;
    if (oMemOperate.ParseFromString(oInMsgBody.body()))
    {
        if (!oMemOperate.has_db_operate())
        {
            return(RedisOnly(stMsgShell, oInMsgHead, oMemOperate));
        }
        if (!oMemOperate.has_redis_operate())
        {
            return(DbOnly(stMsgShell, oInMsgHead, oInMsgBody));
        }
        if (oMemOperate.has_db_operate() && oMemOperate.has_redis_operate())
        {
            return(RedisAndDb(stMsgShell, oInMsgHead, oInMsgBody));
        }
        LOG4_ERROR("%d: %s", ERR_INCOMPLET_DATAPROXY_DATA, "neighter redis_operate nor db_operate was exist!");
        Response(stMsgShell, oInMsgHead, ERR_INCOMPLET_DATAPROXY_DATA, "neighter redis_operate nor db_operate was exist!");
        return(false);
    }
    else
    {
        LOG4_ERROR("%d: %s", ERR_PARASE_PROTOBUF, "failed to parse DataMem::MemOperate from oInMsgBody.body()!");
        Response(stMsgShell, oInMsgHead, ERR_PARASE_PROTOBUF, "failed to parse DataMem::MemOperate from oInMsgBody.body()!");
        return(false);
    }
}

bool CmdLocateData::RedisOnly(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemOperate& oMemOperate)
{
	std::string strSectionFactor;
	if (!m_pProxySess->GetSectionFactor(oMemOperate.redis_operate().data_purpose(),oMemOperate.section_factor(),strSectionFactor))
	{
		LOG4_ERROR("%d: %s oMemOperate(%s)", ERR_LACK_CLUSTER_INFO,m_pProxySess->m_pErrBuff,oMemOperate.DebugString().c_str());
		Response(stMsgShell, oInMsgHead, ERR_LACK_CLUSTER_INFO, m_pProxySess->m_pErrBuff);
		return(false);
	}
	SessionRedisNode* pRedisNodeSession = (SessionRedisNode*)net::GetSession(strSectionFactor, "net::SessionRedisNode");
	if (pRedisNodeSession)
	{
		std::string strMasterIdentify, strSlaveIdentify;
		const std::string& strHashKey = oMemOperate.redis_operate().hash_key().size()?oMemOperate.redis_operate().hash_key():oMemOperate.redis_operate().key_name();
		pRedisNodeSession->GetRedisNode(strHashKey, strMasterIdentify, strSlaveIdentify);
		util::CJsonObject oRspJson;
		oRspJson.Add("code", ERR_OK);
		oRspJson.Add("msg", "successfully");
		oRspJson.Add("redis_node", util::CJsonObject("{}"));
		oRspJson["redis_node"].Add("master", strMasterIdentify);
		oRspJson["redis_node"].Add("slave", strSlaveIdentify);
		return GetLabor()->SendToClient(stMsgShell, oInMsgHead, oRspJson.ToString());
	}
	LOG4_ERROR("GetSession error! strSectionFactor(%s)", strSectionFactor.c_str());
	Response(stMsgShell, oInMsgHead, ERR_LACK_CLUSTER_INFO, "GetSession error!");
	return(false);
}

bool CmdLocateData::DbOnly(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const MsgBody& oInMsgBody)
{
	auto callback = [](const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data,net::Step*pStep)
	{
		pStep->SendToClient(oInMsgBody.body());
	};
	return GetLabor()->SendToCallback(new net::DataStep(stMsgShell,oInMsgHead),oInMsgHead.cmd(),oInMsgBody.body(),callback,AGENT_R);
}

bool CmdLocateData::RedisAndDb(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const MsgBody& oInMsgBody)
{
    DataMem::MemOperate oMemOperate;
    oMemOperate.ParseFromString(oInMsgBody.body());
    std::string strSectionFactor;
	if (!m_pProxySess->GetSectionFactor(oMemOperate.redis_operate().data_purpose(),oMemOperate.section_factor(),strSectionFactor))
	{
		LOG4_ERROR("%d: %s oMemOperate(%s)", ERR_LACK_CLUSTER_INFO, m_pProxySess->m_pErrBuff,oMemOperate.DebugString().c_str());
		Response(stMsgShell, oInMsgHead, ERR_LACK_CLUSTER_INFO, m_pProxySess->m_pErrBuff);
		return(false);
	}
	SessionRedisNode* pRedisNodeSession = (SessionRedisNode*)net::GetSession(strSectionFactor, "net::SessionRedisNode");
	std::string strMasterIdentify;
	std::string strSlaveIdentify;
	const std::string& strHashKey = oMemOperate.redis_operate().hash_key().size() ? oMemOperate.redis_operate().hash_key():oMemOperate.redis_operate().key_name();
	pRedisNodeSession->GetRedisNode(strHashKey, strMasterIdentify, strSlaveIdentify);
	util::CJsonObject oRedisNodeJson;
	oRedisNodeJson.Add("master", strMasterIdentify);
	oRedisNodeJson.Add("slave", strSlaveIdentify);
	struct DataParam:net::StepParam
	{
		DataParam(const util::CJsonObject& oJson):oRedisNodeJson(oJson){}
		util::CJsonObject oRedisNodeJson;
	};
	auto callback = [](const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data,net::Step*pStep)
	{
		net::DataStep* pDataStep = (net::DataStep*)pStep;
		DataParam* pParam = (DataParam*)pDataStep->GetData();
		util::CJsonObject oRspJson;
		if (!oRspJson.Parse(oInMsgBody.body()))
		{
			LOG4_ERROR("oRspJson.Parse failed!");
		}
		oRspJson.Add("redis_node", pParam->oRedisNodeJson);
		pStep->SendToClient(oRspJson.ToString());
	};
	return GetLabor()->SendToCallback(new net::DataStep(stMsgShell,oInMsgHead,new DataParam(oRedisNodeJson)),oInMsgHead.cmd(),oInMsgBody.body(),callback,AGENT_R);
}

void CmdLocateData::Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,int iErrno, const std::string& strErrMsg)
{
    LOG4_DEBUG("%d: %s", iErrno, strErrMsg.c_str());
    util::CJsonObject oRspJson;
    oRspJson.Add("code", iErrno);
    oRspJson.Add("msg", strErrMsg);
    GetLabor()->SendToClient(stMsgShell,oInMsgHead,oRspJson.ToString());
}

} /* namespace core */
