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

namespace hello
{

StepHttpRequestState::StepHttpRequestState(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
	m_stReqMsgShell = stMsgShell;
	m_oInHttpMsg = oInHttpMsg;
	StateInit();
}

StepHttpRequestState::~StepHttpRequestState()
{
}

void StepHttpRequestState::StateInit()
{
	StateAdd(1,(StateFunc)&StepHttpRequestState::State1);//强制转换func()的类型
	StateAdd(2,(StateFunc)&StepHttpRequestState::State2);
	StateAdd(3,(StateFunc)&StepHttpRequestState::State3);
}

thunder::E_CMD_STATUS StepHttpRequestState::State1()
{
	LOG4CPLUS_DEBUG_FMT(GetLogger(), "%s last state:%u ResHttpMsg:%s",
				__FUNCTION__,GetLastState(),m_oResHttpMsg.DebugString().c_str());
	if (HttpGet("https://www.baidu.com"))
	{
		return(thunder::STATUS_CMD_RUNNING);
	}
	else
	{
		return(thunder::STATUS_CMD_FAULT);
	}
}

thunder::E_CMD_STATUS StepHttpRequestState::State2()
{
	LOG4CPLUS_DEBUG_FMT(GetLogger(), "%s last state:%u ResHttpMsg:%s",
			__FUNCTION__,GetLastState(),m_oResHttpMsg.DebugString().c_str());
	if (HttpGet("https://www.github.com"))
	{
		return(thunder::STATUS_CMD_RUNNING);
	}
	else
	{
		return(thunder::STATUS_CMD_FAULT);
	}
}

thunder::E_CMD_STATUS StepHttpRequestState::State3()
{
	LOG4CPLUS_DEBUG_FMT(GetLogger(), "%s last state:%u ResHttpMsg:%s",
				__FUNCTION__,GetLastState(),m_oResHttpMsg.DebugString().c_str());
	LOG4CPLUS_DEBUG_FMT(GetLogger(),"StepState done");
	return(thunder::STATUS_CMD_RUNNING);
}



} /* namespace oss */
