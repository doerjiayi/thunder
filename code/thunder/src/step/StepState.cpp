/*
 * StepNodeAccess.cpp
 *
 *  Created on: 2017年8月1日
 *      Author: chen
 */
#include  <limits.h>
#include "StepState.hpp"
#include "ThunderError.hpp"
#include "ThunderDefine.hpp"

namespace thunder
{

StepState::StepState()
{
    m_uiTimeOut = 0;
    m_uiTimeOutCounter = 3;
    m_uiRetryTry = 0;
    m_uiLastState = m_uiState = 0;
    InitState();
}

void StepState::InitState()
{
	m_StateVec[0] = (StateFunc)&StepState::State0;
	m_StateVec[1] = (StateFunc)&StepState::State1;
	m_StateVec[2] = (StateFunc)&StepState::State2;
	m_StateVec[3] = (StateFunc)&StepState::State3;
	m_StateVec[4] = (StateFunc)&StepState::State4;
	m_StateVec[5] = (StateFunc)&StepState::State5;
	m_StateVec[6] = (StateFunc)&StepState::State6;
	m_StateVec[7] = (StateFunc)&StepState::State7;
	m_StateVec[8] = (StateFunc)&StepState::State8;
	m_StateVec[9] = (StateFunc)&StepState::State9;
}

thunder::E_CMD_STATUS StepState::Emit(int iErrno , const std::string& strErrMsg , const std::string& strErrShow )
{
	LOG4_TRACE("%s() uiState(%u)",__FUNCTION__,m_uiState);
	if (m_uiState < StepStateSize && m_StateVec[m_uiState])
	{
		m_uiLastState = m_uiState;
		LOG4_TRACE("%s() uiState(%u) before run",__FUNCTION__,m_uiState);
		thunder::E_CMD_STATUS s = m_StateVec[m_uiState](this);
		LOG4_TRACE("%s() uiState(%u) after run",__FUNCTION__,m_uiState);
		if (s == thunder::STATUS_CMD_RUNNING && m_uiLastState == m_uiState)
		{
			if ( m_uiState + 1 < StepStateSize)
			{
				++m_uiState;//默认转为下一个状态，如果需要另行设置状态则需要自己设置
			}
			else
			{
				LOG4_TRACE("%s() done uiState(%u)",__FUNCTION__,m_uiState);
				//没有下一个状态的则结束
				return thunder::STATUS_CMD_COMPLETED;
			}
		}
		LOG4_TRACE("%s() return uiState(%u) s(%d)",__FUNCTION__,m_uiState,s);
		return s;
	}
	LOG4_ERROR("%s() invalid state(%u)",__FUNCTION__,m_uiState);
	return thunder::STATUS_CMD_FAULT;
}

bool StepState::Launch(NodeLabor* pLabor,StepState *step)
{
	if (step == NULL)
	{
		LOG4CPLUS_ERROR_FMT(pLabor->GetLogger(),"%s() null step",__FUNCTION__);
		return(false);
	}
	step->InitState();//重新覆盖
	if (!pLabor->RegisterCallback(step))
	{
		LOG4CPLUS_ERROR_FMT(pLabor->GetLogger(),"%s() RegisterCallback error",__FUNCTION__);
		delete step;
		step = NULL;
		return(false);
	}
	if (thunder::STATUS_CMD_RUNNING != step->Emit())
	{
		pLabor->DeleteCallback(step);
		return(false);
	}
	return true;
}

thunder::E_CMD_STATUS StepState::Timeout()
{
    LOG4CPLUS_TRACE_FMT(GetLogger(),"%s()",__FUNCTION__);
    ++m_uiTimeOut;
    if (m_uiTimeOut < m_uiTimeOutCounter)
    {
    	if (m_uiRetryTry > 0)
    	{
    		return Emit();
    	}
        return thunder::STATUS_CMD_RUNNING;
    }
    LOG4CPLUS_ERROR_FMT(GetLogger(),"%s()",__FUNCTION__);
    return thunder::STATUS_CMD_COMPLETED;
}

thunder::E_CMD_STATUS StepState::Callback(
        const thunder::MsgShell& stMsgShell,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody,
        void* data)
{
    LOG4CPLUS_TRACE_FMT(GetLogger(),"%s()",__FUNCTION__);
    if(thunder::CMD_RSP_SYS_ERROR == oInMsgHead.cmd())
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),"system response error");
        return thunder::STATUS_CMD_FAULT;
    }
    m_oResMsgHead = oInMsgHead;
    m_oResMsgBody = oInMsgBody;
    return Emit();
}

thunder::E_CMD_STATUS StepState::Callback(
                        const MsgShell& stMsgShell,
                        const HttpMsg& oHttpMsg,
                        void* data)
{
	LOG4CPLUS_TRACE_FMT(GetLogger(),"%s()",__FUNCTION__);
	m_oResHttpMsg = oHttpMsg;
	return Emit();
}


}
