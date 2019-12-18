/*******************************************************************************
 * Project:  Center
 * @file     StepSetConfig.hpp
 * @brief
 * @author   Bwar
 * @date:    2019-04-04
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDREGISTER_STEPSETCONFIG_HPP_
#define SRC_CMDREGISTER_STEPSETCONFIG_HPP_

#include "Comm.hpp"
#include "CenterError.hpp"
#include "SessionOnlineNodes.hpp"

namespace coor
{

class StepSetConfig: public net::Step
{
public:
    StepSetConfig(
    		SessionOnlineNodes* pSessionOnlineNodes,
            const net::tagMsgShell& stMsgShell,
            int32 iHttpMajor,
            int32 iHttpMinor,
            int32 iCmd,
            const std::string& strNodeType,
            const std::string& strNodeIdentify,
            const std::string& strConfigFileContent,
            const std::string& strConfigFileRelativePath,
            const std::string& strConfigFileName);
    virtual ~StepSetConfig();

    virtual net::E_CMD_STATUS Emit(
    		int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");

    virtual net::E_CMD_STATUS Callback(
            const net::tagMsgShell& stMsgShell,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody,
            void* data = NULL);

    virtual net::E_CMD_STATUS Timeout();

private:
    SessionOnlineNodes* m_pSessionOnlineNodes ;//= nullptr;
    net::tagMsgShell m_stMsgShell;
    int32 m_iEmitNum;
    int32 m_iSetResultCode;
    int32 m_iHttpMajor;
    int32 m_iHttpMinor;
    int32 m_iCmd;
    std::string m_strNodeType;
    std::string m_strNodeIdentify;
    std::string m_strConfigFileContent;
    std::string m_strConfigFileRelativePath;
    std::string m_strConfigFileName;
    util::CJsonObject m_oSetResultMsg;
};

} /* namespace coor */

#endif /* SRC_CMDREGISTER_STEPSETCONFIG_HPP_ */
