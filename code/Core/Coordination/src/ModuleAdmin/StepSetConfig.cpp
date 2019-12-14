/*******************************************************************************
 * Project:  Beacon
 * @file     StepSetConfig.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019-04-04
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepSetConfig.hpp"

namespace coor
{

StepSetConfig::StepSetConfig(
		SessionOnlineNodes* pSessionOnlineNodes,
		const net::tagMsgShell& stMsgShell,
        int32 iHttpMajor,
        int32 iHttpMinor,
        int32 iCmd,
        const std::string& strNodeType,
        const std::string& strNodeIdentify,
        const std::string& strConfigFileContent,
        const std::string& strConfigFileRelativePath,
        const std::string& strConfigFileName)
    : m_pSessionOnlineNodes(pSessionOnlineNodes), m_stMsgShell(stMsgShell),
      m_iEmitNum(0), m_iSetResultCode(0),
      m_iHttpMajor(iHttpMajor), m_iHttpMinor(iHttpMinor),
      m_iCmd(iCmd),
      m_strNodeType(strNodeType),
      m_strNodeIdentify(strNodeIdentify),
      m_strConfigFileContent(strConfigFileContent),
      m_strConfigFileRelativePath(strConfigFileRelativePath),
      m_strConfigFileName(strConfigFileName)
{
    m_oSetResultMsg.Parse("{}");
}

StepSetConfig::~StepSetConfig()
{
}

net::E_CMD_STATUS StepSetConfig::Emit(int iErrno, const std::string& strErrMsg, void* data)
{
    MsgBody oMsgBody;
    neb::ConfigInfo oConfigFileInfo;
    int iDecodeLen = Base64decode_len(m_strConfigFileContent.c_str());
    char* pBufPlain = (char*)malloc(iDecodeLen);
    int iDecodeBytes = Base64decode(pBufPlain, m_strConfigFileContent.c_str());
    oConfigFileInfo.set_file_name(m_strConfigFileName);
    oConfigFileInfo.set_file_content(pBufPlain, iDecodeBytes);
    oConfigFileInfo.set_file_path(m_strConfigFileRelativePath);
    oMsgBody.set_body(oConfigFileInfo.SerializeAsString());
    free(pBufPlain);

    HttpMsg oHttpMsg;
    oHttpMsg.set_type(HTTP_RESPONSE);
    oHttpMsg.set_status_code(200);
    oHttpMsg.set_http_major(m_iHttpMajor);
    oHttpMsg.set_http_minor(m_iHttpMinor);
    util::CJsonObject oResponseData;
    if (m_strNodeIdentify.size() > 0)
    {
        if (net::SendTo(m_strNodeIdentify, m_iCmd, GetSequence(), oMsgBody))
        {
            ++m_iEmitNum;
            return(net::STATUS_CMD_RUNNING);
        }
        else
        {
            oResponseData.Add("code", ERR_NODE_IDENTIFY);
            oResponseData.Add("msg", "unknow identify \"" + m_strNodeIdentify + "\"!");
            oHttpMsg.set_body(oResponseData.ToFormattedString());
            SendTo(m_stMsgShell, oHttpMsg);
            return(net::STATUS_CMD_FAULT);
        }
    }
    else if (m_strNodeType.size() > 0)
    {
        std::vector<std::string> vecNodes;
        if (m_pSessionOnlineNodes->GetOnlineNode(m_strNodeType, vecNodes))
        {
            for (auto it = vecNodes.begin(); it != vecNodes.end(); ++it)
            {
                if (net::SendTo(*it, m_iCmd, GetSequence(), oMsgBody))
                {
                    ++m_iEmitNum;
                }
                else
                {
                    m_iSetResultCode |= ERR_NODE_IDENTIFY;
                    m_oSetResultMsg.Add(*it, "connect failed");
                }
            }
            if (m_iEmitNum == 0)
            {
                oResponseData.Add("code", ERR_INVALID_ARGV);
                oResponseData.Add("msg", "failed");
                oResponseData.Add("data", m_oSetResultMsg);
                oHttpMsg.set_body(oResponseData.ToFormattedString());
                net::SendTo(m_stMsgShell, oHttpMsg);
                return(net::STATUS_CMD_FAULT);
            }
            return(net::STATUS_CMD_RUNNING);
        }
        else
        {
            oResponseData.Add("code", ERR_INVALID_ARGV);
            oResponseData.Add("msg", "There are no nodes of this type!");
            oHttpMsg.set_body(oResponseData.ToFormattedString());
            net::SendTo(m_stMsgShell, oHttpMsg);
            return(net::STATUS_CMD_FAULT);
        }
    }
    else
    {
        oResponseData.Add("code", ERR_INVALID_ARGV);
        oResponseData.Add("msg", "miss node type!");
        oHttpMsg.set_body(oResponseData.ToFormattedString());
        SendTo(m_stMsgShell, oHttpMsg);
        return(net::STATUS_CMD_FAULT);
    }
}

net::E_CMD_STATUS StepSetConfig::Callback(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, void* data)
{
    --m_iEmitNum;
    m_iSetResultCode |= oInMsgBody.rsp_result().code();
    m_oSetResultMsg.Add(net::GetConnectIdentify(stMsgShell), oInMsgBody.rsp_result().msg());
    if (0 == m_iEmitNum)
    {
        HttpMsg oHttpMsg;
        oHttpMsg.set_type(HTTP_RESPONSE);
        oHttpMsg.set_status_code(200);
        oHttpMsg.set_http_major(m_iHttpMajor);
        oHttpMsg.set_http_minor(m_iHttpMinor);
        util::CJsonObject oResponseData;
        if (net::ERR_OK == m_iSetResultCode)
        {
            oResponseData.Add("code", net::ERR_OK);
            oResponseData.Add("msg", "success");
        }
        else
        {
            oResponseData.Add("code", m_iSetResultCode);
            oResponseData.Add("msg", "failed");
            oResponseData.Add("data", m_oSetResultMsg);
        }
        oHttpMsg.set_body(oResponseData.ToFormattedString());
        SendTo(m_stMsgShell, oHttpMsg);
        return(net::STATUS_CMD_COMPLETED);
    }
    else
    {
        return(net::STATUS_CMD_RUNNING);
    }
}

net::E_CMD_STATUS StepSetConfig::Timeout()
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
    net::SendTo(m_stMsgShell, oHttpMsg);
    return(net::STATUS_CMD_FAULT);
}

} /* namespace coor */
