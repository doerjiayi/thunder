/*******************************************************************************
 * Project:  Net
 * @file     Manager.hpp
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

#include "../../NetDefine.hpp"
#include "../../NetError.hpp"
#include "Attribution.hpp"
#include "Worker.hpp"
#include "libev/ev.h"
#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/socketappender.h"
#include "log4cplus/loggingmacros.h"

#include "util/json/CJsonObject.hpp"
#include "util/CBuffer.hpp"
#include "protocol/msg.pb.h"
#include "protocol/oss_sys.pb.h"
#include "labor/Labor.hpp"
#include "cmd/Cmd.hpp"

namespace net
{

class CmdConnectWorker;
class Manager;

struct tagClientConnWatcherData
{
    in_addr_t iAddr;
    Manager* pManager;     // 不在结构体析构时回收
    tagClientConnWatcherData() : iAddr(0), pManager(NULL)
    {
    }
};

struct tagManagerIoWatcherData
{
    int iFd;
    uint32 ulSeq;
    Manager* pManager;     // 不在结构体析构时回收
    tagManagerIoWatcherData() : iFd(0), ulSeq(0), pManager(NULL)
    {
    }
};

struct tagManagerWaitExitWatcherData
{
    tagMsgShell stMsgShell;
    uint32 cmd;
    uint32 seq;
    Manager* pManager;     // 不在结构体析构时回收
    tagManagerWaitExitWatcherData() : cmd(0),seq(0),pManager(NULL)
    {
    }
};

class Manager : public Labor
{
public:
    Manager(const std::string& strConfFile);
    virtual ~Manager();
    void Run();
    //libev回调函数
    static void SignalCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents);
    static void IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents);
    static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
    static void IoTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void PeriodicTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);  // 周期任务回调，用于替换IdleCallback
    static void WaitToExitTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents); //优雅等待关闭进程
    static void ClientConnFrequencyTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    //信号处理函数
    bool OnManagerTerminated(struct ev_signal* watcher);
    bool OnChildTerminated(struct ev_signal* watcher);
    //io处理函数
    bool IoRead(tagManagerIoWatcherData* pData, struct ev_io* watcher);
    bool IoWrite(tagManagerIoWatcherData* pData, struct ev_io* watcher);
    bool IoError(tagManagerIoWatcherData* pData, struct ev_io* watcher);
    bool IoTimeout(tagManagerIoWatcherData* pData, struct ev_timer* watcher);
    bool FdTransfer(int iFd);
	bool AcceptServerConn(int iFd);
	bool RecvDataAndDispose(tagManagerIoWatcherData* pData, struct ev_io* watcher);
    bool ClientConnFrequencyTimeout(tagClientConnWatcherData* pData, struct ev_timer* watcher);
    //配置
    bool InitLogger(const util::CJsonObject& oJsonConf);
    virtual bool SetProcessName(const util::CJsonObject& oJsonConf){ngx_setproctitle(oJsonConf("server_name").c_str());return true;}
    virtual void ResetLogLevel(log4cplus::LogLevel iLogLevel){m_oLogger.setLogLevel(iLogLevel);}
    //发送
    virtual bool SendTo(const tagMsgShell& stMsgShell);
    virtual bool SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual bool SendToParent(const MsgHead& oMsgHead,const MsgBody& oMsgBody){return false;}
    virtual bool SetConnectIdentify(const tagMsgShell& stMsgShell, const std::string& strIdentify);
    virtual bool AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual bool AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep){return(true);}
    //属性\负载
    virtual void SetNodeId(uint32 uiNodeId) {m_uiNodeId = uiNodeId;}
    virtual void AddInnerFd(const tagMsgShell& stMsgShell){};
    void SetWorkerLoad(int iPid, util::CJsonObject& oJsonLoad);
    void AddWorkerLoad(int iPid, int iLoad = 1);
    const std::unordered_map<int, tagWorkerAttr>& GetWorkerAttr() const;
protected:
    //初始化
    bool LoadConf();
    bool Init();
    void Destroy();
    void CreateWorker();
    bool CreateEvents();
    bool RegisterToCenter();
    bool RestartWorker(int iDeathPid);
    //事件
    bool AddPeriodicTaskEvent();
    bool AddWaitToExitTaskEvent(const tagMsgShell& stMsgShell,uint32 cmd,uint32 seq);
    bool AddIoReadEvent(tagConnectionAttr* pConn);
    bool AddIoWriteEvent(tagConnectionAttr* pConn);
    bool AddIoTimeout(int iFd, uint32 ulSeq, ev_tstamp dTimeout = 1.0);
    bool AddClientConnFrequencyTimeout(in_addr_t iAddr, ev_tstamp dTimeout = 60.0);
    bool RemoveIoWriteEvent(tagConnectionAttr* pConn);
	bool DelEvents(ev_io** io_watcher_addr);
    //连接
    tagConnectionAttr* CreateFdAttr(int iFd, uint32 ulSeq);
    bool DestroyConnect(std::unordered_map<int, tagConnectionAttr*>::iterator iter);
    //子进程管理
    std::pair<int, int> GetMinLoadWorkerDataFd();
    bool CheckWorker();
    bool RestartWorkers();
    bool CheckWorkersLoadEmptyExit(tagManagerWaitExitWatcherData* pData,struct ev_timer* watcher);
    bool CheckWorkersLoadEmptyRestart(tagManagerWaitExitWatcherData* pData,struct ev_timer* watcher);
    //消息
    void RefreshServer(bool boForce=false);
    bool ReportToCenter();  // 向管理中心上报负载信息
    bool SendToWorker(const MsgHead& oMsgHead, const MsgBody& oMsgBody);    // 向Worker发送数据
    bool SendToWorkerWithMod(uint32 uiModFactor,const MsgHead& oMsgHead, const MsgBody& oMsgBody);    // 向Worker发送数据
    bool DisposeDataFromWorker(const tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, util::CBuffer* pSendBuff);
    bool DisposeDataAndTransferFd(const tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, util::CBuffer* pSendBuff);
    bool DisposeDataFromCenter(const tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, tagConnectionAttr* pConn);
    //属性
    uint32 GetSequence(){return (++m_ulSequence > 0 ?m_ulSequence:++m_ulSequence);}// Server长期运行，sequence达到最大正整数又回到0
    log4cplus::Logger GetLogger(){return(m_oLogger);}
    virtual uint32 GetNodeId() const{return(m_uiNodeId);}
    void CloseSocket(int &s){if (s >= 0){close(s);s = -1;}}
private:
    util::CJsonObject m_oLastConf;          ///< 上次加载的配置
    util::CJsonObject m_oCurrentConf;       ///< 当前加载的配置

    uint32 m_ulSequence;
    char m_pErrBuff[gc_iErrBuffLen];
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
    int32 m_iGatewayPort;                     ///< 对Client服务的真实端口
    uint32 m_uiWorkerNum;                   ///< Worker子进程数量
    util::E_CODEC_TYPE m_eCodec;            ///< 接入端编解码器
    ev_tstamp m_dAddrStatInterval;          ///< IP地址数据统计时间间隔
    int32  m_iAddrPermitNum;                ///< IP地址统计时间内允许连接次数
    int m_iLogLevel;
    int m_iWorkerBeat;                      ///< worker进程心跳超时时间，若大于此心跳未收到worker进程上报，则重启worker进程
    int m_iRefreshInterval;                 ///< 刷新Server的间隔周期
    int m_iLastRefreshCalc;                 ///< 上次刷新Server后的运行周期数

    int m_iS2SListenFd;                     ///< Server to Server监听文件描述符（Server与Server之间的连接较少，但每个Server的每个Worker均与其他Server的每个Worker相连）
    int m_iC2SListenFd;                     ///< Client to Server监听文件描述符（Client与Server之间的连接较多，但每个Client只需连接某个Server的某个Worker）
    struct ev_loop* m_loop;
    ev_timer* m_pPeriodicTaskWatcher;              ///< 周期任务定时器
    int m_iWaitToExitCounter;                   ///< 优雅等待关闭任务定时器次数

    std::unordered_map<int, tagWorkerAttr> m_mapWorker;       ///< 业务逻辑工作进程及进程属性，key为pid
    std::unordered_map<int, int> m_mapWorkerRestartNum;       ///< 进程被重启次数，key为WorkerIdx
    std::unordered_map<int, int> m_mapWorkerFdPid;            ///< 工作进程通信FD对应的进程号
    std::unordered_map<std::string, tagMsgShell> m_mapCenterMsgShell; ///< 到center服务器的连接

    std::unordered_map<int, tagConnectionAttr*> m_mapFdAttr;  ///< 连接的文件描述符属性
    std::unordered_map<uint32, int> m_mapSeq2WorkerIndex;      ///< 序列号对应的Worker进程编号（用于connect成功后，向对端Manager发送希望连接的Worker进程编号）
    std::unordered_map<in_addr_t, uint32> m_mapClientConnFrequency; ///< 客户端连接频率 (unsigned long,uint32)
    std::unordered_map<int32, Cmd*> m_mapCmd;

    int nPid;
};

} /* namespace net */

#endif /* NodeManager_HPP_ */
