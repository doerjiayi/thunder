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
#include "step/StepCo.hpp"

namespace core
{

class StepHttpRequestState: public net::StepState
{
public:
	StepHttpRequestState(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
    virtual ~StepHttpRequestState();
    net::E_CMD_STATUS State0();//回调成员函数调用不带自定义参数（参数在step成员变量中实现）
    net::E_CMD_STATUS State1();
    net::E_CMD_STATUS State2();
    net::E_CMD_STATUS State3();
    net::E_CMD_STATUS State4();

    void Response(int nCode);
    HttpMsg m_oInHttpMsg;
    uint32 m_uiTestVal;
};

} /* namespace hello */

#endif /* SRC_CMDHELLO_STEPHTTPREQUEST_HPP_ */
