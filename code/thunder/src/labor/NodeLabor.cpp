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
	m_pCoroutineSchedule = llib::coroutine_open();
	m_uiCoroutineRunIndex = 0;
}

NodeLabor::~NodeLabor()
{
	if (m_pCoroutineSchedule)
	{
		coroutine_close(m_pCoroutineSchedule);
	}
}

int NodeLabor::CoroutineNew(llib::coroutine_func func,void *ud)
{
	int coid = llib::coroutine_new(m_pCoroutineSchedule, func, ud);//使用指针，因为栈内存会被保存
	if (coid >= 0)
	{
		m_CoroutineIdList.push_back(coid);
		LOG4_TRACE("%s coroutine coid(%u) status(%d)",
				__FUNCTION__,coid,llib::coroutine_status(m_pCoroutineSchedule,coid));
	}
	else
	{
		LOG4_ERROR("%s coroutine invalid coid(%u)", __FUNCTION__, coid);
	}
	return coid;
}

bool NodeLabor::CoroutineResume()
{
	while (m_CoroutineIdList.size() > 0)
	{
		if (m_uiCoroutineRunIndex >= m_CoroutineIdList.size())
		{
			m_uiCoroutineRunIndex = 0;
		}
		if (0 > m_CoroutineIdList[m_uiCoroutineRunIndex])
		{
			LOG4CPLUS_ERROR_FMT(GetLogger(), "invaid coid(%d)",m_CoroutineIdList[m_uiCoroutineRunIndex]);
			m_CoroutineIdList.erase(m_CoroutineIdList.begin() + m_uiCoroutineRunIndex);
			continue;
		}
		int nStatus = CoroutineStatus(m_CoroutineIdList[m_uiCoroutineRunIndex]);
		if (0 == nStatus)
		{
			LOG4CPLUS_DEBUG_FMT(GetLogger(), "dead coid(%d)",m_CoroutineIdList[m_uiCoroutineRunIndex]);
			m_CoroutineIdList.erase(m_CoroutineIdList.begin() + m_uiCoroutineRunIndex);
			continue;
		}
		{
			LOG4CPLUS_TRACE_FMT(GetLogger(), "CoroutineResume coid(%d)",m_CoroutineIdList[m_uiCoroutineRunIndex]);
			CoroutineResume(m_CoroutineIdList[m_uiCoroutineRunIndex]);//该函数有可能会修改m_CoroutineIdList
			++m_uiCoroutineRunIndex;
			return true;
		}
	}
	LOG4CPLUS_DEBUG_FMT(GetLogger(), "no co to run");
	return false;
}

void NodeLabor::CoroutineResume(int coid,int index)
{
	int running_id = llib::coroutine_running(m_pCoroutineSchedule);
	if (running_id >= 0)//抢占式
	{
		LOG4_TRACE("%s coroutine_yield running_id(%d)", __FUNCTION__, running_id);
		llib::coroutine_yield(m_pCoroutineSchedule);//放弃执行权
	}
	LOG4_TRACE("%s coroutine_resume coid(%d) status(%d)",
			__FUNCTION__, coid,llib::coroutine_status(m_pCoroutineSchedule,coid));
	llib::coroutine_resume(m_pCoroutineSchedule,coid);//执行函数
	int status = llib::coroutine_status(m_pCoroutineSchedule,coid);
	if (0 == status)
	{
		if (index >= 0 && index < m_CoroutineIdList.size() && (m_CoroutineIdList[index] == coid))
		{
			m_CoroutineIdList.erase(m_CoroutineIdList.begin() + index);
		}
		else
		{
			std::vector<int>::iterator it = m_CoroutineIdList.begin();
			std::vector<int>::iterator itEnd = m_CoroutineIdList.end();
			for (;it != itEnd;++it)
			{
				if (*it == coid)
				{
					m_CoroutineIdList.erase(it);//直接删除，后面的任务前移,保证原来的轮询顺序
					break;
				}
			}
		}
	}
}

void NodeLabor::CoroutineYield()
{
	int running_id = llib::coroutine_running(m_pCoroutineSchedule);
	if (running_id >= 0)
	{
		LOG4_TRACE("%s coroutine_yield running_id(%d) status(%d)",
				__FUNCTION__, running_id,coroutine_status(m_pCoroutineSchedule,running_id));
		llib::coroutine_yield(m_pCoroutineSchedule);//放弃执行权
	}
	else
	{
		LOG4_WARN("%s no running coroutine", __FUNCTION__);
	}
}

int NodeLabor::CoroutineStatus(int coid)
{
	if (coid >= 0)
	{
		int status = llib::coroutine_status(m_pCoroutineSchedule,coid);
		LOG4_TRACE("%s coroutine_status coid(%d) status(%d)",
				__FUNCTION__, coid,status);
		return status;
	}
	int running_id = llib::coroutine_running(m_pCoroutineSchedule);
	if (-1 != running_id)
	{
		int status = llib::coroutine_status(m_pCoroutineSchedule,running_id);
		LOG4_TRACE("%s coroutine_status running_id(%d) status(%d)",
				__FUNCTION__, running_id,status);
		return status;
	}
	return 0;//dead
}

int NodeLabor::CoroutineRunning()
{
	int running_id = llib::coroutine_running(m_pCoroutineSchedule);
	LOG4_TRACE("%s coroutine_status running_id(%d)",
					__FUNCTION__, running_id);
	return running_id;
}

} /* namespace thunder */
