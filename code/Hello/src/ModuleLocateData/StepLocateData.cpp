/*******************************************************************************
 * Project:  Hello
 * @file     StepLocateData.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年4月19日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepLocateData.hpp"
#include "util/HashCalc.hpp"

namespace core
{

StepLocateData::StepLocateData(const net::tagMsgShell& stInMsgShell, const HttpMsg& oInHttpMsg)
    : m_stInMsgShell(stInMsgShell), m_oInHttpMsg(oInHttpMsg)
{
}

StepLocateData::~StepLocateData()
{
}

net::E_CMD_STATUS StepLocateData::Emit(int iErrno, const std::string& strErrMsg, const std::string& strErrClientShow)
{
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    util::CJsonObject oRequest(m_oInHttpMsg.body());
    LOG4_DEBUG("%s", oRequest.ToString().c_str());
    uint32 uiFactor = 0;
    std::string strTableName;
    std::string strRedisKey;
    /*
    {
        "note":"factor or factor_string(only the factor_type was 3) should be set.",
        "factor":10100,
        "tb_name":"tb_userinfo",
        "redis_key":"1:3:10100"
    }
    */
    if (!oRequest.Get("factor", uiFactor))
    {
        uiFactor = util::CalcKeyHash(oRequest("factor_string").c_str(), oRequest("factor_string").size());
    }
    if (oRequest.Get("tb_name", strTableName) && oRequest.Get("redis_key", strRedisKey))
    {
        net::MemOperator oMemOper(uiFactor,
                        strTableName, DataMem::MemOperate::DbOperate::SELECT,
                        strRedisKey, "get");
        oMsgBody.set_body(oMemOper.MakeMemOperate()->SerializeAsString());
    }
    else if (oRequest.Get("tb_name", strTableName))
    {
        net::DbOperator oDbOper(uiFactor, strTableName, DataMem::MemOperate::DbOperate::SELECT);
        oMsgBody.set_body(oDbOper.MakeMemOperate()->SerializeAsString());
    }
    else if (oRequest.Get("redis_key", strRedisKey))
    {
        net::RedisOperator oRedisOper(uiFactor, strRedisKey, "get");
        oMsgBody.set_body(oRedisOper.MakeMemOperate()->SerializeAsString());
    }
    else
    {
        Response(ERR_LACK_CLUSTER_INFO, "tb_name or redis_key must be set!", "tb_name or redis_key must be set!");
        return(net::STATUS_CMD_FAULT);
    }
    oMsgHead.set_cmd(net::CMD_REQ_LOCATE_STORAGE);
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    oMsgHead.set_seq(GetSequence());
    LOG4_DEBUG("body: %s",oMsgBody.body().c_str());
    if (!GetLabor()->SendToNext("PROXY", oMsgHead, oMsgBody))
    {
        LOG4_ERROR("send to dataproxy error!");
        Response(ERR_SERVERINFO, "send to dataproxy error!", "send to dataproxy error!");
        return(net::STATUS_CMD_FAULT);
    }
    return(net::STATUS_CMD_RUNNING);
}

net::E_CMD_STATUS StepLocateData::Callback(
                const net::tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody,
                void* data)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    HttpMsg oHttpMsg;
    oHttpMsg.set_type(HTTP_RESPONSE);
    oHttpMsg.set_status_code(200);
    oHttpMsg.set_http_major(m_oInHttpMsg.http_major());
    oHttpMsg.set_http_minor(m_oInHttpMsg.http_minor());
    oHttpMsg.set_body(oInMsgBody.body());
    GetLabor()->SendTo(m_stInMsgShell, oHttpMsg);
    return(net::STATUS_CMD_COMPLETED);
}

net::E_CMD_STATUS StepLocateData::Timeout()
{
    LOG4_DEBUG("StepTestGet Timeout");
    HttpMsg oHttpMsg;
    oHttpMsg.set_type(HTTP_RESPONSE);
    oHttpMsg.set_status_code(200);
    oHttpMsg.set_http_major(m_oInHttpMsg.http_major());
    oHttpMsg.set_http_minor(m_oInHttpMsg.http_minor());
    oHttpMsg.set_body("timeout");
    GetLabor()->SendTo(m_stInMsgShell, oHttpMsg);
    return(net::STATUS_CMD_COMPLETED);
}

bool StepLocateData::Response(int iErrno, const std::string& strErrMsg, const std::string& strErrClientShow)
{
    HttpMsg oHttpMsg;
    util::CJsonObject oRsp;
    oHttpMsg.set_type(HTTP_RESPONSE);
    oHttpMsg.set_status_code(200);
    oHttpMsg.set_http_major(m_oInHttpMsg.http_major());
    oHttpMsg.set_http_minor(m_oInHttpMsg.http_minor());
    oRsp.Add("code", iErrno);
    oRsp.Add("msg", strErrClientShow.c_str());
    oHttpMsg.set_body(oRsp.ToFormattedString());
    GetLabor()->SendTo(m_stInMsgShell, oHttpMsg);
    return(net::STATUS_CMD_COMPLETED);
}

} /* namespace net */
