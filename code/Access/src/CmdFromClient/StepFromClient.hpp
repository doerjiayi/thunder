/*******************************************************************************
 * Project:  AccessServer
 * @file     StepFromClient.hpp
 * @brief 
 * @author   lbh
 * @date:    2019年10月21日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDFROMCLIENT_STEPFROMCLIENT_HPP_
#define SRC_CMDFROMCLIENT_STEPFROMCLIENT_HPP_

#include "ImError.h"
#include "step/Step.hpp"

namespace im
{

class StepFromClient: public net::Step
{
public:
    StepFromClient(const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
    virtual ~StepFromClient();
    virtual net::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    virtual net::E_CMD_STATUS Callback(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data = NULL);
    virtual net::E_CMD_STATUS Timeout();
};

} /* namespace im */

#endif /* SRC_CMDFROMCLIENT_STEPFROMCLIENT_HPP_ */
