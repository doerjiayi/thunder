/*******************************************************************************
 * Project:  PluginServer
 * @file     Cmd.cpp
 * @brief 
 * @author   bwarliao
 * @date:    2017年3月6日
 * @note
 * Modify history:
 ******************************************************************************/
#include "step/Step.hpp"
#include "Cmd.hpp"

namespace thunder
{

Cmd::Cmd()
    : m_pErrBuff(NULL), m_pLabor(0), m_pLogger(0), m_iCmd(0)
{
    m_pErrBuff = new char[gc_iErrBuffLen];
	m_uiCmd = 0;
}

Cmd::~Cmd()
{
    if (m_pErrBuff != NULL)
    {
        delete[] m_pErrBuff;
        m_pErrBuff = NULL;
    }
}

const std::string& Cmd::GetWorkPath() const
{
    return(m_pLabor->GetWorkPath());
}

uint32 Cmd::GetNodeId()
{
    return(m_pLabor->GetNodeId());
}

uint32 Cmd::GetWorkerIndex()
{
    return(m_pLabor->GetWorkerIndex());
}

const std::string& Cmd::GetWorkerIdentify()
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

const std::string& Cmd::GetNodeType() const
{
    return(m_pLabor->GetNodeType());
}

const thunder::CJsonObject& Cmd::GetCustomConf() const
{
    return(m_pLabor->GetCustomConf());
}

time_t Cmd::GetNowTime() const
{
    return(m_pLabor->GetNowTime());
}

bool Cmd::RegisterCallback(Step* pStep, ev_tstamp dTimeout)
{
    return(m_pLabor->RegisterCallback(pStep, dTimeout));
}

void Cmd::DeleteCallback(Step* pStep)
{
    m_pLabor->DeleteCallback(pStep);
}

bool Cmd::Pretreat(Step* pStep)
{
    return(m_pLabor->Pretreat(pStep));
}

bool Cmd::RegisterCallback(Session* pSession)
{
    return(m_pLabor->RegisterCallback(pSession));
}

void Cmd::DeleteCallback(Session* pSession)
{
    return(m_pLabor->DeleteCallback(pSession));
}

Session* Cmd::GetSession(uint64 uiSessionId, const std::string& strSessionClass)
{
    return(m_pLabor->GetSession(uiSessionId, strSessionClass));
}

Session* Cmd::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    return(m_pLabor->GetSession(strSessionId, strSessionClass));
}

bool Cmd::RegisterCallback(const std::string& strRedisNodeType, RedisStep* pRedisStep)
{
    return(GetLabor()->RegisterCallback(strRedisNodeType, pRedisStep));
}

bool Cmd::RegisterCallback(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    return(GetLabor()->RegisterCallback(strHost, iPort, pRedisStep));
}

bool Cmd::AsyncStep(Step* pStep,ev_tstamp dTimeout)
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
    if (STATUS_CMD_RUNNING != pStep->Emit(ERR_OK))
    {
        DeleteCallback(pStep);
        return(false);
    }
    return true;
}

} /* namespace thunder */
