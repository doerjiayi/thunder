/*******************************************************************************
 * Project:  DataProxyServer
 * @file     StorageStep.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年3月21日
 * @note
 * Modify history:
 ******************************************************************************/
#include "RedisStorageStep.hpp"

namespace core
{

RedisStorageStep::RedisStorageStep(Step* pNextStep)
    : net::RedisStep(pNextStep)
{
}

RedisStorageStep::RedisStorageStep(const net::tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, Step* pNextStep)
    : net::RedisStep(stReqMsgShell, oReqMsgHead, oReqMsgBody, pNextStep)
{
}

RedisStorageStep::~RedisStorageStep()
{
}

bool RedisStorageStep::Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,int iErrno, const std::string& strErrMsg)
{
    LOG4_TRACE("%d: %s", iErrno, strErrMsg.c_str());
    DataMem::MemRsp oRsp;
    oRsp.set_err_no(iErrno);
    oRsp.set_err_msg(strErrMsg);
    return GetLabor()->SendToClient(stMsgShell,oInMsgHead,oRsp);
}

bool RedisStorageStep::Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemRsp& oRsp)
{
    LOG4_TRACE("%d: %s", oRsp.err_no(), oRsp.err_msg().c_str());
    if (0 == oRsp.err_no())
    {
        LOG4_DEBUG("oRsp:%s",oRsp.DebugString().c_str());
    }
    return GetLabor()->SendToClient(stMsgShell,oInMsgHead,oRsp);
}

} /* namespace core */
