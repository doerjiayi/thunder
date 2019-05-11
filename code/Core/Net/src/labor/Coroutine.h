#ifndef SRC_Coroutine_HPP_
#define SRC_Coroutine_HPP_
#include <set>
#include <map>
#include <unordered_map>
#include "coroutine/coroutine.h"

namespace net
{

struct tagCoroutineArg//自定义协程参数需要继承tagCoroutineArg
{
};

struct tagCoroutineSchedule
{
	tagCoroutineSchedule():schedule(NULL),coroutineRunIter(0){}
	tagCoroutineSchedule(const tagCoroutineSchedule& coroutine)
	{
		schedule = coroutine.schedule;
		coroutineIds = coroutine.coroutineIds;
		coroutineRunIter = coroutine.coroutineRunIter;
	}
	const tagCoroutineSchedule& operator = (const tagCoroutineSchedule& coroutine)
	{
		schedule = coroutine.schedule;
		coroutineIds = coroutine.coroutineIds;
		coroutineRunIter = coroutine.coroutineRunIter;
		return *this;
	}
	util::schedule* schedule;
	std::set<int> coroutineIds;
	std::set<int>::iterator coroutineRunIter;
};

/**
 * @brief 协程封装
 */
class Coroutine
{
public:
	Coroutine();
    virtual ~Coroutine();
	bool CoroutineResumeWithTimes(unsigned int nMaxTimes=0);//唤醒协程次数 (自定义调度策略，nMaxTimes最大执行协程次数，0则执行所有的协程).返回true 还有需要执行的协程，返回false没有还需要执行的协程
	bool CoroutineNewWithArg(util::coroutine_func func,tagCoroutineArg *arg);//创建一个协程
	int CoroutineNew(util::coroutine_func func,void *ud);//创建一个协程(自定义参数)
	bool CoroutineResume();//自定义调用策略,轮流执行规则
	bool CoroutineResume(int coid);//唤醒指定协程
	int CoroutineStatus(int coid);//返回协程状态
	unsigned int CoroutineTaskSize();
	bool CoroutineYield();//在协程函数中放弃协程的本次执行
	int CoroutineRunning();//获取正在进行的协程的id
private:
	tagCoroutineSchedule m_CoroutineSchedule;
	std::unordered_map<int,tagCoroutineArg*> m_CoroutineScheduleArgs;
};

}

#endif /* SRC_Coroutine_HPP_ */
