/*******************************************************************************
 * Project:  DbAgent
 * @file     CmdPgOper.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年3月28日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdPgOper.hpp"
#include "PgAgentSession.h"

MUDULE_CREATE(core::CmdPgOper);

namespace core
{

CmdPgOper::CmdPgOper():pDbAgentSession(NULL)
{
}

CmdPgOper::~CmdPgOper()
{

}

bool CmdPgOper::Init()
{
	pDbAgentSession = GetPgAgentSession();
	if (!pDbAgentSession)
	{
		LOG4_ERROR("GetPgAgentSession error!");
		return false;
	}
    return(true);
}

bool CmdPgOper::AnyMessage(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    DataMem::MemOperate oMemOperate;
    if (!oMemOperate.ParseFromString(oInMsgBody.body()))
    {
        LOG4_ERROR("Parse protobuf msg error!");
        pDbAgentSession->Response(stMsgShell,oInMsgHead,ERR_PARASE_PROTOBUF, "DataMem::MemOperate ParseFromString() failed!");
        return(false);
    }
    DataMem::MemRsp oRsp;
    oRsp.set_from(DataMem::MemRsp::FROM_DB);
    pDbAgentSession->QueryOper(oRsp,oMemOperate);
	pDbAgentSession->Response(stMsgShell,oInMsgHead,oRsp);
    return(true);
}





} /* namespace core */
