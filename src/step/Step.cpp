/*******************************************************************************
 * Project:  AsyncServer
 * @file     Step.cpp
 * @brief 
 * @author   bwarliao
 * @date:    2015年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#include "hiredis/adapters/libev.h"
#include "Step.hpp"

namespace oss
{

Step::Step(Step* pNextStep)
    : m_bRegistered(false), m_ulSequence(0), m_dActiveTime(0.0), m_dTimeout(0.5),
      m_pLabor(0), m_pLogger(0), m_pTimeoutWatcher(0), m_pNextStep(pNextStep)
{
    AddNextStepSeq(pNextStep);
	m_uiUserId = 0;
	m_uiCmd = 0;
}

Step::Step(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, Step* pNextStep)
    : m_stReqMsgShell(stReqMsgShell), m_oReqMsgHead(oReqMsgHead), m_oReqMsgBody(oReqMsgBody),
      m_bRegistered(false), m_ulSequence(0), m_dActiveTime(0.0), m_dTimeout(0.5),
      m_pLabor(0), m_pLogger(0), m_pTimeoutWatcher(0), m_pNextStep(pNextStep)
{
    AddNextStepSeq(pNextStep);
	m_uiUserId = 0;
	m_uiCmd = 0;
}

Step::Step(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, Step* pNextStep)
    : m_stReqMsgShell(stReqMsgShell), m_oReqMsgHead(oReqMsgHead),
      m_bRegistered(false), m_ulSequence(0), m_dActiveTime(0.0), m_dTimeout(0.5),
      m_pLabor(0), m_pLogger(0), m_pTimeoutWatcher(0), m_pNextStep(pNextStep)
{
    AddNextStepSeq(pNextStep);
    m_uiUserId = 0;
    m_uiCmd = 0;
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
}

bool Step::RegisterCallback(Step* pStep, ev_tstamp dTimeout)
{
    bool bRegisterResult = false;
    bRegisterResult = m_pLabor->RegisterCallback(GetSequence(), pStep, dTimeout);
    if (bRegisterResult && (m_pNextStep == pStep))
    {
        m_setNextStepSeq.insert(pStep->GetSequence());
    }
    return(bRegisterResult);
}

void Step::DeleteCallback(Step* pStep)
{
    LOG4_TRACE("Step[%u]::%s()", GetSequence(), __FUNCTION__);
    m_pLabor->DeleteCallback(GetSequence(), pStep);
}

bool Step::Pretreat(Step* pStep)
{
    return(m_pLabor->Pretreat(pStep));
}

bool Step::RegisterCallback(Session* pSession)
{
    return(m_pLabor->RegisterCallback(pSession));
}

void Step::DeleteCallback(Session* pSession)
{
    return(m_pLabor->DeleteCallback(pSession));
}

const std::string& Step::GetWorkPath() const
{
    return(m_pLabor->GetWorkPath());
}

uint32 Step::GetNodeId()
{
    return(m_pLabor->GetNodeId());
}

uint32 Step::GetWorkerIndex()
{
    return(m_pLabor->GetWorkerIndex());
}

const std::string& Step::GetWorkerIdentify()
{
    if (m_strWorkerIdentify.size() < 5) // IP + port + worker_index长度一定会大于这个数即可，不在乎数值是什么
    {
        char szWorkerIdentify[64] = {0};
        snprintf(szWorkerIdentify, 64, "%s:%d.%d", m_pLabor->GetHostForServer().c_str(),
                        m_pLabor->GetPortForServer(), m_pLabor->GetWorkerIndex());
        m_strWorkerIdentify = szWorkerIdentify;
    }
    return(m_strWorkerIdentify);
}

const std::string& Step::GetNodeType() const
{
    return(m_pLabor->GetNodeType());
}

const loss::CJsonObject& Step::GetCustomConf() const
{
    return(m_pLabor->GetCustomConf());
}

time_t Step::GetNowTime() const
{
    return(m_pLabor->GetNowTime());
}

Session* Step::GetSession(uint64 uiSessionId, const std::string& strSessionClass)
{
    return(m_pLabor->GetSession(uiSessionId, strSessionClass));
}

Session* Step::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    return(m_pLabor->GetSession(strSessionId, strSessionClass));
}

bool Step::SendTo(const tagMsgShell& stMsgShell)
{
    return(m_pLabor->SendTo(stMsgShell));
}

bool Step::SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    return(m_pLabor->SendTo(stMsgShell, oMsgHead, oMsgBody));
}

bool Step::SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    return(m_pLabor->SendTo(strIdentify, oMsgHead, oMsgBody));
}

bool Step::SendToNext(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    return(m_pLabor->SendToNext(strNodeType, oMsgHead, oMsgBody));
}

bool Step::SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    return(m_pLabor->SendToWithMod(strNodeType, uiModFactor, oMsgHead, oMsgBody));
}

bool Step::AsyncStep(Step* pStep,ev_tstamp dTimeout)
{
    if (pStep == NULL)
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),"pStep == NULL!");
        return(false);
    }
    if (!RegisterCallback(pStep,dTimeout))
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),"RegisterCallback(pStep) error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (oss::STATUS_CMD_RUNNING != pStep->Emit(ERR_OK))
    {
        DeleteCallback(pStep);
        return(false);
    }
    return true;
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
            if (oss::STATUS_CMD_RUNNING != pNextStep->Emit(iErrno, strErrMsg, strErrClientShow))
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
    for (std::set<uint32>::iterator seq_iter = m_setNextStepSeq.begin();
                    seq_iter != m_setNextStepSeq.end(); ++seq_iter)
    {
        m_pLabor->ExecStep(GetSequence(), *seq_iter, iErrno, strErrMsg, strErrClientShow);
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
            for (int i = 0; i < 3; ++i)
            {
                if (RegisterCallback(m_pNextStep))
                {
                    break;
                }
            }
        }
        if (m_pNextStep->IsRegistered())
        {
            if (oss::STATUS_CMD_RUNNING != m_pNextStep->Emit(iErrno, strErrMsg, strErrClientShow))
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
        if (NULL != m_pLabor)
        {
            m_ulSequence = m_pLabor->GetSequence();
        }
    }
    return(m_ulSequence);
}

void Step::DelayTimeout()
{
    if (IsRegistered())
    {
        m_pLabor->ResetTimeout(this, m_pTimeoutWatcher);
    }
    else
    {
        m_dActiveTime += m_dTimeout + 0.5;
    }
}

bool Step::AddMsgShell(const std::string& strIdentify, const tagMsgShell& stMsgShell)
{
	return(m_pLabor->AddMsgShell(strIdentify, stMsgShell));
}

void Step::DelMsgShell(const std::string& strIdentify, const tagMsgShell& stMsgShell)
{
    m_pLabor->DelMsgShell(strIdentify,stMsgShell);
}

void Step::AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    m_pLabor->AddNodeIdentify(strNodeType, strIdentify);
}

void Step::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    m_pLabor->DelNodeIdentify(strNodeType, strIdentify);
}

/*
void Step::AddRedisNodeConf(const std::string strNodeType, const std::string strHost, int iPort)
{
    m_pLabor->AddRedisNodeConf(strNodeType, strHost, iPort);
}

void Step::DelRedisNodeConf(const std::string strNodeType, const std::string strHost, int iPort)
{
    m_pLabor->DelRedisNodeConf(strNodeType, strHost, iPort);
}
*/

bool Step::AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx)
{
    return(m_pLabor->AddRedisContextAddr(strHost, iPort, ctx));
}

void Step::DelRedisContextAddr(const redisAsyncContext* ctx)
{
    m_pLabor->DelRedisContextAddr(ctx);
}

bool Step::RegisterCallback(const std::string& strIdentify, RedisStep* pRedisStep)
{
    return(m_pLabor->RegisterCallback(strIdentify, pRedisStep));
}

bool Step::RegisterCallback(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    return(m_pLabor->RegisterCallback(strHost, iPort, pRedisStep));
}

void Step::AddNextStepSeq(Step* pStep)
{
    if (NULL != pStep && pStep->IsRegistered())
    {
        m_setNextStepSeq.insert(pStep->GetSequence());
//        if (pStep == m_pNextStep)
//        {
//            m_pNextStep == NULL;    // 将下一个step的seq加入到m_setNextStepSeq后即将m_pNextStep抹掉，以免重复调用或析构
//        }
    }
}

void Step::AddPreStepSeq(Step* pStep)
{
    if (NULL != pStep && pStep->IsRegistered())
    {
        m_setPreStepSeq.insert(pStep->GetSequence());
    }
}

} /* namespace oss */
