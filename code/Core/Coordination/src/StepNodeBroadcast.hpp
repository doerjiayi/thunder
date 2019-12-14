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

#include "Comm.hpp"

namespace coor
{

class StepNodeBroadcast: public net::Step
{
public:
public:
    StepNodeBroadcast(const std::string& strNodeIdentity, int32 iCmd, const std::string& strBody);
    virtual ~StepNodeBroadcast();

    virtual net::E_CMD_STATUS Emit(
    		int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");

    virtual net::E_CMD_STATUS Callback(
            const net::tagMsgShell& stMsgShell,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody,
            void* data = NULL);

    virtual net::E_CMD_STATUS Timeout();

private:
    std::string m_strTargetNodeIdentity;
    int32 m_iCmd;
    std::string m_strBody;
};

} /* namespace coor */

#endif /* SRC_CMDREGISTER_STEPNODEBROADCAST_HPP_ */
