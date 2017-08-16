/*******************************************************************************
 * Project:  HelloThunder
 * @file     StepHello.hpp
 * @brief 
 * @author   chenjiayi
 * @date:    2017年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef STEPHELLO_HPP_
#define STEPHELLO_HPP_
#include "step/Step.hpp"
#include "step/HttpStep.hpp"
#include "SessionHello.hpp"

namespace hello
{

class StepHello: public thunder::HttpStep
{
public:
    StepHello(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
    virtual ~StepHello();

    virtual thunder::E_CMD_STATUS Emit(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    virtual thunder::E_CMD_STATUS Callback(
                    const thunder::MsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data = NULL);
    virtual thunder::E_CMD_STATUS Callback(
                        const thunder::MsgShell& stMsgShell,
                        const HttpMsg& oHttpMsg,
                        void* data = NULL);
    virtual thunder::E_CMD_STATUS Timeout();
private:
    HttpMsg m_oInHttpMsg;
};

} /* namespace hello */

#endif /* STEPHELLO_HPP_ */
