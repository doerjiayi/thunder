/*******************************************************************************
 * Project:  DbAgent
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

CmdLocateData::CmdLocateData()
{
}

CmdLocateData::~CmdLocateData()
{
}

bool CmdLocateData::Init()
{
	pDbAgentSession = GetDbAgentSession();
	if (!pDbAgentSession)
	{
		LOG4_ERROR("GetDbAgentSession error!");
		return false;
	}
	net::GetCustomConf().Get("sync",pDbAgentSession->m_uiSync);
	if (pDbAgentSession->m_uiSync) LOG4_TRACE("sync db connection");
	else LOG4_TRACE("async db connection");
    return(true);
}

bool CmdLocateData::AnyMessage(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    DataMem::MemOperate oQuery;
    if (!oQuery.ParseFromString(oInMsgBody.body()))
    {
        LOG4_ERROR("DataMem::MemOperate ParseFromString(oInMsgBody.body()) error!");
        Response(stMsgShell, oInMsgHead, ERR_PARASE_PROTOBUF, "DataMem::MemOperate ParseFromString(oInMsgBody.body()) error!");
        return(false);
    }
    util::CJsonObject dbInstanceConf;
    std::string strInstance;
    if (!pDbAgentSession->LocateDbConn(oQuery,strInstance,dbInstanceConf))
    {
    	Response(stMsgShell, oInMsgHead, ERR_LACK_CLUSTER_INFO, pDbAgentSession->m_sErrMsg);
		return(false);
    }
    std::string strTableName = pDbAgentSession->GetFullTableName(oQuery.db_operate().table_name(), oQuery.db_operate().mod_factor());
	if (strTableName.empty())
	{
		LOG4_ERROR("dbname_table is NULL");
		Response(stMsgShell, oInMsgHead, ERR_LACK_CLUSTER_INFO, "dbname_table is NULL");
		return false;
	}
    util::CJsonObject oRspJson;
	oRspJson.Add("code", ERR_OK);
	oRspJson.Add("msg", "successfully");
	oRspJson.Add("db_node", util::CJsonObject("{}"));
	oRspJson["db_node"].Add(strInstance, dbInstanceConf);
	oRspJson["db_node"].Add("table_name", strTableName);
	return GetLabor()->SendToClient(stMsgShell, oInMsgHead, oRspJson.ToFormattedString());
}

bool CmdLocateData::Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,int iErrno, const std::string& strErrMsg)
{
    LOG4_DEBUG("%d: %s", iErrno, strErrMsg.c_str());
    util::CJsonObject oRspJson;
    oRspJson.Add("code", iErrno);
    oRspJson.Add("msg", strErrMsg);
    return GetLabor()->SendToClient(stMsgShell, oInMsgHead, oRspJson.ToFormattedString());
}

} /* namespace core */
