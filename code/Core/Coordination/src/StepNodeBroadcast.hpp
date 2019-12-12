/*******************************************************************************
 * Project:  Beacon
 * @file     StepNodeBroadcast.hpp
 * @brief
 * @author   bwar
 * @date:    Dec 28, 2016
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDREGISTER_STEPNODEBROADCAST_HPP_
#define SRC_CMDREGISTER_STEPNODEBROADCAST_HPP_

#include "Step.hpp"

namespace coor
{

class StepNodeBroadcast: public net::Step
{
public:
public:
    StepNodeBroadcast(const std::string& strNodeIdentity, int32 iCmd, const MsgBody& oMsgBody);
    virtual ~StepNodeBroadcast();

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
    std::string m_strTargetNodeIdentity;
    int32 m_iCmd;
    MsgBody m_oMsgBody;
};

} /* namespace coor */

#endif /* SRC_CMDREGISTER_STEPNODEBROADCAST_HPP_ */
