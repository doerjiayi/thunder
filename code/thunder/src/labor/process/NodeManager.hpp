/*******************************************************************************
 * Project:  AsyncServer
 * @file     NodeManager.hpp
 * @brief    Node管理者
 * @author   cjy
 * @date:    2017年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef NodeManager_HPP_
#define NodeManager_HPP_

#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>

#include "../../ThunderDefine.hpp"
#include "../../ThunderError.hpp"
#include "../process/Attribution.hpp"
#include "../process/NodeWorker.hpp"
#include "libev/ev.h"
#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/socketappender.h"
#include "log4cplus/loggingmacros.h"

#include "json/CJsonObject.hpp"
#include "CBuffer.hpp"
#include "protocol/msg.pb.h"
#include "protocol/oss_sys.pb.h"
#include "labor/NodeLabor.hpp"
#include "cmd/Cmd.hpp"

namespace thunder
{

class CmdConnectWorker;
class NodeManager;

struct tagClientConnWatcherData
{
    in_addr_t iAddr;
    NodeManager* pManager;     // 不在结构体析构时回收

    tagClientConnWatcherData() : iAddr(0), pManager(NULL)
    {
    }
};

struct tagManagerIoWatcherData
{
    int iFd;
    uint32 ulSeq;
    NodeManager* pManager;     // 不在结构体析构时回收

    tagManagerIoWatcherData() : iFd(0), ulSeq(0), pManager(NULL)
    {
    }
};

struct tagManagerWaitExitWatcherData
{
    MsgShell stMsgShell;
    uint32 cmd;
    uint32 seq;
    NodeManager* pManager;     // 不在结构体析构时回收

    tagManagerWaitExitWatcherData() : cmd(0),seq(0),pManager(NULL)
    {
    }
};

class NodeManager : public NodeLabor
{
public:
    NodeManager(const std::string& strConfFile);
    virtual ~NodeManager();

    static void SignalCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents);
    static void IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents);
    static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
    static void IoTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void PeriodicTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);  // 周期任务回调，用于替换IdleCallback
    static void WaitToExitTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents); //优雅等待关闭进程
    static void ClientConnFrequencyTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    bool ManagerTerminated(struct ev_signal* watcher);
    bool ChildTerminated(struct ev_signal* watcher);
    bool IoRead(tagManagerIoWatcherData* pData, struct ev_io* watcher);
    bool FdTransfer(int iFd);
    bool AcceptServerConn(int iFd);
    bool RecvDataAndDispose(tagManagerIoWatcherData* pData, struct ev_io* watcher);
    bool IoWrite(tagManagerIoWatcherData* pData, struct ev_io* watcher);
    bool IoError(tagManagerIoWatcherData* pData, struct ev_io* watcher);
    bool IoTimeout(tagManagerIoWatcherData* pData, struct ev_timer* watcher);
    bool ClientConnFrequencyTimeout(tagClientConnWatcherData* pData, struct ev_timer* watcher);

    void Run();

public:     // Manager相关设置（由专用Cmd类调用这些方法完成Manager自身的初始化和更新）
    bool InitLogger(const thunder::CJsonObject& oJsonConf);
    virtual bool SetProcessName(const thunder::CJsonObject& oJsonConf);
    /** @brief 加载配置，刷新Server */
    virtual void ResetLogLevel(log4cplus::LogLevel iLogLevel);
    virtual bool SendTo(const MsgShell& stMsgShell);
    virtual bool SendTo(const MsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual bool SendToParent(const MsgHead& oMsgHead,const MsgBody& oMsgBody){return false;}
    virtual bool SetConnectIdentify(const MsgShell& stMsgShell, const std::string& strIdentify);
    virtual bool AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual bool AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep){return(true);};
    virtual void SetNodeId(uint32 uiNodeId) {m_uiNodeId = uiNodeId;}
    virtual void AddInnerFd(const MsgShell& stMsgShell){};

    void SetWorkerLoad(int iPid, thunder::CJsonObject& oJsonLoad);
    void AddWorkerLoad(int iPid, int iLoad = 1);
    const std::map<int, tagWorkerAttr>& GetWorkerAttr() const;

protected:
    bool GetConf();
    bool Init();
    void Destroy();
    void CreateWorker();
    bool CreateEvents();
    bool RegisterToCenter();
    bool RestartWorker(int iDeathPid);
    bool AddPeriodicTaskEvent();
    bool AddWaitToExitTaskEvent(const MsgShell& stMsgShell,uint32 cmd,uint32 seq);
    bool AddIoReadEvent(tagConnectionAttr* pTagConnectionAttr,int iFd);
    bool AddIoWriteEvent(tagConnectionAttr* pTagConnectionAttr,int iFd);
    bool RemoveIoWriteEvent(tagConnectionAttr* pTagConnectionAttr);
    bool DelEvents(ev_io** io_watcher_addr);
    bool AddIoTimeout(int iFd, uint32 ulSeq, ev_tstamp dTimeout = 1.0);
    bool AddClientConnFrequencyTimeout(in_addr_t iAddr, ev_tstamp dTimeout = 60.0);
    tagConnectionAttr* CreateFdAttr(int iFd, uint32 ulSeq);
    bool DestroyConnect(std::map<int, tagConnectionAttr*>::iterator iter);
    std::pair<int, int> GetMinLoadWorkerDataFd();
    bool CheckWorker();
    bool RestartWorkers();
    bool CheckWorkerLoadNullExit(tagManagerWaitExitWatcherData* pData,struct ev_timer* watcher);
    bool CheckWorkerLoadNullRestartWorkers(tagManagerWaitExitWatcherData* pData,struct ev_timer* watcher);
    void RefreshServer();
    bool ReportToCenter();  // 向管理中心上报负载信息
    bool SendToWorker(const MsgHead& oMsgHead, const MsgBody& oMsgBody);    // 向Worker发送数据
    bool SendToWorkerWithMod(unsigned int uiModFactor,const MsgHead& oMsgHead, const MsgBody& oMsgBody);    // 向Worker发送数据
    bool DisposeDataFromWorker(const MsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, thunder::CBuffer* pSendBuff);
    bool DisposeDataAndTransferFd(const MsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, thunder::CBuffer* pSendBuff);
    bool DisposeDataFromCenter(const MsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, tagConnectionAttr* pTagConnectionAttr);

    uint32 GetSequence()
    {
        return(++m_ulSequence);
    }

    log4cplus::Logger GetLogger()
    {
        return(m_oLogger);
    }

    virtual uint32 GetNodeId() const
    {
        return(m_uiNodeId);
    }

private:
    thunder::CJsonObject m_oLastConf;          ///< 上次加载的配置
    thunder::CJsonObject m_oCurrentConf;       ///< 当前加载的配置

    uint32 m_ulSequence;
    char* m_pErrBuff;
    log4cplus::Logger m_oLogger;
    bool m_bInitLogger;
    ev_tstamp m_dIoTimeout;             ///< IO超时配置

    std::string m_strConfFile;              ///< 配置文件
    std::string m_strWorkPath;              ///< 工作路径
    std::string m_strNodeType;              ///< 节点类型
    uint32 m_uiNodeId;                      ///< 节点ID（由center分配）
    std::string m_strHostForServer;         ///< 对其他Server服务的IP地址，对应 m_iS2SListenFd
    std::string m_strHostForClient;         ///< 对Client服务的IP地址，对应 m_iC2SListenFd
    std::string m_strGateway;               ///< 对Client服务的真实IP地址（此ip转发给m_strHostForClient）
    int32 m_iPortForServer;                 ///< Server间通信监听端口，对应 m_iS2SListenFd
    int32 m_iPortForClient;                 ///< 对Client通信监听端口，对应 m_iC2SListenFd
    int32 m_iGatewayIp;                     ///< 对Client服务的真实端口
    uint32 m_uiWorkerNum;                   ///< Worker子进程数量
    thunder::E_CODEC_TYPE m_eCodec;            ///< 接入端编解码器
    ev_tstamp m_dAddrStatInterval;          ///< IP地址数据统计时间间隔
    int32  m_iAddrPermitNum;                ///< IP地址统计时间内允许连接次数
    int m_iLogLevel;
    int m_iWorkerBeat;                      ///< worker进程心跳，若大于此心跳未收到worker进程上报，则重启worker进程
    int m_iRefreshInterval;                 ///< 刷新Server的间隔周期
    int m_iLastRefreshCalc;                 ///< 上次刷新Server后的运行周期数

    int m_iS2SListenFd;                     ///< Server to Server监听文件描述符（Server与Server之间的连接较少，但每个Server的每个Worker均与其他Server的每个Worker相连）
    int m_iC2SListenFd;                     ///< Client to Server监听文件描述符（Client与Server之间的连接较多，但每个Client只需连接某个Server的某个Worker）
    struct ev_loop* m_loop;
    ev_timer* m_pPeriodicTaskWatcher;              ///< 周期任务定时器
    int m_iWaitToExitCounter;                   ///< 优雅等待关闭任务定时器次数
//    CmdConnectWorker* m_pCmdConnect;

    std::map<int, tagWorkerAttr> m_mapWorker;       ///< 业务逻辑工作进程及进程属性，key为pid
    std::map<int, int> m_mapWorkerRestartNum;       ///< 进程被重启次数，key为WorkerIdx
    std::map<int, int> m_mapWorkerFdPid;            ///< 工作进程通信FD对应的进程号
    std::map<std::string, MsgShell> m_mapCenterMsgShell; ///< 到center服务器的连接

    std::map<int, tagConnectionAttr*> m_mapFdAttr;  ///< 连接的文件描述符属性
    std::map<uint32, int> m_mapSeq2WorkerIndex;      ///< 序列号对应的Worker进程编号（用于connect成功后，向对端Manager发送希望连接的Worker进程编号）
    std::map<in_addr_t, uint32> m_mapClientConnFrequency; ///< 客户端连接频率
    std::map<int32, Cmd*> m_mapCmd;

    std::vector<int> m_vecFreeWorkerIdx;            ///< 空闲进程编号
};

} /* namespace thunder */

#endif /* NodeManager_HPP_ */
