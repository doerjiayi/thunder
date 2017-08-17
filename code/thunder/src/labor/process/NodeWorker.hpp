/*******************************************************************************
 * Project:  AsyncServer
 * @file     NodeWorker.hpp
 * @brief    Oss工作者
 * @author   cjy
 * @date:    2017年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef ThunderWorker_HPP_
#define ThunderWorker_HPP_

#include <map>
#include <list>
#include <dlfcn.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "hiredis/hiredis.h"
#include "libev/ev.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/logger.h"
#include "log4cplus/loggingmacros.h"
#include "log4cplus/socketappender.h"

#include "../../step/StepLog.hpp"
#include "../../step/StepNodeAccess.hpp"
#include "../process/Attribution.hpp"

#include "protocol/msg.pb.h"
#include "protocol/oss_sys.pb.h"
#include "labor/NodeLabor.hpp"
#include "codec/ThunderCodec.hpp"

namespace thunder
{

struct tagRedisAttr;
class CmdConnectWorker;

typedef Cmd* CreateCmd();

class NodeWorker;

struct tagSo
{
    void* pSoHandle;
    Cmd* pCmd;
    int iVersion;
    std::string strSoPath;
    std::string strSymbol;
    tagSo();
    ~tagSo();
};

struct tagModule
{
    void* pSoHandle;
    Module* pModule;
    int iVersion;
    std::string strSoPath;
    std::string strSymbol;
    tagModule();
    ~tagModule();
};

struct tagIoWatcherData
{
    int iFd;
    uint32 ulSeq;
    NodeWorker* pWorker;     // 不在结构体析构时回收

    tagIoWatcherData() : iFd(0), ulSeq(0), pWorker(NULL)
    {
    }
};

struct tagSessionWatcherData
{
    char szSessionName[32];
    char szSessionId[64];
    NodeWorker* pWorker;     // 不在结构体析构时回收
    tagSessionWatcherData() : pWorker(NULL)
    {
        memset(szSessionName, 0, sizeof(szSessionName));
        memset(szSessionId, 0, sizeof(szSessionId));
    }
};

class NodeWorker : public NodeLabor
{
public:
    typedef std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > > T_MAP_NODE_TYPE_IDENTIFY;
public:
    NodeWorker(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, llib::CJsonObject& oJsonConf);
    virtual ~NodeWorker();

    void Run();

    static void TerminatedCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents);
    static void IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents);
    static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
    static void IoTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void PeriodicTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);  // 周期任务回调，用于替换IdleCallback
    static void StepTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void SessionTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void RedisConnectCallback(const redisAsyncContext *c, int status);
    static void RedisDisconnectCallback(const redisAsyncContext *c, int status);
    static void RedisCmdCallback(redisAsyncContext *c, void *reply, void *privdata);
    void Terminated(struct ev_signal* watcher);
    bool CheckParent();
    bool SendToParent(const MsgHead& oMsgHead,const MsgBody& oMsgBody);
    bool IoRead(tagIoWatcherData* pData, struct ev_io* watcher);
    bool RecvDataAndDispose(tagIoWatcherData* pData, struct ev_io* watcher);
    bool FdTransfer();
    bool IoWrite(tagIoWatcherData* pData, struct ev_io* watcher);
    bool IoError(tagIoWatcherData* pData, struct ev_io* watcher);
    bool IoTimeout(struct ev_timer* watcher, bool bCheckBeat = true);
    bool StepTimeout(Step* pStep, struct ev_timer* watcher);
    bool SessionTimeout(Session* pSession, struct ev_timer* watcher);
    bool RedisConnect(const redisAsyncContext *c, int status);
    bool RedisDisconnect(const redisAsyncContext *c, int status);
    bool RedisCmdResult(redisAsyncContext *c, void *reply, void *privdata);

public:     // Cmd类和Step类只需关注这些方法
    virtual uint32 GetSequence()
    {
        ++m_ulSequence;
        if (m_ulSequence == 0)      // Server长期运行，sequence达到最大正整数又回到0
        {
            ++m_ulSequence;
        }
        return(m_ulSequence);
    }

    virtual const std::string& GetWorkPath() const
    {
        return(m_strWorkPath);
    }

    virtual log4cplus::Logger GetLogger()
    {
        return(m_oLogger);
    }

    virtual const std::string& GetNodeType() const
    {
        return(m_strNodeType);
    }

    virtual const llib::CJsonObject& GetCustomConf() const
    {
        return(m_oCustomConf);
    }

    virtual uint32 GetNodeId() const
    {
        return(m_uiNodeId);
    }

    virtual const std::string& GetHostForServer() const
    {
        return(m_strHostForServer);
    }

    virtual int GetPortForServer() const
    {
        return(m_iPortForServer);
    }

    virtual int GetWorkerIndex() const
    {
        return(m_iWorkerIndex);
    }

    virtual int GetClientBeatTime() const
    {
        return((int)m_dIoTimeout);
    }

    virtual const std::string& GetWorkerIdentify()
    {
        if (m_strWorkerIdentify.size() == 0)
        {
            char szWorkerIdentify[64] = {0};
            snprintf(szWorkerIdentify, 64, "%s:%d.%d", GetHostForServer().c_str(),
                            GetPortForServer(), GetWorkerIndex());
            m_strWorkerIdentify = szWorkerIdentify;
        }
        return(m_strWorkerIdentify);
    }

    virtual time_t GetNowTime() const;

    virtual bool Pretreat(Cmd* pCmd);
    virtual bool Pretreat(Step* pStep);
    virtual bool Pretreat(Session* pSession);

    int CoroutineNew(llib::coroutine_func callback,void *ud);
    bool CoroutineResume();//自定义调用策略,轮流执行规则
    void CoroutineResume(int co1,int index = -1);
    void CoroutineYield();
    int CoroutineStatus(int coid = -1);
    int CoroutineRunning();
    uint32 CoroutineTaskSize()const{return m_CoroutineIdList.size();}

    virtual bool RegisterCallback(Step* pStep, ev_tstamp dTimeout = 0.0);
    virtual bool RegisterCallback(uint32 uiSelfStepSeq, Step* pStep, ev_tstamp dTimeout = 0.0);
    virtual void DeleteCallback(Step* pStep);
    virtual void DeleteCallback(uint32 uiSelfStepSeq, Step* pStep);
    //virtual bool UnRegisterCallback(Step* pStep);
    virtual bool RegisterCallback(Session* pSession);
    virtual void DeleteCallback(Session* pSession);
    virtual bool RegisterCallback(const redisAsyncContext* pRedisContext, RedisStep* pRedisStep);
    virtual bool ResetTimeout(Step* pStep, struct ev_timer* watcher);
    virtual Session* GetSession(uint64 uiSessionId, const std::string& strSessionClass = "thunder::Session");
    virtual Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass = "thunder::Session");
    virtual bool Disconnect(const MsgShell& stMsgShell, bool bMsgShellNotice = true);
    virtual bool Disconnect(const std::string& strIdentify, bool bMsgShellNotice = true);

public:     // Worker相关设置（由专用Cmd类调用这些方法完成Worker自身的初始化和更新）
    virtual bool SetProcessName(const llib::CJsonObject& oJsonConf);
    /** @brief 加载配置，刷新Server */
    virtual void ResetLogLevel(log4cplus::LogLevel iLogLevel);
    virtual bool AddMsgShell(const std::string& strIdentify, const MsgShell& stMsgShell);
    virtual void DelMsgShell(const std::string& strIdentify, const MsgShell& stMsgShell);
    virtual void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    virtual void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    virtual void GetNodeIdentifys(const std::string& strNodeType, std::vector<std::string>& strIdentifys);
    virtual bool RegisterCallback(const std::string& strIdentify, RedisStep* pRedisStep);
    virtual bool RegisterCallback(const std::string& strHost, int iPort, RedisStep* pRedisStep);
    /*  redis 节点管理相关操作从框架中移除，交由DataProxy的SessionRedisNode来管理，框架只做到redis的连接管理
    virtual bool RegisterCallback(const std::string& strRedisNodeType, RedisStep* pRedisStep);
    virtual void AddRedisNodeConf(const std::string& strNodeType, const std::string strHost, int iPort);
    virtual void DelRedisNodeConf(const std::string& strNodeType, const std::string strHost, int iPort);
    */
    virtual bool AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx);
    virtual void DelRedisContextAddr(const redisAsyncContext* ctx);

public:     // 发送数据或从Worker获取数据
    /** @brief 自动连接成功后，把待发送数据发送出去 */
    virtual bool SendTo(const MsgShell& stMsgShell);
    virtual bool SendTo(const MsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual bool SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual bool SendToNext(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual bool SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual bool SendToNodeType(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual bool SendTo(const MsgShell& stMsgShell, const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL);
    virtual bool SentTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL);
    virtual bool SetConnectIdentify(const MsgShell& stMsgShell, const std::string& strIdentify);
    virtual bool AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual bool AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL);
    virtual bool AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep);
    virtual bool AutoConnect(const std::string& strIdentify);
    virtual void SetNodeId(uint32 uiNodeId) {m_uiNodeId = uiNodeId;}
    virtual void AddInnerFd(const MsgShell& stMsgShell);
    virtual bool GetMsgShell(const std::string& strIdentify, MsgShell& stMsgShell);
    virtual bool SetClientData(const MsgShell& stMsgShell, llib::CBuffer* pBuff);
    virtual bool HadClientData(const MsgShell& stMsgShell);
	virtual bool GetClientData(const MsgShell& stMsgShell, llib::CBuffer* pBuff);
    virtual std::string GetClientAddr(const MsgShell& stMsgShell);
    virtual std::string GetConnectIdentify(const MsgShell& stMsgShell);
    virtual bool AbandonConnect(const std::string& strIdentify);
    virtual void ExecStep(uint32 uiCallerStepSeq, uint32 uiCalledStepSeq,
                    int iErrno, const std::string& strErrMsg, const std::string& strErrShow);

    //接入服务器使用的对外接口
    virtual bool SendToClient(const MsgShell& stMsgShell,MsgHead& oMsgHead,const google::protobuf::Message &message,
                    const std::string& additional = "",uint64 sessionid = 0,const std::string& stressionid = "");
    virtual bool SendToClient(const std::string& strIdentify,MsgHead& oMsgHead,const google::protobuf::Message &message,
                    const std::string& additional = "",uint64 sessionid = 0,const std::string& stressionid = "");
    virtual bool BuildClientMsg(MsgHead& oMsgHead,MsgBody &oMsgBody,const google::protobuf::Message &message,
                            const std::string& additional = "",uint64 sessionid = 0,const std::string& strSession = "");
    virtual bool ParseFromMsg(const MsgBody& oInMsgBody,google::protobuf::Message &message);
    bool EmitStorageAccess(thunder::Session* pSession,const std::string &strMsgSerial,
    		StorageCallbackSession callback,bool boPermanentSession,
			const std::string &nodeType="PROXY",uint32 uiCmd = thunder::CMD_REQ_STORATE);
    bool EmitStorageAccess(thunder::Step* pUpperStep,const std::string &strMsgSerial,
    		StorageCallbackStep callback,const std::string &nodeType="PROXY",uint32 uiCmd = thunder::CMD_REQ_STORATE);

    bool EmitStandardAccess(thunder::Session* pSession,const std::string &strMsgSerial,
    		StandardCallbackSession callback,bool boPermanentSession,
			const std::string &nodeType,uint32 uiCmd);
	bool EmitStandardAccess(thunder::Step* pUpperStep,const std::string &strMsgSerial,
			StandardCallbackStep callback,const std::string &nodeType,uint32 uiCmd);
protected:
    bool Init(llib::CJsonObject& oJsonConf);
    bool InitLogger(const llib::CJsonObject& oJsonConf);
    bool CreateEvents();
    void PreloadCmd();
    void Destroy();
    bool AddPeriodicTaskEvent();

    bool AddIoReadEvent(tagConnectionAttr* pTagConnectionAttr,int iFd);
    bool AddIoWriteEvent(tagConnectionAttr* pTagConnectionAttr,int iFd);
    bool RemoveIoWriteEvent(tagConnectionAttr* pTagConnectionAttr);

    bool AddIoReadEvent(std::map<int, tagConnectionAttr*>::iterator iter);
    bool AddIoWriteEvent(std::map<int, tagConnectionAttr*>::iterator iter);
    bool RemoveIoWriteEvent(std::map<int, tagConnectionAttr*>::iterator iter);

    bool DelEvents(ev_io** io_watcher_attr);
    bool AddIoTimeout(int iFd, uint32 ulSeq, tagConnectionAttr* pConnAttr, ev_tstamp dTimeout = 1.0);
    tagConnectionAttr* CreateFdAttr(int iFd, uint32 ulSeq, llib::E_CODEC_TYPE eCodecType = llib::CODEC_PROTOBUF);
    bool DestroyConnect(std::map<int, tagConnectionAttr*>::iterator iter, bool bMsgShellNotice = true);
    void MsgShellNotice(const MsgShell& stMsgShell, const std::string& strIdentify, llib::CBuffer* pClientData);

    /**
     * @brief 收到完整数据包后处理
     * @note
     * <pre>
     * 框架层接收并解析到完整的数据包后调用此函数做数据处理。数据处理可能有多重不同情况出现：
     * 1. 处理成功，仍需继续解析数据包；
     * 2. 处理成功，但无需继续解析数据包；
     * 3. 处理失败，仍需继续解析数据包；
     * 4. 处理失败，但无需继续解析数据包。
     * 是否需要退出解析数据包循环体取决于Dispose()的返回值。返回true就应继续解析数据包，返回
     * false则应退出解析数据包循环体。处理过程或处理完成后，如需回复对端，则直接使用pSendBuff
     * 回复数据即可。
     * </pre>
     * @param[in] stMsgShell 数据包来源消息外壳
     * @param[in] oInMsgHead 接收的数据包头
     * @param[in] oInMsgBody 接收的数据包体
     * @param[out] oOutMsgHead 待发送的数据包头
     * @param[out] oOutMsgBody 待发送的数据包体
     * @return 是否继续解析数据包（注意不是处理结果）
     */
    bool Dispose(const MsgShell& stMsgShell,
                    const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,
                    MsgHead& oOutMsgHead, MsgBody& oOutMsgBody);

    /**
     * @brief 收到完整的hhtp包后处理
     * @param stMsgShell 数据包来源消息外壳
     * @param oInHttpMsg 接收的HTTP包
     * @param oOutHttpMsg 待发送的HTTP包
     * @return 是否继续解析数据包（注意不是处理结果）
     */
    bool Dispose(const MsgShell& stMsgShell,
                    const HttpMsg& oInHttpMsg, HttpMsg& oOutHttpMsg);

    void LoadSo(llib::CJsonObject& oSoConf);
    void ReloadSo(llib::CJsonObject& oCmds);
    tagSo* LoadSoAndGetCmd(int iCmd, const std::string& strSoPath, const std::string& strSymbol, int iVersion);
    void UnloadSoAndDeleteCmd(int iCmd);
    void LoadModule(llib::CJsonObject& oModuleConf);
    void ReloadModule(llib::CJsonObject& oUrlPaths);
    tagModule* LoadSoAndGetModule(const std::string& strModulePath, const std::string& strSoPath, const std::string& strSymbol, int iVersion);
    void UnloadSoAndDeleteModule(const std::string& strModulePath);
private:
    struct llib::schedule * m_pCoroutineSchedule;
    std::vector<int> m_CoroutineIdList;
	uint32 m_uiCoroutineRunIndex;
    char* m_pErrBuff;
    uint32 m_ulSequence;
    log4cplus::Logger m_oLogger;
    bool m_bInitLogger;
    ev_tstamp m_dIoTimeout;             ///< IO（连接）超时配置
    ev_tstamp m_dStepTimeout;           ///< 步骤超时

    std::string m_strNodeType;          ///< 节点类型
    std::string m_strHostForServer;     ///< 对其他Server服务的IP地址（用于生成当前Worker标识）
    std::string m_strWorkerIdentify;    ///< 进程标识
    int m_iPortForServer;               ///< Server间通信监听端口（用于生成当前Worker标识）
    std::string m_strWorkPath;          ///< 工作路径
    llib::CJsonObject m_oCustomConf;    ///< 自定义配置
    uint32 m_uiNodeId;                  ///< 节点ID
    int m_iManagerControlFd;            ///< 与Manager父进程通信fd（控制流）
    int m_iManagerDataFd;               ///< 与Manager父进程通信fd（数据流）
    int m_iWorkerIndex;
    int m_iWorkerPid;
    ev_tstamp m_dMsgStatInterval;       ///< 客户端连接发送数据包统计时间间隔
    int32  m_iMsgPermitNum;             ///< 客户端统计时间内允许发送消息数量

    int m_iRecvNum;                     ///< 接收数据包（head+body）数量
    int m_iRecvByte;                    ///< 接收字节数（已到达应用层缓冲区）
    int m_iSendNum;                     ///< 发送数据包（head+body）数量（只到达应用层缓冲区，不一定都已发送出去）
    int m_iSendByte;                    ///< 发送字节数（已到达系统发送缓冲区，可认为已发送出去）

    struct ev_loop* m_loop;
    CmdConnectWorker* m_pCmdConnect;
    std::map<llib::E_CODEC_TYPE, ThunderCodec*> m_mapCodec;   ///< 编解码器
    std::map<int, tagConnectionAttr*> m_mapFdAttr;   ///< 连接的文件描述符属性
    std::map<int, uint32> m_mapInnerFd;              ///< 服务端之间连接的文件描述符（用于区分连接是服务内部还是外部客户端接入）
    std::map<uint32, int> m_mapSeq2WorkerIndex;      ///< 序列号对应的Worker进程编号（用于connect成功后，向对端Manager发送希望连接的Worker进程编号）

    std::map<int32, Cmd*> m_mapCmd;                  ///< 预加载逻辑处理命令（一般为系统级命令）
    std::map<int, tagSo*> m_mapSo;                   ///< 动态加载业务逻辑处理命令
    std::map<std::string, tagModule*> m_mapModule;   ///< 动态加载的http逻辑处理模块

    std::map<uint32, Step*> m_mapCallbackStep;
    std::map<int32, std::list<uint32> > m_mapHttpAttr;       ///< TODO 以类似处理redis回调的方式来处理http回调
    std::map<redisAsyncContext*, tagRedisAttr*> m_mapRedisAttr;    ///< Redis连接属性
    std::map<std::string, std::map<std::string, Session*> > m_mapCallbackSession;

    /* 节点连接相关信息 */
    std::map<std::string, MsgShell> m_mapMsgShell;            // key为Identify
    std::map<std::string, std::string> m_mapIdentifyNodeType;    // key为Identify，value为node_type
    T_MAP_NODE_TYPE_IDENTIFY m_mapNodeIdentify;

    /* redis节点信息 */
    // std::map<std::string, std::set<std::string> > m_mapRedisNodeConf;        ///< redis节点配置，key为node_type，value为192.168.16.22:9988形式的IP+端口
    std::map<std::string, const redisAsyncContext*> m_mapRedisContext;       ///< redis连接，key为identify(192.168.16.22:9988形式的IP+端口)
    std::map<const redisAsyncContext*, std::string> m_mapContextIdentify;    ///< redis标识，与m_mapRedisContext的key和value刚好对调
};

} /* namespace thunder */

#endif /* ThunderWorker_HPP_ */
