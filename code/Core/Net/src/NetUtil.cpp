/*******************************************************************************
* Project:  Net
* @file     NetUtil.hpp
* @brief
* @author   cjy
* @date:    2015年7月27日
* @note
* Modify history:
******************************************************************************/
#include "NetUtil.hpp"
#include "NetDefine.hpp"
#include "labor/Labor.hpp"


namespace net
{

float SendRate::LastUseTime()
{
	float useTime=1000000*(m_tvRunEndClock.tv_sec-m_tvRunBeginClock.tv_sec)+m_tvRunEndClock.tv_usec-m_tvRunBeginClock.tv_usec;
	return useTime/1000;
}
uint64 SendRate::RecvNotSucc()const{return m_uiRecvCounter - m_uiSuccCounter;}
uint64 SendRate::NewRecvCount()const{return m_uiRecvCounter - m_uiLastRecvCounter;}
uint64 SendRate::SendNotSucc()const{return m_uiSendCounter - m_uiSuccCounter;}
uint64 SendRate::NewSendCount()const{return m_uiSendCounter - m_uiLastSendCounter;}
uint64 SendRate::NewCounter()const{return m_uiCounter - m_uiLastCounter;}
void SendRate::IncrSucc(int count){if (count) m_uiSuccCounter+=count;else ++m_uiSuccCounter;}
void SendRate::IncrSend(int count){if (count) m_uiSendCounter+=count;else ++m_uiSendCounter;CheckSendRate();}
void SendRate::IncrRecv(int count) {if (count) m_uiRecvCounter+=count;else ++m_uiRecvCounter;CheckRecvRate();}
void SendRate::IncrCounter(int count) {if (count) m_uiCounter+=count;else ++m_uiCounter;CheckCounter();}

void SendRate::CheckCounter()
{
	if (m_uiCounter % 1000 == 0)
	{
		gettimeofday(&m_tvRunEndClock,NULL);
		LOG4_INFO("%s() name(%s) NewCounter(%llu) uiCounter(%llu) m_uiLastCounter(%llu) use time(%lf) ms",
				__FUNCTION__,m_strName.c_str(),NewCounter(),m_uiCounter,m_uiLastCounter,LastUseTime());
		m_uiLastCounter = m_uiCounter;
		gettimeofday(&m_tvRunBeginClock,NULL);
	}
}

void SendRate::CheckSendRate()
{
	if (m_uiSendCounter % 1000 == 0)
	{
		gettimeofday(&m_tvRunEndClock,NULL);
		LOG4_INFO("%s() name(%s) uiSendCounter(%llu) uiSuccCounter(%llu) SendNotSucc(%llu) uiLastSendCounter(%llu) NewSendCount(%llu) use time(%lf) ms",
		__FUNCTION__,m_strName.c_str(),m_uiSendCounter,m_uiSuccCounter,SendNotSucc(),m_uiLastSendCounter,NewSendCount(),LastUseTime());
		m_uiLastSendCounter = m_uiSendCounter;
		gettimeofday(&m_tvRunBeginClock,NULL);
	}
}

void SendRate::CheckRecvRate()
{
	if (m_uiRecvCounter % 1000 == 0)
	{
		gettimeofday(&m_tvRunEndClock,NULL);
		LOG4_INFO("%s() name(%s) recvCounter(%llu) succCounter(%llu) RecvNotSucc(%llu) lastRecvCounter(%llu) NewRecvCount(%llu) use time(%lf) ms",
		__FUNCTION__,m_strName.c_str(),m_uiRecvCounter,m_uiSuccCounter,RecvNotSucc(),m_uiLastRecvCounter,NewRecvCount(),LastUseTime());
		m_uiLastRecvCounter = m_uiRecvCounter;
		gettimeofday(&m_tvRunBeginClock,NULL);
	}
}

RunClock::RunClock()
{
	Reset();
}
RunClock::~RunClock()
{
	gettimeofday(&m_tvTotalEndClock,NULL);
	LOG4_TRACE("%s() RunClock use time(%lf) ms",__FUNCTION__,LastUseTime());
}

void RunClock::Reset()
{
	boStart = false;
	gettimeofday(&m_tvTotalBeginClock,NULL);
	m_tvRunBeginClock = m_tvRunEndClock = m_tvTotalEndClock = m_tvTotalBeginClock;
}
void RunClock::StartClock(const char* desc)
{
	snprintf(m_desc,sizeof(m_desc),"%s",desc);
	StartClock();
}
void RunClock::StartClock(int nStage)
{
	snprintf(m_desc,sizeof(m_desc),"stage:%d",nStage);
	StartClock();
}
void RunClock::StartClock()
{
	if (boStart)//已开始的先结束上一个周期计时
	{
		EndClock();
	}
	gettimeofday(&m_tvRunBeginClock,NULL);
	boStart = true;
}
void RunClock::EndClock()
{
	if (boStart)
	{
		gettimeofday(&m_tvRunEndClock,NULL);
		LOG4_TRACE("%s() %s use time(%lf) ms",__FUNCTION__,m_desc,LastUseTime());
		boStart = false;
	}
}

float RunClock::LastUseTime()
{
	float useTime=1000000*(m_tvRunEndClock.tv_sec-m_tvRunBeginClock.tv_sec)+m_tvRunEndClock.tv_usec-m_tvRunBeginClock.tv_usec;
	return useTime/1000;
}

float RunClock::TotalUseTime()
{
	float useTime=1000000*(m_tvTotalEndClock.tv_sec-m_tvTotalBeginClock.tv_sec)+m_tvTotalEndClock.tv_usec-m_tvTotalBeginClock.tv_usec;
	return useTime/1000;
}

void RunClock::TotalRunTime()
{
	gettimeofday(&m_tvTotalEndClock,NULL);
	LOG4_TRACE("%s() RunClock use time(%lf) ms",__FUNCTION__,TotalUseTime());
}


}

