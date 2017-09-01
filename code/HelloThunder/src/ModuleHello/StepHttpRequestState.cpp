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
}

StepHttpRequestState::~StepHttpRequestState()
{
}

void StepHttpRequestState::InitState()
{
	m_StateVec[0] = (StateFunc)&StepHttpRequestState::State0;
	m_StateVec[1] = (StateFunc)&StepHttpRequestState::State1;
	m_StateVec[2] = (StateFunc)&StepHttpRequestState::State2;
	m_StateVec[3] = (StateFunc)&StepHttpRequestState::State3;
}

thunder::E_CMD_STATUS StepHttpRequestState::State0()
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

thunder::E_CMD_STATUS StepHttpRequestState::State1()
{
	LOG4CPLUS_DEBUG_FMT(GetLogger(), "%s last state:%u ResHttpMsg:%s",
			__FUNCTION__,GetLastState(),m_oResHttpMsg.DebugString().c_str());
	if (HttpGet("https://www.sogou.com/"))
	{
		SetNextState(3);
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
	LOG4CPLUS_DEBUG_FMT(GetLogger(),"StepState done");
	SendTo(m_stReqMsgShell, m_oInHttpMsg);
	return(thunder::STATUS_CMD_COMPLETED);
}

thunder::E_CMD_STATUS StepHttpRequestState::State3()
{
	LOG4CPLUS_DEBUG_FMT(GetLogger(), "%s last state:%u ResHttpMsg:%s",
				__FUNCTION__,GetLastState(),m_oResHttpMsg.DebugString().c_str());
	LOG4CPLUS_DEBUG_FMT(GetLogger(),"StepState done");
	SendTo(m_stReqMsgShell, m_oInHttpMsg);
	return(thunder::STATUS_CMD_COMPLETED);
}

} /* namespace oss */
