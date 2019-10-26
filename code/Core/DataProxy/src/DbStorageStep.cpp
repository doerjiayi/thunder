/*******************************************************************************
 * Project:  DataProxyServer
 * @file     DbStorageStep.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年3月21日
 * @note
 * Modify history:
 ******************************************************************************/
#include "DbStorageStep.hpp"

namespace core
{

DbStorageStep::DbStorageStep(Step* pNextStep)
    : Step(pNextStep)
{
}

DbStorageStep::DbStorageStep(const net::tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, Step* pNextStep)
    : Step(stReqMsgShell, oReqMsgHead, oReqMsgBody, pNextStep)
{
}

DbStorageStep::~DbStorageStep()
{
}

bool DbStorageStep::Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,int iErrno, const std::string& strErrMsg)
{
    LOG4_TRACE("error %d: %s", iErrno, strErrMsg.c_str());
    DataMem::MemRsp oRsp;
    oRsp.set_err_no(iErrno);
    oRsp.set_err_msg(strErrMsg);
    return net::SendToClient(stMsgShell,oInMsgHead,oRsp);
}

bool DbStorageStep::Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemRsp& oRsp)
{
    LOG4_TRACE("%d: %s", oRsp.err_no(), oRsp.err_msg().c_str());
    return net::SendToClient(stMsgShell,oInMsgHead,oRsp);
}


} /* namespace core */
