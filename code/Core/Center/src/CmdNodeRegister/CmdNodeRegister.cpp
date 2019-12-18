/*******************************************************************************
 * Project:  CenterServer
 * @file     CmdNodeRegister.cpp
 * @brief 
 * @author   cjy
 * @date:    2019年8月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include <iostream>
#include "util/json/CJsonObject.hpp"
#include "CmdNodeRegister.hpp"

MUDULE_CREATE(core::CmdNodeRegister);

namespace core
{

CmdNodeRegister::CmdNodeRegister()
                :pSess(NULL),boInit(false)
{
}
CmdNodeRegister::~CmdNodeRegister()
{
}

bool CmdNodeRegister::Init()
{
    if (boInit)
    {
        return true;
    }
    pSess = GetNodeSession(true);
    if(!pSess)
    {
        LOG4_ERROR("failed to get GetNodeSession");
        return false;
    }
    boInit = true;
    return true;
}

bool CmdNodeRegister::AnyMessage(const net::tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    util::CJsonObject jParseObj;
    if (!jParseObj.Parse(oInMsgBody.body()))
    {
        LOG4_DEBUG("failed to parse json body");
        return Response(stMsgShell,oInMsgHead,ERR_BODY_JSON);
    }
    LOG4_DEBUG("CmdNodeRegister jsonbuf[%s] Parse is ok",
                                oInMsgBody.body().c_str());
    //解析节点数据
    NodeStatusInfo nodeinfo;
    if (!nodeinfo.pareJsonData(jParseObj))
    {
        LOG4_ERROR("nodeinfo.pareJsonData error");
        return Response(stMsgShell,oInMsgHead,ERR_BODY_JSON);
    }
    if (!pSess->CheckNodeStatus(nodeinfo))
    {
        LOG4_ERROR("CheckNodeStatus error.nodeType(%s)",nodeinfo.nodeType.c_str());
        return Response(stMsgShell,oInMsgHead,ERR_SERVERINFO);
    }
    int regNodeRet = pSess->RegNode(stMsgShell, oInMsgHead,
                    oInMsgBody, nodeinfo);
    if (regNodeRet)//注册失败返回注册应答，否则已在注册函数中发送
    {
        LOG4_ERROR("CmdNodeRegister msg jsonbuf[%s] is wrong,error code(%d)",oInMsgBody.body().c_str(),regNodeRet);
        return Response(stMsgShell,oInMsgHead,ERR_SERVERINFO);
    }
    LOG4_DEBUG("nodeinfo.getNodeKey(%s),stMsgShell(%d,%u)",nodeinfo.getNodeKey().c_str(),stMsgShell.iFd,stMsgShell.ulSeq);
    return true;
}

bool CmdNodeRegister::Response(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,int iRet,int node_id)
{
    LOG4_DEBUG("CmdNodeRegister::Response iRet(%d),node_id(%d)",iRet,node_id);
    if(iRet)
    {
        util::CJsonObject jObjReturn;
        jObjReturn.Add("errcode", iRet);
        jObjReturn.Add("node_id", iRet ? 0 :node_id);
        return net::SendToClient(stMsgShell, oInMsgHead, jObjReturn.ToString());
    }
    return (iRet ? false : true);
}


} /* namespace core */
