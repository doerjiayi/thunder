/*******************************************************************************
* Project:  AsyncServer
* @file     ThunderDefine.hpp
* @brief 
* @author   cjy
* @date:    2015年7月27日
* @note
* Modify history:
******************************************************************************/
#ifndef THUNDERDEFINE_HPP_
#define THUNDERDEFINE_HPP_


#ifndef NODE_BEAT
#define NODE_BEAT 1.0
#endif

#define LOG4_FATAL(args...) LOG4CPLUS_FATAL_FMT(GetLogger(), ##args)
#define LOG4_ERROR(args...) LOG4CPLUS_ERROR_FMT(GetLogger(), ##args)
#define LOG4_WARN(args...) LOG4CPLUS_WARN_FMT(GetLogger(), ##args)
#define LOG4_INFO(args...) LOG4CPLUS_INFO_FMT(GetLogger(), ##args)
#define LOG4_DEBUG(args...) LOG4CPLUS_DEBUG_FMT(GetLogger(), ##args)
#define LOG4_TRACE(args...) LOG4CPLUS_TRACE_FMT(GetLogger(), ##args)

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

/** @brief 心跳间隔时间（单位:秒） */
const int gc_iBeatInterval = NODE_BEAT;

/** @brief 每次epoll_wait能处理的最大事件数  */
const int gc_iMaxEpollEvents = 100;

/** @brief 最大缓冲区大小 */
const int gc_iMaxBuffLen = 65535;

/** @brief 错误信息缓冲区大小 */
const int gc_iErrBuffLen = 256;

const uint32 gc_uiMsgHeadSize = 15;
const uint32 gc_uitagCustomMsgHeadSize = 14;

enum E_CMD_STATUS
{
    STATUS_CMD_START            = 0,    ///< 创建命令执行者，但未开始执行
    STATUS_CMD_RUNNING          = 1,    ///< 正在执行命令
    STATUS_CMD_COMPLETED        = 2,    ///< 命令执行完毕
    STATUS_CMD_OK               = 3,    ///< 单步OK，但命令最终状态仍需调用者判断
    STATUS_CMD_FAULT            = 4,    ///< 命令执行出错并且不必重试
};

/**
 * @brief 消息外壳
 * @note 当外部请求到达时，因Server所有操作均为异步操作，无法立刻对请求作出响应，在完成若干个
 * 异步调用之后再响应，响应时需要有请求通道信息MsgShell。接收请求时在原消息前面加上
 * MsgShell，响应消息从通过MsgShell里的信息原路返回给请求方。若通过MsgShell
 * 里的信息无法找到请求路径，则表明请求方已发生故障或已超时被清理，消息直接丢弃。
 */
struct MsgShell
{
    int32 iFd;          ///< 请求消息来源文件描述符
    uint32 ulSeq;      ///< 文件描述符创建时对应的序列号

    MsgShell() : iFd(0), ulSeq(0)
    {
    }

    MsgShell(const MsgShell& stMsgShell) : iFd(stMsgShell.iFd), ulSeq(stMsgShell.ulSeq)
    {
    }

    MsgShell& operator=(const MsgShell& stMsgShell)
    {
        iFd = stMsgShell.iFd;
        ulSeq = stMsgShell.ulSeq;
        return(*this);
    }
};

/**
 * @brief 心跳结构
 */
struct tagBeat
{
    int32 iId;
    time_t tBeatTime;
    tagBeat()
    {
        tBeatTime = 0;
        iId = 0;
    }

    bool operator < (const tagBeat& stBeat) const
    {
        if (tBeatTime < stBeat.tBeatTime)
        {
            return(true);
        }
        else if (tBeatTime == stBeat.tBeatTime
                && iId < stBeat.iId)
        {
            return(true);
        }
        return(false);
    }
};

/**
 * @brief 序列号结构
 */
struct tagSequence
{
    int32 iId;
    uint32 ulSeq;
};

}

#endif /* THUNDERDEFINE_HPP_ */
