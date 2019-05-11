#include "Coroutine.h"
#include "Interface.hpp"
#include "Labor.hpp"

namespace net
{

Coroutine::Coroutine()
{
	m_CoroutineSchedule.schedule = util::coroutine_open();
}

Coroutine::~Coroutine()
{
	if (m_CoroutineSchedule.schedule)
	{
		coroutine_close(m_CoroutineSchedule.schedule);
		m_CoroutineSchedule.schedule = NULL;
	}
}

bool Coroutine::CoroutineNewWithArg(util::coroutine_func func,tagCoroutineArg *arg)
{
	int coid = CoroutineNew(func,arg);
	if (coid >= 0)
	{
		auto ret = m_CoroutineScheduleArgs.insert(std::make_pair(coid,arg));
		if (!ret.second)
		{
			LOG4_ERROR("%s failed to add args(%p) for coid(%u)", __FUNCTION__,arg,coid);
			return false;
		}
		return true;
	}
	return false;
}

int Coroutine::CoroutineNew(util::coroutine_func func,void *ud)
{
	int coid(0);
	coid = util::coroutine_new(m_CoroutineSchedule.schedule, func, ud);
	if (coid >= 0)
	{
		m_CoroutineSchedule.coroutineIds.insert(coid);
		m_CoroutineSchedule.coroutineRunIter = m_CoroutineSchedule.coroutineIds.begin();
		LOG4_TRACE("%s coroutine coid(%u) status(%d)",__FUNCTION__,coid,util::coroutine_status(m_CoroutineSchedule.schedule,coid));
	}
	else
	{
		LOG4_ERROR("%s coroutine invalid coid(%u)", __FUNCTION__, coid);

	}
	return coid;
}

bool Coroutine::CoroutineResumeWithTimes(unsigned int nMaxTimes)
{
	bool boHasCo(true);
	if (0 == nMaxTimes)
	{
		while (boHasCo) {boHasCo = CoroutineResume();}
	}
	else
	{
		while (nMaxTimes > 0 && boHasCo) {--nMaxTimes;boHasCo = CoroutineResume();}
	}
	return boHasCo;
}

bool Coroutine::CoroutineResume()
{
	LOG4_TRACE("%s current TaskSize(%u) Args size(%u)",__FUNCTION__,CoroutineTaskSize(),m_CoroutineScheduleArgs.size());
	tagCoroutineSchedule& coroutineSchedule = m_CoroutineSchedule;
	while (coroutineSchedule.coroutineIds.size() > 0)
	{
		if (coroutineSchedule.coroutineRunIter == coroutineSchedule.coroutineIds.end())
		{
			coroutineSchedule.coroutineRunIter = coroutineSchedule.coroutineIds.begin();
		}
		int coid = *coroutineSchedule.coroutineRunIter;
		if (0 > coid)
		{
			LOG4_ERROR("%s invaid coid(%d)",__FUNCTION__,coid);
			coroutineSchedule.coroutineIds.erase(coroutineSchedule.coroutineRunIter++);
			continue;
		}
		int nStatus = util::coroutine_status(coroutineSchedule.schedule,coid);
		LOG4_TRACE("%s coroutine_status coid(%d) status(%d)",__FUNCTION__,coid,nStatus);
		if (0 == nStatus)
		{
			LOG4_TRACE("%s dead coid(%d)",__FUNCTION__,coid);
			coroutineSchedule.coroutineIds.erase(coroutineSchedule.coroutineRunIter++);
			auto argIter = m_CoroutineScheduleArgs.find(coid);
			if (argIter != m_CoroutineScheduleArgs.end())
			{
				LOG4_INFO("%s destroy arg for coid(%d) status(%d)",__FUNCTION__,coid,nStatus);
				delete argIter->second;
				m_CoroutineScheduleArgs.erase(argIter);
			}
			continue;
		}
		{
			LOG4_TRACE("%s CoroutineResume coid(%d)",__FUNCTION__,coid);
			{
				int running_id = util::coroutine_running(coroutineSchedule.schedule);
				if (running_id >= 0)//抢占式(唤醒操作时一般是不会有正在运行的协程)
				{
					LOG4_TRACE("%s coroutine_yield running_id(%d)", __FUNCTION__, running_id);
					util::coroutine_yield(coroutineSchedule.schedule);//放弃执行权
				}
				LOG4_TRACE("%s coroutine_resume coid(%d) status(%d)",
						__FUNCTION__, coid,util::coroutine_status(coroutineSchedule.schedule,coid));
				util::coroutine_resume(coroutineSchedule.schedule,coid);//执行函数
				int status = util::coroutine_status(coroutineSchedule.schedule,coid);
				if (0 == status)
				{
					LOG4_TRACE("%s dead coid(%d)",__FUNCTION__,coid);
					coroutineSchedule.coroutineIds.erase(coroutineSchedule.coroutineRunIter++);
					auto argIter = m_CoroutineScheduleArgs.find(coid);
					if (argIter != m_CoroutineScheduleArgs.end())
					{
						LOG4_INFO("%s destroy arg for coid(%d) status(%d)",__FUNCTION__,coid,nStatus);
						delete argIter->second;
						m_CoroutineScheduleArgs.erase(argIter);
					}
					continue;
				}
			}
			++coroutineSchedule.coroutineRunIter;
			return true;
		}
	}
	coroutineSchedule.coroutineRunIter = coroutineSchedule.coroutineIds.end();
	LOG4_INFO("no co to run");
	return false;
}

bool Coroutine::CoroutineYield()
{
    util::schedule* schedule = m_CoroutineSchedule.schedule;
    int running_id = util::coroutine_running(schedule);
    if (running_id >= 0)
    {
        LOG4_TRACE("%s coroutine_yield running_id(%d) status(%d)",__FUNCTION__, running_id,coroutine_status(schedule,running_id));
        util::coroutine_yield(schedule);//放弃执行权
        return true;
    }
    else
    {
        LOG4_WARN("%s no running coroutine", __FUNCTION__);
        return false;
    }
    return true;
}
int Coroutine::CoroutineRunning()
{
	int running_id(0);
	running_id = util::coroutine_running(m_CoroutineSchedule.schedule);
	LOG4_TRACE("%s coroutine_status running_id(%d)",__FUNCTION__, running_id);
	return running_id;
}

unsigned int Coroutine::CoroutineTaskSize()
{
	return m_CoroutineSchedule.coroutineIds.size();
}
int Coroutine::CoroutineStatus(int coid)
{
	if (coid >= 0)
	{
		return util::coroutine_status(m_CoroutineSchedule.schedule,coid);
	}
	LOG4_WARN("%s invalid coid(%d)", __FUNCTION__,coid);
	return 0;
}

bool Coroutine::CoroutineResume(int coid)
{
	if (coid >= 0)
	{
		util::coroutine_resume(m_CoroutineSchedule.schedule,coid);
		int status = util::coroutine_status(m_CoroutineSchedule.schedule,coid);
		if (0 == status)
		{
			m_CoroutineSchedule.coroutineIds.erase(coid);
		}
		return true;
	}
	LOG4_WARN("%s invalid coid(%d) coroutineName(%u)", __FUNCTION__,coid);
	return false;
}

}
