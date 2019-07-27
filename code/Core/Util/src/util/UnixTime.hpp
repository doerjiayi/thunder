/*******************************************************************************
* Project:  DataAnalysis
* File:     UnixTime.hpp
* Description:   loss库time_t型时间处理API
* Author:        chenjiayi
* Created date:  2010-12-14
* Modify history:
*******************************************************************************/

#ifndef UNIXTIME_HPP_
#define UNIXTIME_HPP_
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iostream>
#include <string>
#include <sstream>
#include <sys/time.h>
#include "CommonUtils.hpp"

namespace util
{

time_t GetCurrentTime();
std::string GetCurrentTime(int iTimeSize);

time_t TimeStr2time_t(const char* szTimeStr);
time_t TimeStr2time_t(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

const std::string time_t2TimeStr(
        time_t lTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//本地时间与格林尼治时间相隔的秒数，返回正数为东时区，负数为西时区
long LocalTimeDiffGmTime();

//两时间之间相隔的秒数
long DiffTime(
        const std::string& strTime1,
        const std::string& strTime2,
        const std::string& strTimeFormat1 = "YYYY-MM-DD HH:MI:SS",
        const std::string& strTimeFormat2 = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在小时开始一刻（00:00）
time_t GetBeginTimeOfTheHour(time_t lTime);
const std::string GetBeginTimeOfTheHour(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在小时最后一刻（59:59）
time_t GetEndTimeOfTheHour(time_t lTime);
const std::string GetEndTimeOfTheHour(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在天的开始一刻（00:00:00）
time_t GetBeginTimeOfTheDay(time_t lTime);
const std::string GetBeginTimeOfTheDay(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在天的最后一刻（23:59:59）
time_t GetEndTimeOfTheDay(time_t lTime);
const std::string GetEndTimeOfTheDay(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在月的开始一刻（00:00:00）
time_t GetBeginTimeOfTheWeek(time_t lTime);
const std::string GetBeginTimeOfTheWeek(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在月的开始一刻（00:00:00）
time_t GetBeginTimeOfTheMonth(time_t lTime);
const std::string GetBeginTimeOfTheMonth(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间所在月的最后一刻（23:59:59）
time_t GetEndTimeOfTheMonth(time_t lTime);
const std::string GetEndTimeOfTheMonth(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取指定时间下一个月的开始一刻（00:00:00）
time_t GetBeginTimeOfNextMonth(time_t lTime);
const std::string GetBeginTimeOfNextMonth(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//取相对于指定时间nDaysBefore天前的时间，如果nDaysBefore为负数
//则取的是相对于指定时间nDaysBefore天后的时间
time_t GetDaysBefore(
        time_t lTime,
        int iDaysBefore);
const std::string GetDaysBefore(
        const std::string& strTime,
        int iDaysBefore,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

//获取指定时间所在月共有多少天
int GetTotalDaysOfTheMonth(time_t lTime);
int GetTotalDaysOfTheMonth(
        const std::string& strTime,
        const std::string& strTimeFormat = "YYYY-MM-DD HH:MI:SS");

unsigned long long GetMicrosecond();

typedef struct _SYSTEMTIME
{
    uint16 wYear;
    uint16 wMonth;
    uint16 wDayOfWeek;
    uint16 wDay;
    uint16 wHour;
    uint16 wMinute;
    uint16 wSecond;
    uint16 wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME;

inline void GetLocalTime(SYSTEMTIME *pSysTime)
{
    if (!pSysTime)
    {
        return;
    }
    time_t t = ::time(0);
    tm _tm;
    localtime_r(&t, &_tm);
    pSysTime->wYear = _tm.tm_year + 1900;
    pSysTime->wMonth = _tm.tm_mon + 1;
    pSysTime->wDay = _tm.tm_mday;
    pSysTime->wHour = _tm.tm_hour;
    pSysTime->wMinute = _tm.tm_min;
    pSysTime->wSecond = _tm.tm_sec;
    pSysTime->wMilliseconds = 0;
}

inline void GetLocalTime(SYSTEMTIME *pSysTime,time_t t)
{
    if (!pSysTime)
    {
        return;
    }
    tm _tm;
    localtime_r(&t, &_tm);
    pSysTime->wYear = _tm.tm_year + 1900;
    pSysTime->wMonth = _tm.tm_mon + 1;
    pSysTime->wDay = _tm.tm_mday;
    pSysTime->wHour = _tm.tm_hour;
    pSysTime->wMinute = _tm.tm_min;
    pSysTime->wSecond = _tm.tm_sec;
    pSysTime->wMilliseconds = 0;
}

//根据获取时间字符串
inline void GetTimeStampMinuteStr(char* pBuff, uint16 buffSize)
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);
    snprintf(pBuff, buffSize, "%04u%02u%02u%02u%02u", systemTime.wYear,
                    systemTime.wMonth, systemTime.wDay, systemTime.wHour,
                    systemTime.wMinute);
}
//根据获取时间字符串(缓存最少16字节)
inline void GetTimeStampMillisecondStr(char* pBuff, uint16 buffSize)
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);
    snprintf(pBuff, buffSize - 1, "%04u%02u%02u%02u%02u%02u%03u", systemTime.wYear,
                    systemTime.wMonth, systemTime.wDay, systemTime.wHour,
                    systemTime.wMinute,systemTime.wSecond,systemTime.wMilliseconds);
}
inline void GetTimeStampSecStrCheck(char* pBuff, uint16 buffSize)
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);
    snprintf(pBuff, buffSize, "%04u:%02u:%02u:%02u:%02u:%02u", systemTime.wYear,
                    systemTime.wMonth, systemTime.wDay, systemTime.wHour,
                    systemTime.wMinute, systemTime.wSecond);
}
//根据时间间隔获取时间字符串
inline void GetTimeStampMinuteStrByTimeInterval(char* pBuff, uint16 buffSize,
                uint32 timeInterval)
{
    time_t t = ::time(0);
    t = (t / timeInterval) * timeInterval;
    tm _tm;
    localtime_r(&t, &_tm);
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);
    snprintf(pBuff, buffSize - 1, "%04u%02u%02u%02u%02u", systemTime.wYear,
                    systemTime.wMonth, systemTime.wDay, systemTime.wHour,
                    systemTime.wMinute);
}

//根据获取日期字符串
inline void GetDateStr(char* pBuff, uint16 buffSize)
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);
    snprintf(pBuff, buffSize, "%04u%02u%02u", systemTime.wYear,
                    systemTime.wMonth, systemTime.wDay);
}

inline void GetDateStr(char* pBuff, uint16 buffSize,time_t t)
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime,t);
    snprintf(pBuff, buffSize, "%04u%02u%02u", systemTime.wYear,
                    systemTime.wMonth, systemTime.wDay);
}

inline uint32 GetDateUint32(time_t t)
{
    char sDate[32];
    GetDateStr(sDate,sizeof(sDate),t);
    return ::strtoul(sDate,NULL,10);
}

//根据获取时间字符串
inline void GetMinuteStr(char* pBuff, uint16 buffSize,time_t t)
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime,t);
    snprintf(pBuff, buffSize, "%04u%02u%02u%02u%02u", systemTime.wYear,
                    systemTime.wMonth, systemTime.wDay, systemTime.wHour,
                    systemTime.wMinute);
}

//自定义排序函数
inline bool SortStringByTime(const std::string &v1, const std::string &v2) //注意：本函数的参数的类型一定要与vector中元素的类型一致
{
    //字符串格式为如 LogData201511241139.fd ，数字为时间
    return v1 < v2; //升序排列
}

inline bool SortStringByTimeDesc(const std::string &v1, const std::string &v2) //注意：本函数的参数的类型一定要与vector中元素的类型一致
{
    //字符串格式为如 LogData201511241139.fd ，数字为时间
    return v1 > v2; //降序排列
}

}

#endif /* UNIXTIME_HPP_ */
