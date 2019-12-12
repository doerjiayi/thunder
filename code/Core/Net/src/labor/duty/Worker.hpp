/*******************************************************************************
 * Project:  Net
 * @file     Worker.hpp
 * @brief    Oss工作者
 * @author   cjy
 * @date:    2017年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef Worker_HPP_
#define Worker_HPP_

#include <map>
#include <unordered_map>
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

#include "hiredis_vip/hiredis.h"
#include "hiredis_vip/hircluster.h"
#include "libev/ev.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/logger.h"
#include "log4cplus/loggingmacros.h"
#include "log4cplus/socketappender.h"

#include "dbi/MysqlAsyncConn.h"
#include "curl/CurlClient.hpp"

#include "../../step/StepNode.hpp"
#include "../../step/StepState.hpp"
#include "../../step/MysqlStep.hpp"
#include "Attribution.hpp"

#include "Hash/ConHash.hpp"
#include "protocol/msg.pb.h"
#include "protocol/oss_sys.pb.h"
#include "labor/Labor.hpp"
#include "codec/StarshipCodec.hpp"
#include "util/StreamCodec.hpp"

namespace net
{

struct tagRedisAttr;
class CmdConnectWorker;

typedef Cmd* CreateCmd();

class Worker;

struct tagSo
{
    void* pSoHandle;//不在本对象内管理
    Cmd* pCmd;
    int iVersion;
    std::string strSoPath;
    std::string strSymbol;
    std::string strLoadTime;
    tagSo();
    ~tagSo();
};

struct tagModule
{
    void* pSoHandle;//不在本对象内管理
    Module* pModule;
    int iVersion;
    std::string strSoPath;
    std::string strSymbol;
    std::string strLoadTime;
    tagModule();
    ~tagModule();
};

struct tagIoWatcherData
{
    int iFd;
    uint32 ulSeq;
    Worker* pWorker;     // 不在结构体析构时回收

    tagIoWatcherData() : iFd(0), ulSeq(0), pWorker(NULL)
    {
    }
};

struct tagSessionWatcherData
{
    char szSessionName[32];
    char szSessionId[64];
    Worker* pWorker;     // 不在结构体析构时回收
    tagSessionWatcherData() : pWorker(NULL)
    {
        memset(szSessionName, 0, sizeof(szSessionName));
        memset(szSessionId, 0, sizeof(szSessionId));
    }
};

class Worker : public Labor
{
public:
    typedef std::unordered_map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > > T_MAP_NODE_TYPE_IDENTIFY;
    Worker(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, util::CJsonObject& oJsonConf);
    ~Worker();
    //节点信息
    uint32 GetSequence(){return (++m_ulSequence > 0 ?m_ulSequence:++m_ulSequence);}// Server长期运行，sequence达到最大正整数又回到0
    const std::string& GetWorkPath() const {return(m_strWorkPath);}
    log4cplus::Logger GetLogger() {return(m_oLogger);}
    const std::string& GetNodeType() const {return(m_strNodeType);}
    const util::CJsonObject& GetCustomConf() const {return(m_oCustomConf);}
    uint32 GetNodeId() const {return(m_uiNodeId);}
    const std::string& GetHostForServer() const {return(m_strHostForServer);}
    int GetPortForServer() const{return(m_iPortForServer);}
    int GetWorkerIndex() const{return(m_iWorkerIndex);}
    int GetClientBeatTime() const{return((int)m_dIoTimeout);}
    const std::string& GetWorkerIdentify();
    time_t GetNowTime() const;
    void CloseSocket(int &s){if (s >= 0){close(s);s = -1;}}
    //步骤与会话
    bool RegisterCallback(Step* pStep, ev_tstamp dTimeout = 0.0);
    bool RegisterCallback(uint32 uiSelfStepSeq, Step* pStep, ev_tstamp dTimeout = 0.0);
    void DeleteCallback(Step* pStep);
    void DeleteCallback(uint32 uiSelfStepSeq, Step* pStep);
    bool RegisterCallback(Session* pSession);
    void DeleteCallback(Session* pSession);
    bool RegisterCallback(const redisAsyncContext* pRedisContext, RedisStep* pRedisStep);
    bool ResetTimeout(Step* pStep, struct ev_timer* watcher);
    Session* GetSession(uint64 uiSessionId, const std::string& strSessionClass = "net::Session");
    Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass = "net::Session");
    bool ExecStep(uint32 uiCallerStepSeq, uint32 uiCalledStepSeq,int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    bool ExecStep(uint32 uiCalledStepSeq,int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    bool ExecStep(Step* pStep,int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "",ev_tstamp dTimeout = 0.0);
    bool ExecStep(RedisStep* pStep);
    Step* GetStep(uint32 uiStepSeq);
    //发送数据  ,含自动连接成功后，发送待发送数据
    bool SendTo(const tagMsgShell& stMsgShell);
    bool SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    bool SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    bool SendToAuto(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    bool SendToNext(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    bool SendToWithMod(const std::string& strNodeType, uint32 uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    bool SendToConHash(const std::string& strNodeType, uint32 uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    bool SendToNodeType(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    bool SendTo(const tagMsgShell& stMsgShell, const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL);
    bool SentTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL);
    bool SetConnectIdentify(const tagMsgShell& stMsgShell, const std::string& strIdentify);
    bool AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    bool AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL);
    bool AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep);
    bool AutoRedisCluster(const std::string& sAddrList, RedisStep* pRedisStep);
    bool AutoMysqlCmd(MysqlStep* pMysqlStep);
    bool AutoConnect(const std::string& strIdentify);
    void SetNodeId(uint32 uiNodeId) {m_uiNodeId = uiNodeId;}
    bool HttpsGet(const std::string & strUrl, std::string & strResponse,const std::string& strUserpwd = "",
    		util::CurlClient::eContentType eType = util::CurlClient::eContentType_none,const std::string& strCaPath= "",int iPort = 0);
    bool HttpsPost(const std::string & strUrl, const std::string & strFields,std::string & strResponse,const std::string& strUserpwd = "",
    		util::CurlClient::eContentType eType = util::CurlClient::eContentType_none,const std::string& strCaPath= "",int iPort = 0);
	//返回消息
	bool BuildMsgBody(MsgHead& oMsgHead,MsgBody &oMsgBody,const google::protobuf::Message &message,const std::string& additional = "",uint64 sessionid = 0,const std::string& strSession = "",bool boJsonBody=false);
	bool ParseMsgBody(const MsgBody& oInMsgBody,google::protobuf::Message &message);
	bool SendToClient(const std::string& strIdentify,const MsgHead& oInMsgHead,const google::protobuf::Message &message,const std::string& additional = "",uint64 sessionid = 0,const std::string& stressionid = "",bool boJsonBody=false);
	bool SendToClient(const tagMsgShell& stInMsgShell,const MsgHead& oInMsgHead,const google::protobuf::Message &message,const std::string& additional = "",uint64 sessionid = 0,const std::string& stressionid = "",bool boJsonBody=false);
	bool SendToClient(const tagMsgShell& stInMsgShell,const MsgHead& oInMsgHead,const std::string &strBody);
	bool SendToClient(const tagMsgShell& stInMsgShell,const HttpMsg& oInHttpMsg,const std::string &strBody,int iCode=200,const std::unordered_map<std::string,std::string> &heads = std::unordered_map<std::string,std::string>());
	//发送消息(nodeType支持节点类型和节点标识符)
	bool SendToCallback(Session* pSession,const DataMem::MemOperate* pMemOper,SessionCallbackMem callback,const std::string &nodeType=PROXY_NODE,uint32 uiCmd = CMD_REQ_STORATE,int64 uiModFactor=-1);
	bool SendToCallback(Step* pUpperStep,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,const std::string &nodeType=PROXY_NODE,uint32 uiCmd = CMD_REQ_STORATE,int64 uiModFactor=-1);
	bool SendToCallback(Session* pSession,uint32 uiCmd,const std::string &strBody,SessionCallback callback,const std::string &nodeType,int64 uiModFactor=-1);
	bool SendToCallback(Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,const std::string &nodeType,int64 uiModFactor=-1);
	bool SendToCallback(Session* pSession,uint32 uiCmd,const std::string &strBody,SessionCallback callback,const tagMsgShell& stMsgShell,int64 uiModFactor=-1);
	bool SendToCallback(Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,const tagMsgShell& stMsgShell,int64 uiModFactor=-1);
public:
	void Run();
	//事件回调
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
	static void RedisClusterConnectCallback(const redisAsyncContext *c, int status);
	static void RedisClusterDisconnectCallback(const redisAsyncContext *c, int status);
	static void RedisClusterCmdCallback(redisClusterAsyncContext *acc, void *r, void *privdata);
	bool IoRead(tagIoWatcherData* pData, struct ev_io* watcher);
	bool IoWrite(tagIoWatcherData* pData, struct ev_io* watcher);
	bool IoError(tagIoWatcherData* pData, struct ev_io* watcher);
	bool IoTimeout(struct ev_timer* watcher, bool bCheckBeat = true);
	bool StepTimeout(Step* pStep, struct ev_timer* watcher);
	bool SessionTimeout(Session* pSession, struct ev_timer* watcher);
	bool FdTransfer();
	bool RecvDataAndDispose(tagIoWatcherData* pData, struct ev_io* watcher);
	bool OnRedisConnect(const redisAsyncContext *c, int status);
	bool OnRedisDisconnect(const redisAsyncContext *c, int status);
	bool OnRedisCmdResult(redisAsyncContext *c, void *reply, void *privdata);
	bool OnRedisClusterCmdResult(redisClusterAsyncContext *acc, void *r, void *privdata);
	//进程
	void OnTerminated(struct ev_signal* watcher);
	bool CheckParent();
	bool SendToParent(const MsgHead& oMsgHead,const MsgBody& oMsgBody);
    bool SetProcessName(const util::CJsonObject& oJsonConf);
    void ResetLogLevel(log4cplus::LogLevel iLogLevel);
    //连接
    bool Disconnect(const tagMsgShell& stMsgShell, bool bMsgShellNotice = true);
	bool Disconnect(const std::string& strIdentify, bool bMsgShellNotice = true);
    bool AddMsgShell(const std::string& strIdentify, const tagMsgShell& stMsgShell);
    void DelMsgShell(const std::string& strIdentify, const tagMsgShell& stMsgShell);
    void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    void GetNodeIdentifys(const std::string& strNodeType, std::vector<std::string>& strIdentifys);
    bool RegisterCallback(const std::string& strIdentify, RedisStep* pRedisStep);
    bool RegisterCallback(const std::string& strHost, int iPort, RedisStep* pRedisStep);
    bool AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx);
    void DelRedisContextAddr(const redisAsyncContext* ctx);
	bool RegisterCallback(MysqlStep* pMysqlStep);
	bool RegisterCallback(util::MysqlAsyncConn* pMysqlContext, MysqlStep* pMysqlStep);
	bool AddMysqlContextAddr(MysqlStep* pMysqlStep, util::MysqlAsyncConn* ctx);
	void DelMysqlContextAddr(util::MysqlAsyncConn* ctx);
    bool Host2Addr(const std::string & strHost,int iPort,struct sockaddr_in &stAddr,bool boRefresh=false);
    void AddInnerFd(const tagMsgShell& stMsgShell);
    bool GetMsgShell(const std::string& strIdentify, tagMsgShell& stMsgShell);
    bool SetClientData(const tagMsgShell& stMsgShell, util::CBuffer* pBuff);
    bool HadClientData(const tagMsgShell& stMsgShell);
	bool GetClientData(const tagMsgShell& stMsgShell, util::CBuffer* pBuff);
    std::string GetClientAddr(const tagMsgShell& stMsgShell);
    std::string GetConnectIdentify(const tagMsgShell& stMsgShell);
    bool AbandonConnect(const std::string& strIdentify);
	bool Dispose(util::MysqlAsyncConn *c, util::SqlTask *task, MYSQL_RES *pResultSet);
protected:
	//初始化
    bool Init(util::CJsonObject& oJsonConf);
    bool InitLogger(const util::CJsonObject& oJsonConf);
    bool CreateEvents();
    void PreloadCmd();
    void Destroy();
    bool AddPeriodicTaskEvent();
    //事件
    bool AddIoReadEvent(tagConnectionAttr* pConn);
    bool AddIoWriteEvent(tagConnectionAttr* pConn);
    bool RemoveIoWriteEvent(tagConnectionAttr* pConn);
    bool DelEvents(ev_io** io_watcher_attr);
    bool AddIoTimeout(int iFd, uint32 ulSeq, tagConnectionAttr* pConnAttr, ev_tstamp dTimeout = 1.0);
    //连接
    tagConnectionAttr* CreateFdAttr(int iFd, uint32 ulSeq, util::E_CODEC_TYPE eCodecType = util::CODEC_PROTOBUF);
    bool DestroyConnect(std::unordered_map<int, tagConnectionAttr*>::iterator iter, bool bMsgShellNotice = true);
    //消息
    void MsgShellNotice(const tagMsgShell& stMsgShell, const std::string& strIdentify, util::CBuffer* pClientData);
    bool Dispose(const tagMsgShell& stMsgShell,const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,MsgHead& oOutMsgHead, MsgBody& oOutMsgBody);
    bool Dispose(const tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg, HttpMsg& oOutHttpMsg);
    //动态库
    void LoadSo(util::CJsonObject& oSoConf,bool boForce=false);
    void ReloadSo(util::CJsonObject& oCmds);
    tagSo* LoadSoAndGetCmd(int iCmd, const std::string& strSoPath, const std::string& strSymbol, int iVersion);
    void UnloadSoAndDeleteCmd(int iCmd);
    void LoadModule(util::CJsonObject& oModuleConf,bool boForce=false);
    void ReloadModule(util::CJsonObject& oUrlPaths);
    tagModule* LoadSoAndGetModule(const std::string& strModulePath, const std::string& strSoPath, const std::string& strSymbol, int iVersion);
    void UnloadSoAndDeleteModule(const std::string& strModulePath);
private:
    char m_pErrBuff[gc_iErrBuffLen];
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
    util::CJsonObject m_oCustomConf;    ///< 自定义配置
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

    std::unordered_map<int, StarshipCodec*> m_mapCodec;   ///< 编解码器 util::E_CODEC_TYPE, StarshipCodec*
    std::unordered_map<int, tagConnectionAttr*> m_mapFdAttr;   ///< 连接的文件描述符属性
    std::unordered_map<int, uint32> m_mapInnerFd;              ///< 服务端之间连接的文件描述符（用于区分连接是服务内部还是外部客户端接入）
    std::unordered_map<uint32, int> m_mapSeq2WorkerIndex;      ///< 序列号对应的Worker进程编号（用于connect成功后，向对端Manager发送希望连接的Worker进程编号）

    std::unordered_map<int32, Cmd*> m_mapCmd;                  ///< 预加载逻辑处理命令（一般为系统级命令）
    std::unordered_map<int, tagSo*> m_mapSo;                   ///< 动态加载业务逻辑处理命令
    std::unordered_map<std::string, tagModule*> m_mapModule;   ///< 动态加载的http逻辑处理模块

    std::unordered_map<uint32, Step*> m_mapCallbackStep;
    std::unordered_map<int32, std::list<uint32> > m_mapHttpAttr;       ///< TODO 以类似处理redis回调的方式来处理http回调
    std::unordered_map<redisAsyncContext*, tagRedisAttr*> m_mapRedisAttr;    ///< Redis连接属性
    std::unordered_map<std::string, std::unordered_map<std::string, Session*> > m_mapCallbackSession;

    //节点连接
    std::unordered_map<std::string, tagMsgShell> m_mapMsgShell;            // key为Identify
    std::unordered_map<std::string, std::string> m_mapIdentifyNodeType;    // key为Identify，value为node_type
    T_MAP_NODE_TYPE_IDENTIFY m_mapNodeIdentify;

    std::unordered_map<std::string,ChannelConHash> m_mapChannelConHash;


    //redis节点连接
    // std::unordered_map<std::string, std::set<std::string> > m_mapRedisNodeConf;        ///< redis节点配置，key为node_type，value为192.168.16.22:9988形式的IP+端口
    std::unordered_map<std::string, const redisAsyncContext*> m_mapRedisContext;       ///< redis连接，key为identify(192.168.16.22:9988形式的IP+端口)
    std::unordered_map<const redisAsyncContext*, std::string> m_mapContextIdentify;    ///< redis标识，与m_mapRedisContext的key和value刚好对调
    //redis cluster连接
    std::unordered_map<std::string,redisClusterAsyncContext*> m_mapRedisClusterContext;
    std::unordered_map<redisClusterAsyncContext*,std::string> m_mapRedisClusterContextIdentify;
    std::unordered_map<redisClusterAsyncContext*, tagRedisAttr*> m_mapRedisClusterAttr;    ///< Redis连接属性
    //mysql节点连接
    typedef std::unordered_map<std::string, std::pair<std::set<util::MysqlAsyncConn*>::iterator,std::set<util::MysqlAsyncConn*> > > MysqlContextMap;
    MysqlContextMap m_mapMysqlContext;       ///< mysql连接，key为identify(192.168.16.22:9988形式的IP+端口)
	std::unordered_map<util::MysqlAsyncConn*, std::string> m_mapMysqlContextIdentify;    ///< mysql标识，与m_mapMysqlContext的key和value刚好对调

	struct tagSockaddr
    {
        unsigned long sockaddr;
        uint32 uiLastTime;
    };
	std::unordered_map<std::string,tagSockaddr> m_mapHosts;//dns host cache
};

} /* namespace net */

#endif /* Worker_HPP_ */
