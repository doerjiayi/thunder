/*
 * CommonUtils.hpp
 *
 *  Created on: 2017年1月17日
 *      Author: chen
 */

#ifndef CODE_LOSS_SRC_UTIL_COMMONUTILS_HPP_
#define CODE_LOSS_SRC_UTIL_COMMONUTILS_HPP_
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iostream>
#include <string>
#include <sstream>
#include <sys/time.h>

namespace thunder
{

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int int64;
typedef unsigned long long int uint64;

#define MAKE_FOUR_CC(ch0, ch1, ch2, ch3) ((uint32)(uint8)(ch0) | ((uint32)(uint8)(ch1) << 8) | ((uint32)(uint8)(ch2) << 16) | ((uint32)(uint8)(ch3) << 24 ))

//生成session_id
#define MAKE_SESSION_ID(appid,userid) ((unsigned long long)(unsigned int)(userid) | ((unsigned long long)(unsigned int)(appid) << 32))
//从session_id获取userid
#define USERID_FROM_SESSION_ID(session_id) ((unsigned int)(session_id & 0xFFFFFFFF))
//从session_id获取appid
#define APPID_FROM_SESSION_ID(session_id) ((unsigned int)((session_id & 0xFFFFFFFF00000000) >> 32))
//使用uint64_0 的48位和uint16_0的16位
#define MAKE_UINT64_FROM_UINT16_UINT64(uint16_0, uint64_0) ((uint64)(uint16)(uint16_0) << 48 | ((uint64)(uint64_0 & 0xFFFFFFFFFFFF) ))

//获取唯一id
inline uint64 GetUniqueId(unsigned short uiNodeId, unsigned char ucWorkerIndex)
{
    const unsigned long long ullSequenceBit =              0xFFFF;
    const unsigned long long ullWorkerIndexBit =         0xFF0000;
    const unsigned long long ullNodeIndexBit =         0xFF000000;
    const unsigned long long ullTimeIndexBit = 0xFFFFFFFF00000000;

    static unsigned int uiSequence = 0;
    unsigned long long ullTime = ::time(NULL);//1484276747
    ++uiSequence;
    uiSequence &= ullSequenceBit;
    //4294967296    256   256   65536(0xFFFF)
    uint64 ullUniqueId = (uint64(ullTime << 32) & ullTimeIndexBit) | \
                    (uint64(uiNodeId << 24) & ullNodeIndexBit) | (uint64(ucWorkerIndex << 16) & ullWorkerIndexBit) | \
                    (uint64(uiSequence) & ullSequenceBit);
    return ullUniqueId;
}

inline void GetUniqueIdStr(char* pBuff, uint16 buffSize,unsigned short uiNodeId, unsigned char ucWorkerIndex)
{
    const unsigned long long ullSequenceBit =              0xFFFF;
    const unsigned long long ullWorkerIndexBit =         0xFF0000;
    const unsigned long long ullNodeIndexBit =         0xFF000000;
    const unsigned long long ullTimeIndexBit = 0xFFFFFFFF00000000;

    static unsigned int uiSequence = 0;
    unsigned long long ullTime = ::time(NULL);//1484276747
    ++uiSequence;
    uiSequence &= ullSequenceBit;
    //4294967296    256   256   65536(0xFFFF)
    uint64 ullUniqueId = (uint64(ullTime << 32) & ullTimeIndexBit) | \
                    (uint64(uiNodeId << 24) & ullNodeIndexBit) | (uint64(ucWorkerIndex << 16) & ullWorkerIndexBit) | \
                    (uint64(uiSequence) & ullSequenceBit);
    snprintf(pBuff, buffSize - 1,"%llu",ullUniqueId);
}

inline uint64 TimeToUniqueId(unsigned long long ullTime)
{
    const unsigned long long ullTimeIndexBit = 0xFFFFFFFF00000000;
    uint64 ullUniqueId = (uint64(ullTime << 32) & ullTimeIndexBit);
    return ullUniqueId;
}

}//namespace thunder

#endif /* CODE_LOSS_SRC_UTIL_COMMONUTILS_HPP_ */
