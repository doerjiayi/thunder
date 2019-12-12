/*******************************************************************************
* Project:  Net
* @file     NetUtil.hpp
* @brief
* @author   cjy
* @date:    2019年7月27日
* @note
* Modify history:
******************************************************************************/
#ifndef NETUTIL_HPP_
#define NETUTIL_HPP_
#include <time.h>
#include <sys/time.h>
#include <memory.h>
#include <string>
#include "util/json/CJsonObject.hpp"
#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"

#define DEFINE_INT \
typedef net::uint8 uint8;\
typedef net::uint16 uint16;\
typedef net::uint32 uint32;\
typedef net::uint64 uint64;\
typedef net::int8 int8;\
typedef net::int16 int16;\
typedef net::int32 int32;\
typedef net::int64 int64;

namespace net
{

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int int64;
typedef unsigned long long int uint64;

struct SendRate
{
	SendRate(){Reset();}
	SendRate(const std::string &strName){Reset();SetName(strName);}
	void Reset()
	{
		m_uiSendCounter = m_uiLastSendCounter = m_uiSuccCounter = m_uiRecvCounter = m_uiLastRecvCounter = m_uiCounter = m_uiLastCounter = 0;
		gettimeofday(&m_tvRunBeginClock,NULL);gettimeofday(&m_tvRunEndClock,NULL);
	}
	void SetName(const std::string &strName){m_strName = strName;}
	std::string m_strName;

	timeval m_tvRunBeginClock;
	timeval m_tvRunEndClock;
	uint64 m_uiSendCounter;
	uint64 m_uiLastSendCounter;
	uint64 m_uiSuccCounter;

	uint64 m_uiRecvCounter;
	uint64 m_uiLastRecvCounter;

	uint64 m_uiCounter;
	uint64 m_uiLastCounter;

	float LastUseTime();
	uint64 RecvNotSucc()const;
	uint64 NewRecvCount()const;
	uint64 SendNotSucc()const;
	uint64 NewSendCount()const;
	uint64 NewCounter()const;
	void IncrSucc(int count=0);
	void IncrSend(int count=0);
	void IncrRecv(int count=0);
	void IncrCounter(int count=0);
	void CheckSendRate();
	void CheckRecvRate();
	void CheckCounter();
};

//函数运行时间计算类
class RunClock
{
public:
	RunClock();
	~RunClock();
	void Reset();
    void StartClock(const char* desc);
    void StartClock(int nStage);
    void StartClock();
    void EndClock();
    void TotalRunTime();
    float LastUseTime();
    float TotalUseTime();
    bool boStart;
    timeval m_tvRunBeginClock;
    timeval m_tvRunEndClock;

    timeval m_tvTotalBeginClock;
	timeval m_tvTotalEndClock;
    char m_desc[256];
};

struct BUFF_RW
{
    BUFF_RW(): m_pbuffer(NULL), size(0),indexW(0),indexR(0)
    {
    }
    ~BUFF_RW()
    {
        if (m_pbuffer)
        {
            ::free(m_pbuffer);
        }
    }
    char* m_pbuffer;
    uint32 size;
    uint32 indexW;
    uint32 indexR;
    inline void Clear()
    {
        indexR = indexW = 0;
    }
    inline void Resize(uint32 buffsize)
    {
        if (buffsize > 0)
        {
            if (size < buffsize)//容量只会扩大
            {
                m_pbuffer = (char*) ::realloc(m_pbuffer, buffsize);
                size = buffsize;
            }
        }
    }
    inline void Write(const char* data,int dataSize)
    {
        if ((indexW + dataSize) > size)
        {
            Resize(indexW + dataSize);
        }
        memcpy(m_pbuffer + indexW,data,dataSize);
        indexW += dataSize;
    }
    inline bool Read(char* data,unsigned int dataSize)
    {
        if ((indexR + dataSize) > indexW)
        {
            return false;
        }
        memcpy(data,m_pbuffer + indexR,dataSize);
        indexR += dataSize;
        return true;
    }
    inline char* Getbuff() const
    {
        return m_pbuffer;
    }
    inline char* GetWriteBuff() const
    {
        return m_pbuffer + indexW;
    }
    inline char* GetReadBuff() const
    {
        return m_pbuffer + indexR;
    }
    inline void AdvanceIndexW(uint32 s)
    {
        if (indexW + s <= size)
        {
            indexW += s;
        }
    }
    void RewindR(uint32 s)
    {
        if (s > 0 && indexR > s)
        {
            indexR -= s;
        }
    }

    uint32 GetIndexW()const {return indexW;}
    uint32 GetIndexR()const {return indexR;}
    uint32 GetSize()const {return size;}
    uint32 ReadDataLen()const
    {
        if (indexW > indexR)
        {
            return indexW - indexR;
        }
        return 0;
    }
};

}

#endif /* NETUTIL_HPP_ */
