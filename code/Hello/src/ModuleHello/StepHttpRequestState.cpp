/*******************************************************************************
 * Project:  HelloThunder
 * @file     StepHttpRequest.cpp
 * @brief 
 * @author   chenjiayi
 * @date:    2017年11月3日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepHttpRequestState.hpp"

namespace core
{

StepHttpRequestState::StepHttpRequestState(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
	m_stReqMsgShell = stMsgShell;
	m_oInHttpMsg = oInHttpMsg;
	m_uiTestVal = 0;
	AddStateFunc((StepStateFunc)&StepHttpRequestState::State0);//回调成员函数调用不带自定义参数（参数在step成员变量中实现）
	AddStateFunc((StepStateFunc)&StepHttpRequestState::State1);
	AddStateFunc((StepStateFunc)&StepHttpRequestState::State2);
	AddStateFunc((StepStateFunc)&StepHttpRequestState::State3);
	AddStateFunc((StepStateFunc)&StepHttpRequestState::State4);
}

StepHttpRequestState::~StepHttpRequestState()
{
}

net::E_CMD_STATUS StepHttpRequestState::State0()
{
	LOG4_DEBUG("%s last state:%u uiTestVal:%u ResHttpMsg:%s",
				__FUNCTION__,GetLastState(),++m_uiTestVal,m_oResHttpMsg.DebugString().c_str());
	if (HttpGet("http://www.baidu.com/"))
	{
		return(net::STATUS_CMD_RUNNING);
	}
	else
	{
		LOG4_ERROR("HttpGet http://www.baidu.com/ error");
		Response(1);
		return(net::STATUS_CMD_FAULT);
	}
}

net::E_CMD_STATUS StepHttpRequestState::State1()
{
	LOG4_DEBUG("%s last state:%u uiTestVal:%u ResHttpMsg:%s",
			__FUNCTION__,GetLastState(),++m_uiTestVal,m_oResHttpMsg.DebugString().c_str());
	if (HttpGet("http://www.sogou.com/"))
	{
		SetNextState(4);// 0 -> 1 -> 4
		return(net::STATUS_CMD_RUNNING);
	}
	else
	{
		LOG4_ERROR("HttpGet http://www.sogou.com/ error");
		Response(1);
		return(net::STATUS_CMD_FAULT);
	}
}

net::E_CMD_STATUS StepHttpRequestState::State2()
{
	LOG4_DEBUG("%s last state:%u uiTestVal:%u ResHttpMsg:%s",
				__FUNCTION__,GetLastState(),++m_uiTestVal,m_oResHttpMsg.DebugString().c_str());
	if (HttpGet("http://www.alipay.com/"))
	{
		SetNextState(4);// 0 -> 1 -> 2 -> 4
		return(net::STATUS_CMD_RUNNING);
	}
	else
	{
		LOG4_ERROR("HttpGet http://www.alipay.com/ error");
		Response(1);
		return(net::STATUS_CMD_FAULT);
	}
}

net::E_CMD_STATUS StepHttpRequestState::State3()
{
	LOG4_DEBUG("%s last state:%u uiTestVal:%u ResHttpMsg:%s",
				__FUNCTION__,GetLastState(),++m_uiTestVal,m_oResHttpMsg.DebugString().c_str());
	LOG4_DEBUG("StepState done");
	Response(0);
	return(net::STATUS_CMD_COMPLETED);
}

net::E_CMD_STATUS StepHttpRequestState::State4()
{
	LOG4_DEBUG("%s last state:%u uiTestVal:%u ResHttpMsg:%s",
				__FUNCTION__,GetLastState(),++m_uiTestVal,m_oResHttpMsg.DebugString().c_str());
	LOG4_DEBUG("StepState done");
	Response(0);
	return(net::STATUS_CMD_COMPLETED);
}

void StepHttpRequestState::Response(int nCode)
{
    HttpMsg oHttpMsg;
    util::CJsonObject oRsp;
    oHttpMsg.set_type(HTTP_RESPONSE);
    oHttpMsg.set_status_code(200);
    oHttpMsg.set_http_major(m_oInHttpMsg.http_major());
    oHttpMsg.set_http_minor(m_oInHttpMsg.http_minor());
    oRsp.Add("code", nCode);
    oRsp.Add("msg", "ok");
    oHttpMsg.set_body(oRsp.ToString());
    g_pLabor->SendTo(m_stReqMsgShell, oHttpMsg);
}

} /* namespace net */
