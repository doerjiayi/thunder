/*******************************************************************************
 * Project:  Beacon
 * @file     StepGetConfig.hpp
 * @brief
 * @author   Bwar
 * @date:    2019-04-01
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDREGISTER_STEPGETCONFIG_HPP_
#define SRC_CMDREGISTER_STEPGETCONFIG_HPP_

#include <util/http/http_parser.h>
#include <util/json/CJsonObject.hpp>
#include "Step.hpp"

namespace coor
{

class StepGetConfig: public net::Step
{
public:
    StepGetConfig(
            const net::tagMsgShell& stMsgShell,
            int32 iHttpMajor,
            int32 iHttpMinor,
            int32 iCmd,
            const std::string& strNodeIdentify,
            const std::string& strConfigFileRelativePath,
            const std::string& strConfigFileName);
    virtual ~StepGetConfig();

    virtual net::E_CMD_STATUS Emit(
            int iErrno = 0,
            const std::string& strErrMsg = "",
            void* data = NULL);

    virtual net::E_CMD_STATUS Callback(
            const net::tagMsgShell& stMsgShell,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody,
            void* data = NULL);

    virtual net::E_CMD_STATUS Timeout();

private:
    net::tagMsgShell m_tagMsgShell;
    int32 m_iHttpMajor;
    int32 m_iHttpMinor;
    int32 m_iCmd;
    std::string m_strNodeIdentify;
    std::string m_strConfigFileRelativePath;
    std::string m_strConfigFileName;
};

} /* namespace coor */

#endif /* SRC_CMDREGISTER_STEPGETCONFIG_HPP_ */
