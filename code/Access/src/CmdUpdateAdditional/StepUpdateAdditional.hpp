/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepUpdateAdditional.hpp
 * @brief    更新Additional通知
 * @author   ty
 * @date:    2019年9月17日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_UPDATE_ADDITIONAL_HPP_
#define SRC_STEP_UPDATE_ADDITIONAL_HPP_
#include "protocol/oss_sys.pb.h"
#include "step/Step.hpp"
#include "RobotError.h"
#include "user_basic.pb.h"
namespace im
{
class StepUpdateAdditional : public net::Step
{
public:
    StepUpdateAdditional(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody);
    virtual ~StepUpdateAdditional();

    virtual net::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    virtual net::E_CMD_STATUS Callback(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data = NULL);
    virtual net::E_CMD_STATUS Timeout();
private:
	int m_iTimeoutNum;          ///< 超时次数
	net::tagMsgShell m_stMsgShell;
	MsgHead m_oInMsgHead;
	MsgBody m_oInMsgBody;
	MsgHead m_oOutMsgHead;
	MsgBody m_oOutMsgBody;
	user_basic m_basicinfo;
};

}
/* namespace im */

#endif /* SRC_STEP_UPDATE_ADDITIONAL_HPP_ */
