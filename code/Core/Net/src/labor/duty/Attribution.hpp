/*******************************************************************************
* Project:  Thunder
* @file     Attribute.hpp
* @brief 
* @author   cjy
* @date:    2016年4月27日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_LABOR_DUTY_ATTRIBUTION_HPP_
#define SRC_LABOR_DUTY_ATTRIBUTION_HPP_

#include <stdlib.h>
#include "libev/ev.h"
#include "util/CBuffer.hpp"
#include "util/StreamCodec.hpp"
#include "util/json/CJsonObject.hpp"

/**
 * @brief 工作进程属性
 */
struct tagWorkerAttr
{
    int iWorkerIndex;                   ///< 工作进程序号
    int iControlFd;                     ///< 与Manager进程通信的文件描述符（控制流）
    int iDataFd;                        ///< 与Manager进程通信的文件描述符（数据流）
    int32 iLoad;                        ///< 负载
    int32 iConnect;                     ///< 连接数量
    int32 iRecvNum;                     ///< 接收数据包数量
    int32 iRecvByte;                    ///< 接收字节数
    int32 iSendNum;                     ///< 发送数据包数量
    int32 iSendByte;                    ///< 发送字节数
    int32 iClientNum;                   ///< 客户端数量
    ev_tstamp dBeatTime;                ///< 心跳时间

    tagWorkerAttr()
    {
        iWorkerIndex = 0;
        iControlFd = -1;
        iDataFd = -1;
        iLoad = 0;
        iConnect = 0;
        iRecvNum = 0;
        iRecvByte = 0;
        iSendNum = 0;
        iSendByte = 0;
        iClientNum = 0;
        dBeatTime = time(NULL);
    }

    tagWorkerAttr(const tagWorkerAttr& stAttr)
    {
        iWorkerIndex = stAttr.iWorkerIndex;
        iControlFd = stAttr.iControlFd;
        iDataFd = stAttr.iDataFd;
        iLoad = stAttr.iLoad;
        iConnect = stAttr.iConnect;
        iRecvNum = stAttr.iRecvNum;
        iRecvByte = stAttr.iRecvByte;
        iSendNum = stAttr.iSendNum;
        iSendByte = stAttr.iSendByte;
        iClientNum = stAttr.iClientNum;
        dBeatTime = stAttr.dBeatTime;
    }

    tagWorkerAttr& operator=(const tagWorkerAttr& stAttr)
    {
        iWorkerIndex = stAttr.iWorkerIndex;
        iControlFd = stAttr.iControlFd;
        iDataFd = stAttr.iDataFd;
        iLoad = stAttr.iLoad;
        iConnect = stAttr.iConnect;
        iRecvNum = stAttr.iRecvNum;
        iRecvByte = stAttr.iRecvByte;
        iSendNum = stAttr.iSendNum;
        iSendByte = stAttr.iSendByte;
        iClientNum = stAttr.iClientNum;
        dBeatTime = stAttr.dBeatTime;
        return(*this);
    }
};
enum ConnectStatus
{
    eConnectStatus_init = 0,
    eConnectStatus_connecting = 1,
    eConnectStatus_ok = 2,
};
/**
 * @brief 连接属性
 * @note  连接属性，因内部带有许多指针，并且没有必要提供深拷贝构造，所以不可以拷贝，也无需拷贝
 */
struct tagConnectionAttr
{
    /**
     * @brief 连接状态
     * @note 连接状态 （复用发起连接CMD：
     *  0 connect未返回结果，
     *  CMD_REQ_CONNECT_TO_WORKER 发起连接worker
     *  CMD_RSP_CONNECT_TO_WORKER 已得到对方manager响应，转给worker
     *  CMD_REQ_TELL_WORKER 将己方worker信息通知对方worker
     *  CMD_RSP_TELL_WORKER 收到对方worker响应，连接已就绪）
     */
    unsigned char ucConnectStatus;
    util::CBuffer* pRecvBuff;           ///< 在结构体析构时回收
    util::CBuffer* pSendBuff;           ///< 在结构体析构时回收
    util::CBuffer* pWaitForSendBuff;    ///< 等待发送的数据缓冲区（数据到达时，连接并未建立，等连接建立并且pSendBuff发送完毕后立即发送）
    util::CBuffer* pClientData;         ///< 客户端相关数据（例如IM里的用户昵称、头像等，登录或连接时保存起来，后续发消息或其他操作无须客户端再带上来）
    char pRemoteAddr[32];                  ///< 对端IP地址（不是客户端地址，但可能跟客户端地址相同）
    util::E_CODEC_TYPE eCodecType;      ///< 协议（编解码）类型
    ev_tstamp dActiveTime;              ///< 最后一次访问时间
    ev_tstamp dKeepAlive;               ///< 连接保持时间，默认值0为用心跳保持的长连接，大于0的值不做心跳检查，时间到即断连接,小于0为收完数据立即断开连接（主要用于http连接）
    int iFd;						///< 文件描述符
    uint32 ulSeq;                       ///< 文件描述符创建时对应的序列号
    uint32 ulForeignSeq;                ///< 外来的seq，每个连接的包都是有序的，用作接入Server数据包检查，防止篡包
    uint32 ulMsgNumUnitTime;            ///< 统计单位时间内发送消息数量
    uint32 ulMsgNum;                    ///< 发送消息数量
    std::string strIdentify;            ///< 连接标识（可以为空，不为空时用于标识业务层与连接的关系）
    ev_io* pIoWatcher;                  ///< 不在结构体析构时回收
    ev_timer* pTimeWatcher;             ///< 不在结构体析构时回收

    tagConnectionAttr()
        : ucConnectStatus(0), pRecvBuff(NULL), pSendBuff(NULL), pWaitForSendBuff(NULL), pClientData(NULL),
          eCodecType(util::CODEC_PROTOBUF),dActiveTime(0), dKeepAlive(0),iFd(0),ulSeq(0), ulForeignSeq(0), ulMsgNumUnitTime(0), ulMsgNum(0),
          pIoWatcher(NULL), pTimeWatcher(NULL)
    {
    	pRemoteAddr[0] = 0;
    }

    ~tagConnectionAttr()
    {
    	SAFE_DELETE(pRecvBuff);
    	SAFE_DELETE(pSendBuff);
    	SAFE_DELETE(pWaitForSendBuff);
    	SAFE_DELETE(pClientData);
    }
};

#endif /* SRC_LABOR_DUTY_ATTRIBUTION_HPP_ */
