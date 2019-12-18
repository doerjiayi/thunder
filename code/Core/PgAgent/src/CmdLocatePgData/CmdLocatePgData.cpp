/*******************************************************************************
 * Project:  DbAgent
 * @file     CmdLocatePgData.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年4月18日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdLocatePgData.hpp"

MUDULE_CREATE(core::CmdLocatePgData);

namespace core
{

CmdLocatePgData::CmdLocatePgData():pDbAgentSession(NULL)
{
}

CmdLocatePgData::~CmdLocatePgData()
{
}

bool CmdLocatePgData::Init()
{
	pDbAgentSession = GetPgAgentSession();
	if (!pDbAgentSession)
	{
		LOG4_ERROR("GetPgAgentSession error!");
		return false;
	}
    return(true);
}

bool CmdLocatePgData::AnyMessage(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    DataMem::MemOperate oQuery;
    if (!oQuery.ParseFromString(oInMsgBody.body()))
    {
        LOG4_ERROR("DataMem::MemOperate ParseFromString(oInMsgBody.body()) error!");
        pDbAgentSession->Response(stMsgShell, oInMsgHead, ERR_PARASE_PROTOBUF, "DataMem::MemOperate ParseFromString(oInMsgBody.body()) error!");
        return(false);
    }
    int nCode(0);std::string strErrMsg;util::CJsonObject oRspJson;
    pDbAgentSession->LocatePgConfig(oQuery,nCode,strErrMsg,oRspJson);
    oRspJson.Add("code", nCode);
    oRspJson.Add("msg", strErrMsg);
	return GetLabor()->SendToClient(stMsgShell,oInMsgHead,oRspJson.ToString());
}


} /* namespace core */
