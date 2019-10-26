/*******************************************************************************
 * Project:  DataProxy
 * @file     StepSyncToDb.hpp
 * @brief 
 * @author   cjy
 * @date:    2016年7月21日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDDATAPROXY_STEPSYNCTODB_HPP_
#define SRC_CMDDATAPROXY_STEPSYNCTODB_HPP_

#include <string>
#include "step/Step.hpp"

namespace core
{

class StepSyncToDb: public net::Step
{
public:
    StepSyncToDb(const std::string& strSessionId, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
    virtual ~StepSyncToDb();
    virtual net::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    virtual net::E_CMD_STATUS Callback(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data = NULL);
    virtual net::E_CMD_STATUS Timeout();
private:
    std::string m_strSessionId;
    MsgHead m_oMsgHead;
    MsgBody m_oMsgBody;
    uint32 m_timeOut;
};

} /* namespace core */

#endif /* SRC_CMDDATAPROXY_STEPSYNCTODB_HPP_ */
