/*******************************************************************************
 * Project:  Beacon
 * @file     StepGetConfig.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019-04-02
 * @note
 * Modify history:
 ******************************************************************************/
#include <util/encrypt/base64.h>
#include "StepGetConfig.hpp"

namespace coor
{

StepGetConfig::StepGetConfig(
        const net::tagMsgShell& stMsgShell,
        int32 iHttpMajor,
        int32 iHttpMinor,
        int32 iCmd,
        const std::string& strNodeIdentify,
        const std::string& strConfigFileRelativePath,
        const std::string& strConfigFileName)
    : m_tagMsgShell(stMsgShell),
      m_iHttpMajor(iHttpMajor), m_iHttpMinor(iHttpMinor),
      m_iCmd(iCmd), m_strNodeIdentify(strNodeIdentify),
      m_strConfigFileRelativePath(strConfigFileRelativePath),
      m_strConfigFileName(strConfigFileName)
{
}

StepGetConfig::~StepGetConfig()
{
}

net::E_CMD_STATUS StepGetConfig::Emit(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "")
{
    MsgBody oMsgBody;
    ConfigInfo oConfigFileInfo;
    oConfigFileInfo.set_file_name(m_strConfigFileName);
    oConfigFileInfo.set_file_path(m_strConfigFileRelativePath);
    oMsgBody.set_body(oConfigFileInfo.SerializeAsString());
    if (net::SendTo(m_strNodeIdentify, m_iCmd, GetSequence(), oMsgBody))
    {
        return(net::STATUS_CMD_RUNNING);
    }
    else
    {
        HttpMsg oHttpMsg;
        oHttpMsg.set_type(HTTP_RESPONSE);
        oHttpMsg.set_status_code(200);
        oHttpMsg.set_http_major(m_iHttpMajor);
        oHttpMsg.set_http_minor(m_iHttpMinor);
        util::CJsonObject oResponseData;
        oResponseData.Add("code", net::ERR_CONNECTION);
        oResponseData.Add("msg", "unknow identify \"" + m_strNodeIdentify + "\"!");
        oHttpMsg.set_body(oResponseData.ToFormattedString());
        SendTo(m_tagMsgShell, oHttpMsg);
        return(net::STATUS_CMD_FAULT);
    }
}

net::E_CMD_STATUS StepGetConfig::Callback(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, void* data)
{
    HttpMsg oHttpMsg;
    oHttpMsg.set_type(HTTP_RESPONSE);
    oHttpMsg.set_status_code(200);
    oHttpMsg.set_http_major(m_iHttpMajor);
    oHttpMsg.set_http_minor(m_iHttpMinor);
    util::CJsonObject oResponseData;
    oResponseData.Add("code", oInMsgBody.rsp_result().code());
    oResponseData.Add("msg", oInMsgBody.rsp_result().msg());
    if (net::ERR_OK == oInMsgBody.rsp_result().code())
    {
        ConfigInfo oConfigInfo;
        if (oConfigInfo.ParseFromString(oInMsgBody.body()))
        {
            int iEncodeLen = Base64encode_len(oConfigInfo.file_content().size());
            char* pBufEncoded = (char*)malloc(iEncodeLen);
            int iEncodeBytes = Base64encode(pBufEncoded, oConfigInfo.file_content().c_str(), oConfigInfo.file_content().size());
            std::string strFileContent;
            strFileContent.assign(pBufEncoded, iEncodeBytes);
            free(pBufEncoded);
            oResponseData.AddEmptySubObject("data");
            oResponseData["data"].Add("file_content", strFileContent);
        }
    }
    oHttpMsg.set_body(oResponseData.ToFormattedString());
    SendTo(m_tagMsgShell, oHttpMsg);
    return(net::STATUS_CMD_COMPLETED);
}

net::E_CMD_STATUS StepGetConfig::Timeout()
{
    HttpMsg oHttpMsg;
    oHttpMsg.set_type(HTTP_RESPONSE);
    oHttpMsg.set_status_code(200);
    oHttpMsg.set_http_major(m_iHttpMajor);
    oHttpMsg.set_http_minor(m_iHttpMinor);
    util::CJsonObject oResponseData;
    oResponseData.Add("code", net::ERR_TIMEOUT);
    oResponseData.Add("msg", "get config file timeout!");
    oHttpMsg.set_body(oResponseData.ToFormattedString());
    SendTo(m_tagMsgShell, oHttpMsg);
    return(net::STATUS_CMD_FAULT);
}

} /* namespace coor */
