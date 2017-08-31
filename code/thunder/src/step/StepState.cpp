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
    m_uiLastState = m_uiState = INT_MAX;
}

thunder::E_CMD_STATUS StepState::Emit(int iErrno , const std::string& strErrMsg , const std::string& strErrShow )
{
	LOG4CPLUS_TRACE_FMT(GetLogger(),"%s() uiState(%u)",__FUNCTION__,m_uiState);
	StateMap::iterator it = m_StateMap.find(m_uiState);
	if (it != m_StateMap.end())
	{
		m_uiLastState = m_uiState;
		LOG4CPLUS_TRACE_FMT(GetLogger(),"%s() uiState(%u) before exe",__FUNCTION__,m_uiState);
		thunder::E_CMD_STATUS s = it->second(this);
		LOG4CPLUS_TRACE_FMT(GetLogger(),"%s() uiState(%u) after exe",__FUNCTION__,m_uiState);
		if (s == thunder::STATUS_CMD_RUNNING && m_uiLastState == m_uiState)
		{
			StateMap::iterator itNext = it;
			++itNext;
			if (itNext != m_StateMap.end())
			{
				m_uiState = itNext->first;//默认为下一个状态，如果需要另行设置状态则需要自己设置
				LOG4CPLUS_TRACE_FMT(GetLogger(),"%s() next uiState(%u)",__FUNCTION__,m_uiState);
			}
			else
			{
				LOG4CPLUS_TRACE_FMT(GetLogger(),"%s() done uiState(%u)",__FUNCTION__,m_uiState);
				//没有下一个状态的则结束
				return thunder::STATUS_CMD_COMPLETED;
			}
		}
		LOG4CPLUS_TRACE_FMT(GetLogger(),"%s() return uiState(%u) s(%d)",__FUNCTION__,m_uiState,s);
		return s;
	}
	LOG4CPLUS_ERROR_FMT(GetLogger(),"%s() invalid state(%u)",__FUNCTION__,m_uiState);
	return thunder::STATUS_CMD_FAULT;
}

bool StepState::Launch(NodeLabor* pLabor,StepState *step)
{
	if (step == NULL)
	{
		LOG4CPLUS_ERROR_FMT(pLabor->GetLogger(),"%s() null step",__FUNCTION__);
		return(false);
	}
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
		return(true);
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
    return Emit();
}

thunder::E_CMD_STATUS StepState::Callback(
                        const MsgShell& stMsgShell,
                        const HttpMsg& oHttpMsg,
                        void* data)
{
	LOG4CPLUS_TRACE_FMT(GetLogger(),"%s()",__FUNCTION__);
	return Emit();
}


}
