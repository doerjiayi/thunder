/*******************************************************************************
 * Project:  CenterServer
 * @file     CmdNodeReport.cpp
 * @brief 
 * @author   cjy
 * @date:    2019年8月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include <iostream>
#include "CmdNodeReport.hpp"
#include "util/json/CJsonObject.hpp"

MUDULE_CREATE(core::CmdNodeReport);

namespace core
{

CmdNodeReport::CmdNodeReport()
                : pSess(NULL),boInit(false)
{
}

CmdNodeReport::~CmdNodeReport()
{
}

bool CmdNodeReport::Init()
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

bool CmdNodeReport::AnyMessage(const net::tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    //上报信息
    util::CJsonObject jParseObj;
    if (!jParseObj.Parse(oInMsgBody.body()))
    {
        LOG4_ERROR("error jsonParse error! json[%s]",
                                    oInMsgBody.body().c_str());
        return Response(stMsgShell,oInMsgHead,ERR_BODY_JSON);
    }
    LOG4_DEBUG("CmdNodeReport jsonbuf[%s] Parse is ok",oInMsgBody.body().c_str());
    //解析节点数据
    NodeStatusInfo nodeinfo;
    if (!nodeinfo.pareJsonData(jParseObj))
    {
        LOG4_ERROR("nodeinfo.pareJsonDat error jsonbuf[%s] Parse is ok",
                        oInMsgBody.body().c_str());
        return Response(stMsgShell,oInMsgHead,ERR_BODY_JSON);
    }
    if (!pSess->CheckNodeStatus(nodeinfo))
    {
        LOG4_ERROR("NodeRegMgr CheckNodeStatus error.nodeType(%s)",
                        nodeinfo.nodeType.c_str());
        return Response(stMsgShell,oInMsgHead,ERR_SERVERINFO);
    }
    LOG4_DEBUG("report node(%s,%s) connect:%d,client:%d",
                    nodeinfo.nodeInnerIp.c_str(),
                    nodeinfo.nodeType.c_str(), nodeinfo.connect,
                    nodeinfo.client);
    int iRet = pSess->UpdateNode(stMsgShell, oInMsgHead,
                    oInMsgBody, nodeinfo);
    if (iRet > 0)
    {
        LOG4_ERROR("CmdNodeReport UpdateNode msg jsonbuf[%s] is wrong,error code(%d)",
                        oInMsgBody.body().c_str(), iRet);
    }
    return Response(stMsgShell,oInMsgHead,iRet);
}

bool CmdNodeReport::Response(const net::tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,int iRet)
{
    util::CJsonObject jObjReturn;
    jObjReturn.Add("errcode", iRet);
    return net::SendToClient(stMsgShell, oInMsgHead, jObjReturn.ToString());
}


} /* namespace core */
