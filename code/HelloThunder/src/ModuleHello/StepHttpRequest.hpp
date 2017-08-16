/*******************************************************************************
 * Project:  HelloThunder
 * @file     StepHttpRequest.hpp
 * @brief 
 * @author   chenjiayi
 * @date:    2017年11月3日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDHELLO_STEPHTTPREQUEST_HPP_
#define SRC_CMDHELLO_STEPHTTPREQUEST_HPP_
#include "step/HttpStep.hpp"

namespace hello
{

class StepHttpRequest: public thunder::HttpStep
{
public:
    StepHttpRequest(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
    virtual ~StepHttpRequest();

    virtual thunder::E_CMD_STATUS Emit(
        		int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    virtual thunder::E_CMD_STATUS Callback(
                    const thunder::MsgShell& stMsgShell,
                    const HttpMsg& oHttpMsg,
                    void* data = NULL);
    /**
     * @brief 步骤超时回调
     */
    virtual thunder::E_CMD_STATUS Timeout();
    HttpMsg m_oInHttpMsg;
};

} /* namespace hello */

#endif /* SRC_CMDHELLO_STEPHTTPREQUEST_HPP_ */
