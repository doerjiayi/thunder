/*******************************************************************************
 * Project:  Net
 * @file     Step.cpp
 * @brief 
 * @author   chenjiayi
 * @date:    2019年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#include "hiredis_vip/adapters/libev.h"
#include "Step.hpp"
#include "labor/duty/Worker.hpp"

namespace net
{

Step::Step(Step* pNextStep)
    : m_bRegistered(false), m_ulSequence(0), m_dActiveTime(0.0), m_dTimeout(0.5),
      m_pTimeoutWatcher(0), m_pNextStep(pNextStep)
{
    AddNextStepSeq(pNextStep);
	m_uiUserId = 0;
	m_uiCmd = 0;
	m_data = NULL;
}

Step::Step(const tagMsgShell& stReqMsgShell,Step* pNextStep)
    : m_stReqMsgShell(stReqMsgShell),
      m_bRegistered(false), m_ulSequence(0), m_dActiveTime(0.0), m_dTimeout(0.5),
      m_pTimeoutWatcher(0), m_pNextStep(pNextStep)
{
    AddNextStepSeq(pNextStep);
    Init();
}

Step::Step(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, Step* pNextStep)
    : m_stReqMsgShell(stReqMsgShell), m_oReqMsgHead(oReqMsgHead),
      m_bRegistered(false), m_ulSequence(0), m_dActiveTime(0.0), m_dTimeout(0.5),
      m_pTimeoutWatcher(0), m_pNextStep(pNextStep)
{
    AddNextStepSeq(pNextStep);
    Init();
}

Step::Step(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, Step* pNextStep)
    : m_stReqMsgShell(stReqMsgShell), m_oReqMsgHead(oReqMsgHead), m_oReqMsgBody(oReqMsgBody),
      m_bRegistered(false), m_ulSequence(0), m_dActiveTime(0.0), m_dTimeout(0.5),
      m_pTimeoutWatcher(0), m_pNextStep(pNextStep)
{
    AddNextStepSeq(pNextStep);
    Init();
}

void Step::Init()
{
	m_uiUserId = 0;
	m_uiCmd = 0;
	m_data = NULL;
	m_boDelayDel = false;
}

Step::~Step()
{
    if (IsRegistered())
    {
        LOG4_TRACE("step %u destruct, m_pNextStep 0x%x", GetSequence(), m_pNextStep);
    }
    if (m_pTimeoutWatcher != 0)
    {
        free(m_pTimeoutWatcher);
        m_pTimeoutWatcher = 0;
    }
    if (m_pNextStep)
    {
        if (!m_pNextStep->IsRegistered())
        {
            delete m_pNextStep;
            m_pNextStep = NULL;
        }
    }
    m_setNextStepSeq.clear();
    m_setPreStepSeq.clear();
    SAFE_DELETE(m_data);
}

bool Step::RegisterCallback(Step* pStep, ev_tstamp dTimeout)
{
    bool bRegisterResult = false;
    bRegisterResult = g_pLabor->RegisterCallback(GetSequence(), pStep, dTimeout);
    if (bRegisterResult && (m_pNextStep == pStep))
    {
        m_setNextStepSeq.insert(pStep->GetSequence());
    }
    return(bRegisterResult);
}

void Step::DelayNextStep()
{
    if (m_pNextStep)
    {
        if (m_pNextStep->IsRegistered())
        {
            m_pNextStep->DelayTimeout();
        }
    }
}

bool Step::NextStep(Step* pNextStep, int iErrno, const std::string& strErrMsg, const std::string& strErrClientShow)
{
    if (pNextStep)
    {
        if (!pNextStep->IsRegistered())
        {
            for (int i = 0; i < 3; ++i)
            {
                if (RegisterCallback(pNextStep))
                {
                    break;
                }
            }
        }
        if (pNextStep->IsRegistered())
        {
            if (net::STATUS_CMD_RUNNING != pNextStep->Emit(iErrno, strErrMsg, strErrClientShow))
            {
                DeleteCallback(pNextStep);
            }
            return(true);
        }
    }
    return(false);
}

bool Step::NextStep(int iErrno, const std::string& strErrMsg, const std::string& strErrClientShow)
{
    for (auto seq_iter:m_setNextStepSeq)
    {
        g_pLabor->ExecStep(GetSequence(),seq_iter, iErrno, strErrMsg, strErrClientShow);
    }
    if (m_setNextStepSeq.size() > 0)
    {
        return(true);
    }
    LOG4_TRACE("m_pNextStep 0x%x", m_pNextStep);
    if (m_pNextStep)
    {
        if (!m_pNextStep->IsRegistered())
        {
        	RegisterCallback(m_pNextStep);
        }
        if (m_pNextStep->IsRegistered())
        {
            if (net::STATUS_CMD_RUNNING != m_pNextStep->Emit(iErrno, strErrMsg, strErrClientShow))
            {
                DeleteCallback(m_pNextStep);
                m_pNextStep = NULL;
            }
            return(true);
        }
        else
        {
            delete m_pNextStep;
            m_pNextStep = NULL;
        }
    }
    return(false);
}

void Step::SetNextStepNull()
{
    m_setNextStepSeq.clear();
    m_pNextStep = NULL;
}

uint32 Step::GetSequence()
{
    if (!m_bRegistered)
    {
        return(0);
    }
    if (0 == m_ulSequence)
    {
        if (NULL != g_pLabor)
        {
            m_ulSequence = g_pLabor->GetSequence();
        }
    }
    return(m_ulSequence);
}

void Step::DelayTimeout()
{
    if (m_bRegistered)
    {
    	LOG4_TRACE("step %u DelayTimeout dActiveTime(%lf) dTimeout(%lf)", GetSequence(),m_dActiveTime,m_dTimeout);
        g_pLabor->ResetTimeout(this, m_pTimeoutWatcher);
    }
    else
    {
        m_dActiveTime += m_dTimeout + 0.5;
    }
}

void Step::AddNextStepSeq(Step* pStep)
{
    if (NULL != pStep && pStep->IsRegistered() && m_bRegistered)
    {
    	LOG4_DEBUG("step %u AddNextStepSeq %u",GetSequence(),pStep->GetSequence());
        m_setNextStepSeq.insert(pStep->GetSequence());
    }
}

void Step::AddPreStepSeq(Step* pStep)
{
    if (NULL != pStep && pStep->IsRegistered())
    {
        m_setPreStepSeq.insert(pStep->GetSequence());
    }
}

void Step::RemovePreStepSeq(Step* pStep)
{
	if (NULL != pStep && pStep->IsRegistered())
	{
		m_setPreStepSeq.erase(pStep->GetSequence());
	}
}

} /* namespace net */
