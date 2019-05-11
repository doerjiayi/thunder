/*
 * StepNode.cpp
 *
 *  Created on: 2017年8月1日
 *      Author: chen
 */
#include  <limits.h>
#include "StepState.hpp"
#include "NetError.hpp"
#include "NetDefine.hpp"

namespace net
{

StepState::StepState()
{
	Init();
}

StepState::StepState(const tagMsgShell& stInMsgShell, const MsgHead& oInMsgHead):net::HttpStep(stInMsgShell,oInMsgHead)
{
	Init();
}

StepState::StepState(const tagMsgShell& stInMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody):net::HttpStep(stInMsgShell,oInMsgHead,oInMsgBody)
{
	Init();
}

StepState::StepState(const tagMsgShell& stInMsgShell, const HttpMsg& oInHttpMsg):net::HttpStep(stInMsgShell,oInHttpMsg)
{
	Init();
}
uint64 g_CreStepState = 0;
uint64 g_DelStepState = 0;
StepState::~StepState()
{
	++g_DelStepState;
}

void StepState::Init()
{
	++g_CreStepState;
	m_uiTimeOutCounter = 0;
	m_uiTimeOutMax = 3;
	m_uiTimeOutRetry = 0;
	m_uiLastState = m_uiState = 0;
	m_uiNextState = -1;
	m_uiStateVecNum = 0;
	m_data = NULL;
	m_iErrno = 0;
	memset(m_StateVec,0,sizeof(m_StateVec));
	m_SuccFunc = NULL;
	m_FailFunc = NULL;
	m_strStepDesc = "StepState";
	m_boJumpState = false;
}

void StepState::AddStateFunc(StateFunc func)
{
	if (m_uiStateVecNum < StepStateVecSize)
	{
		m_StateVec[m_uiStateVecNum++] = func;
	}
}

E_CMD_STATUS StepState::Emit(int iErrno , const std::string& strErrMsg , const std::string& strErrShow )
{
	LOG4_TRACE("%s() uiState(%u) uiStateVecNum(%u) strStepDesc:%s g_CreStepState(%llu) g_DelStepState(%llu)",
			__FUNCTION__,m_uiState,m_uiStateVecNum,m_strStepDesc.c_str(),g_CreStepState,g_DelStepState);
	if (0 != iErrno)
	{
		m_iErrno = iErrno;
		m_strErrMsg = strErrMsg;
		OnFail();
		LOG4_WARN("%s() Fail uiLastState(%u) uiState(%u)",__FUNCTION__,m_uiLastState,m_uiState);
		return STATUS_CMD_FAULT;
	}
	if (m_uiNextState >= 0)
    {
        //如果修改了状态则运行该状态(因为可以在回调中修改状态)
        LOG4_TRACE("%s() uiLastState(%u) next uiState(%u)",__FUNCTION__,m_uiLastState,m_uiState);
        m_uiState = m_uiNextState;
        m_uiNextState = -1;
    }
	if (m_uiState < m_uiStateVecNum)
	{
		m_uiLastState = m_uiState;
		LOG4_TRACE("%s() uiLastState(%u) uiState(%u) before run",__FUNCTION__,m_uiLastState,m_uiState);
		if (m_StateVec[m_uiState])
		{
			m_RunClock.StartClock(m_uiState);
			bool boRet = m_StateVec[m_uiState](this);
			m_RunClock.EndClock();
			LOG4_TRACE("%s() uiLastState(%u) uiState(%u) after run",__FUNCTION__,m_uiLastState,m_uiState);
			if (!boRet)
			{
				OnFail();
				LOG4_TRACE("%s() Fail uiLastState(%u) uiState(%u) uiStateVecNum(%u)",__FUNCTION__,m_uiLastState,m_uiState,m_uiStateVecNum);
				return STATUS_CMD_FAULT;
			}
			if (m_uiNextState >= 0)
            {
				//如果修改了状态则运行该状态
				LOG4_TRACE("%s() uiLastState(%u) next uiState(%u) uiStateVecNum(%u)",__FUNCTION__,m_uiLastState,m_uiState,m_uiStateVecNum);
				if (m_boJumpState)//不等待本状态回调，直接运行下一个状态
				{
					m_boJumpState = false;
					return Emit();
				}
                m_uiState = m_uiNextState;
                m_uiNextState = -1;
                return STATUS_CMD_RUNNING;
            }
			++m_uiState;//默认转为下一个状态，如果需要另行设置状态则需要自己设置
			if (m_uiState < m_uiStateVecNum)
			{
				LOG4_TRACE("%s() uiLastState(%u) next uiState(%u) uiStateVecNum(%u)",__FUNCTION__,m_uiLastState,m_uiState,m_uiStateVecNum);
				return STATUS_CMD_RUNNING;
			}
		}
	}
	OnSucc();
	LOG4_TRACE("%s() complete uiState(%u) uiLastState(%u) uiStateVecNum(%u)",__FUNCTION__,m_uiState,m_uiLastState,m_uiStateVecNum);
	return STATUS_CMD_COMPLETED;
}

E_CMD_STATUS StepState::Timeout()
{
    LOG4_TRACE("%s()",__FUNCTION__);
    ++m_uiTimeOutCounter;
    if (m_uiTimeOutCounter < m_uiTimeOutMax)
    {
    	if (m_uiTimeOutRetry > 0)
    	{
    	    SetNextState(m_uiState - 1);
    		LOG4_WARN("%s() retry last stage. uiTimeOutCounter(%u) uiTimeOutMax(%u) uiTimeOutRetry(%u) State(%u)",
    		    		__FUNCTION__,m_uiTimeOutCounter,m_uiTimeOutMax,m_uiTimeOutRetry,m_uiState);
    		m_RunClock.TotalRunTime();
    		return Emit();//retry last stage
    	}
        return STATUS_CMD_RUNNING;
    }
    LOG4_ERROR("%s() uiTimeOutCounter(%u) uiTimeOutMax(%u) uiTimeOutRetry(%u) State(%u)",
    		__FUNCTION__,m_uiTimeOutCounter,m_uiTimeOutMax,m_uiTimeOutRetry,m_uiState);
    OnFail();
    return STATUS_CMD_FAULT;
}

E_CMD_STATUS StepState::Callback(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data)
{
    LOG4_TRACE("%s()",__FUNCTION__);
    if(net::CMD_RSP_SYS_ERROR == oInMsgHead.cmd())
    {
        LOG4_ERROR("system response error");
        return net::STATUS_CMD_FAULT;
    }
    m_oResMsgHead = oInMsgHead;
    m_oResMsgBody = oInMsgBody;
    //继续下一个状态
	LOG4_TRACE("%s() continue next uiState(%u) uiLastState(%u) ",__FUNCTION__,m_uiState,m_uiLastState);
	m_uiTimeOutCounter = 0;//新状态重置超时计数
	return Emit();
}

E_CMD_STATUS StepState::Callback(const tagMsgShell& stMsgShell,const HttpMsg& oHttpMsg,void* data)
{
	LOG4_TRACE("%s()",__FUNCTION__);
	m_oResHttpMsg = oHttpMsg;
	//继续下一个状态
	LOG4_TRACE("%s() continue next uiState(%u) uiLastState(%u) ",__FUNCTION__,m_uiState,m_uiLastState);
	m_uiTimeOutCounter = 0;//新状态重置超时计数
	return Emit();
}

}
