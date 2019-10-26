/*******************************************************************************
 * Project:  Hello
 * @file     StepLocateData.hpp
 * @brief 
 * @author   cjy
 * @date:    2016年4月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_MODULELOCATEDATA_STEPLOCATEDATA_HPP_
#define SRC_MODULELOCATEDATA_STEPLOCATEDATA_HPP_
#include "../HelloSession.h"
#include "step/Step.hpp"
#include "step/HttpStep.hpp"
#include "storage/MemOperator.hpp"

namespace core
{

class StepLocateData: public net::Step
{
public:
    StepLocateData(const net::tagMsgShell& stInMsgShell, const HttpMsg& oInHttpMsg);
    virtual ~StepLocateData();

    virtual net::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg = "", const std::string& strErrClientShow = "");

    virtual net::E_CMD_STATUS Callback(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data = NULL);

    virtual net::E_CMD_STATUS Timeout();

protected:
    bool Response(int iErrno, const std::string& strErrMsg, const std::string& strErrClientShow);

private:
    net::tagMsgShell m_stInMsgShell;
    HttpMsg m_oInHttpMsg;
};

} /* namespace net */

#endif /* SRC_MODULELOCATEDATA_STEPLOCATEDATA_HPP_ */
