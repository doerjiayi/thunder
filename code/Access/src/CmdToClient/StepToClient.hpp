/*******************************************************************************
 * Project:  AccessServer
 * @file     StepToClient.hpp
 * @brief 
 * @author   lbh
 * @date:    2019年10月21日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDTOCLIENT_STEPTOCLIENT_HPP_
#define SRC_CMDTOCLIENT_STEPTOCLIENT_HPP_

#include "RobotError.h"
#include "step/Step.hpp"

namespace im
{

class StepToClient: public net::Step
{
public:
    StepToClient(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
    virtual ~StepToClient();

    virtual net::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    virtual net::E_CMD_STATUS Callback(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data = NULL);
    virtual net::E_CMD_STATUS Timeout();
};

} /* namespace im */

#endif /* SRC_CMDTOCLIENT_STEPTOCLIENT_HPP_ */
