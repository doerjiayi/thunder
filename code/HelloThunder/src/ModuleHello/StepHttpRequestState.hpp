/*******************************************************************************
 * Project:  HelloThunder
 * @file     StepHttpRequest.hpp
 * @brief 
 * @author   chenjiayi
 * @date:    2017年11月3日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDHELLO_StepHttpRequestState_HPP_
#define SRC_CMDHELLO_StepHttpRequestState_HPP_
#include "step/HttpStep.hpp"
#include "step/StepState.hpp"
#include "SessionHello.hpp"

namespace hello
{

class StepHttpRequestState: public thunder::StepState
{
public:
	StepHttpRequestState(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
    virtual ~StepHttpRequestState();
    virtual void InitState();
    virtual thunder::E_CMD_STATUS State0();
    virtual thunder::E_CMD_STATUS State1();
    virtual thunder::E_CMD_STATUS State2();
    virtual thunder::E_CMD_STATUS State3();
    HttpMsg m_oInHttpMsg;
};

} /* namespace hello */

#endif /* SRC_CMDHELLO_STEPHTTPREQUEST_HPP_ */
