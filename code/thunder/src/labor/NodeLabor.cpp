/*******************************************************************************
 * Project:  AsyncServer
 * @file     NodeLabor.cpp
 * @brief 
 * @author   cjy
 * @date:    2017年9月6日
 * @note
 * Modify history:
 ******************************************************************************/
#include "NodeLabor.hpp"

namespace thunder
{

NodeLabor::NodeLabor()
{
}

NodeLabor::~NodeLabor()
{
//	coroutine_close(m_pCoroutineSchedule);
}

int NodeLabor::CoroutineNew(const std::string &coroutineName,llib::coroutine_func func,void *ud)
{
	CoroutineScheduleMap::iterator it = m_pCoroutineScheduleMap.find(coroutineName);
	llib::schedule* schedule(NULL);
	if (it == m_pCoroutineScheduleMap.end())
	{
		CoroutineSchedule coroutineSchedule;
		schedule = coroutineSchedule.schedule = llib::coroutine_open();
		std::pair<CoroutineScheduleMap::iterator,bool> ret = m_pCoroutineScheduleMap.insert(std::make_pair(coroutineName,coroutineSchedule));
		if (!ret.second)
		{
			LOG4_ERROR("%s failed to create schedule", __FUNCTION__);
			coroutine_close(schedule);
			return -1;
		}
		it = ret.first;
	}
	else
	{
		schedule = it->second.schedule;
	}
	int coid = llib::coroutine_new(schedule, func, ud);//使用指针，因为栈内存会被保存
	if (coid >= 0)
	{
		it->second.coroutineIds.push_back(coid);
		LOG4_TRACE("%s coroutine coid(%u) status(%d) coroutineName(%s)",
				__FUNCTION__,coid,llib::coroutine_status(schedule,coid),coroutineName.c_str());
	}
	else
	{
		LOG4_ERROR("%s coroutine invalid coid(%u)", __FUNCTION__, coid);
	}
	return coid;
}

bool NodeLabor::CoroutineResume(const std::string &coroutineName)
{
	CoroutineScheduleMap::iterator it = m_pCoroutineScheduleMap.find(coroutineName);
	if (it == m_pCoroutineScheduleMap.end())
	{
		LOG4_ERROR("%s failed to get CoroutineSchedule coroutineName(%u)", __FUNCTION__, coroutineName.c_str());
		return false;
	}
	CoroutineSchedule& coroutineSchedule = it->second;
	while (coroutineSchedule.coroutineIds.size() > 0)
	{
		if (coroutineSchedule.uiCoroutineRunIndex >= coroutineSchedule.coroutineIds.size())
		{
			coroutineSchedule.uiCoroutineRunIndex = 0;
		}
		int coid = coroutineSchedule.coroutineIds[coroutineSchedule.uiCoroutineRunIndex];
		if (0 > coid)
		{
			LOG4_ERROR("%s invaid coid(%d)",__FUNCTION__,coid);
			coroutineSchedule.coroutineIds.erase(
					coroutineSchedule.coroutineIds.begin() + coroutineSchedule.uiCoroutineRunIndex);
			continue;
		}
		int nStatus = llib::coroutine_status(coroutineSchedule.schedule,coid);
		LOG4_TRACE("%s coroutine_status coid(%d) status(%d)",__FUNCTION__,coid,nStatus);
		if (0 == nStatus)
		{
			LOG4_TRACE("%s dead coid(%d)",__FUNCTION__,coid);
			coroutineSchedule.coroutineIds.erase(coroutineSchedule.coroutineIds.begin() + coroutineSchedule.uiCoroutineRunIndex);
			continue;
		}
		{
			LOG4CPLUS_TRACE_FMT(GetLogger(), "%s CoroutineResume coid(%d)",__FUNCTION__,coid);
			{
				int running_id = llib::coroutine_running(coroutineSchedule.schedule);
				if (running_id >= 0)//抢占式(唤醒操作时一般是不会有正在运行的协程)
				{
					LOG4_TRACE("%s coroutine_yield running_id(%d)", __FUNCTION__, running_id);
					llib::coroutine_yield(coroutineSchedule.schedule);//放弃执行权
				}
				LOG4_TRACE("%s coroutine_resume coid(%d) status(%d)",
						__FUNCTION__, coid,llib::coroutine_status(coroutineSchedule.schedule,coid));
				llib::coroutine_resume(coroutineSchedule.schedule,coid);//执行函数
				int status = llib::coroutine_status(coroutineSchedule.schedule,coid);
				if (0 == status)
				{
					coroutineSchedule.coroutineIds.erase(coroutineSchedule.coroutineIds.begin() + coroutineSchedule.uiCoroutineRunIndex);
				}
			}
			++coroutineSchedule.uiCoroutineRunIndex;
			return true;
		}
	}
	LOG4CPLUS_DEBUG_FMT(GetLogger(), "no co to run");
	return false;
}

void NodeLabor::CoroutineYield(const std::string &coroutineName)
{
	CoroutineScheduleMap::iterator it = m_pCoroutineScheduleMap.find(coroutineName);
	if (it == m_pCoroutineScheduleMap.end())
	{
		LOG4_ERROR("%s failed to get m_pCoroutineScheduleMap coroutineName(%u)", __FUNCTION__, coroutineName.c_str());
		return;
	}
	llib::schedule* schedule = it->second.schedule;
	int running_id = llib::coroutine_running(schedule);
	if (running_id >= 0)
	{
		LOG4_TRACE("%s coroutine_yield running_id(%d) status(%d)",
				__FUNCTION__, running_id,coroutine_status(schedule,running_id));
		llib::coroutine_yield(schedule);//放弃执行权
	}
	else
	{
		LOG4_WARN("%s no running coroutine", __FUNCTION__);
	}
}

int NodeLabor::CoroutineRunning(const std::string &coroutineName)
{
	CoroutineScheduleMap::iterator it = m_pCoroutineScheduleMap.find(coroutineName);
	if (it == m_pCoroutineScheduleMap.end())
	{
		LOG4_ERROR("%s failed to get CoroutineSchedule coroutineName(%u)",
				__FUNCTION__, coroutineName.c_str());
		return -1;
	}
	int running_id = llib::coroutine_running(it->second.schedule);
	LOG4_TRACE("%s coroutine_status running_id(%d) coroutineName(%s)",
			__FUNCTION__, running_id,coroutineName.c_str());
	return running_id;
}

uint32 NodeLabor::CoroutineTaskSize(const std::string &coroutineName)
{
	CoroutineScheduleMap::const_iterator it = m_pCoroutineScheduleMap.find(coroutineName);
	if (it == m_pCoroutineScheduleMap.end())
	{
		LOG4_WARN("%s failed to get coroutineName(%u)", __FUNCTION__,coroutineName.c_str());
		return 0;
	}
	return it->second.coroutineIds.size();
}
int NodeLabor::CoroutineStatus(const std::string &coroutineName,int coid)
{
	CoroutineScheduleMap::const_iterator it = m_pCoroutineScheduleMap.find(coroutineName);
	if (it == m_pCoroutineScheduleMap.end())
	{
		LOG4_WARN("%s failed to get coroutineName(%u)", __FUNCTION__,coroutineName.c_str());
		return 0;
	}
	if (coid >= 0)
	{
		return llib::coroutine_status(it->second.schedule,coid);
	}
	LOG4_WARN("%s invalid coid(%d) coroutineName(%u)", __FUNCTION__,coid,coroutineName.c_str());
	return 0;
}

bool NodeLabor::CoroutineResume(const std::string &coroutineName,int coid)
{
	CoroutineScheduleMap::const_iterator it = m_pCoroutineScheduleMap.find(coroutineName);
	if (it == m_pCoroutineScheduleMap.end())
	{
		LOG4_WARN("%s failed to get coroutineName(%u)", __FUNCTION__,coroutineName.c_str());
		return false;
	}
	if (coid >= 0)
	{
		llib::coroutine_resume(it->second.schedule,coid);
		return true;
	}
	LOG4_WARN("%s invalid coid(%d) coroutineName(%u)", __FUNCTION__,coid,coroutineName.c_str());
	return false;
}

} /* namespace thunder */
