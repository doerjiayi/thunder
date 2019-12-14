/*******************************************************************************
 * Project:  Net
 * @file     Interface.hpp
 * @brief    Node工作成员
 * @author   cjy
 * @date:    2017年9月6日
 * Modify history:
 ******************************************************************************/
#ifndef SRC_Labor_Interface_HPP_
#define SRC_Labor_Interface_HPP_
#include <time.h>
#include <string>
#include <map>
#include <set>
#include <netdb.h>
#include <arpa/inet.h>
//#include <ares.h>
#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"
#include "hiredis_vip/hiredis.h"
#include "hiredis_vip/async.h"
#include "libev/ev.h"         // need ev_tstamp
#include "curl/CurlClient.hpp"
#include "util/json/CJsonObject.hpp"
#include "util/CBuffer.hpp"
#include "protocol/msg.pb.h"
#include "protocol/http.pb.h"
#include "storage/DbOperator.hpp"
#include "storage/MemOperator.hpp"
#include "../NetDefine.hpp"
#include "cmd/CW.hpp"
#include "Coroutine.h"

namespace net
{

#define USE_CONHASH

class Cmd;
class Module;
class Step;
class RedisStep;
class MysqlStep;
class HttpStep;
class StepState;
class Session;
class Labor;
class StepParam;

//访问存储节点回调（Proxy\PgAgent等）
typedef void (*SessionCallbackMem)(const DataMem::MemRsp &oRsp,net::Session*pSession);
typedef void (*StepCallbackMem)(const DataMem::MemRsp &oRsp,net::Step*pStep);
//访问一般节点回调
typedef void (*SessionCallback)(const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data,net::Session*pSession);
typedef void (*StepCallback)(const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data,net::Step*pStep);

/**
 * @brief 获取节点ID
 * @return 节点ID
 */
uint32 GetNodeId();
/**
 * @brief 获取Worker进程编号
 * @return Worker进程编号
 */
uint32 GetWorkerIndex();
/**
* @brief 获取当前节点类型
* @return 当前节点类型
*/
const std::string& GetNodeType() ;
/**
 * @brief 获取Server自定义配置
 * @return Server自定义配置
 */
const util::CJsonObject& GetCustomConf() ;
/**
 * @brief 获取当前Worker进程标识符
 * @note 当前Worker进程标识符由 IP:port:worker_index组成，例如： 192.168.18.22:30001.2
 * @return 当前Worker进程标识符
 */
const std::string& GetWorkerIdentify() ;
/**
 * @brief 获取工作目录
 * @return 工作目录
 */
const std::string& GetWorkPath();
/**
 * @brief 获取配置目录
 * @return 配置目录
 */
std::string GetConfigPath();
/**
 * @brief 获取当前时间
 * @note 获取当前时间，比time(NULL)速度快消耗小，不过没有time(NULL)精准，如果对时间精度
 * 要求不是特别高，建议调用GetNowTime()替代time(NULL)
 * @return 当前时间
 */
time_t GetNowTime();
/**
 * @brief 获取标识的节点类型属性列表
 * @param strNodeType 节点类型
 * @param strIdentifys 连接标识符列表
 */
void GetNodeIdentifys(const std::string& strNodeType, std::vector<std::string>& strIdentifys);

//返回消息到客户端
bool SendToClient(const net::tagMsgShell& oInMsgShell,const MsgHead &oInMsgHead,const std::string &strBody);
bool SendToClient(const net::tagMsgShell& oInMsgShell,const HttpMsg& oInHttpMsg,const std::string &strBody,int iCode=200,const std::unordered_map<std::string,std::string> &heads = std::unordered_map<std::string,std::string>());
bool SendToClient(const tagMsgShell& oInMsgShell,const MsgHead& oInMsgHead,const google::protobuf::Message &message,const std::string& additional = "",uint64 sessionid = 0,const std::string& stressionid = "",bool boJsonBody=false);
bool SendToClient(const std::string& strIdentify,const MsgHead& oInMsgHead,const google::protobuf::Message &message,const std::string& additional = "",uint64 sessionid = 0,const std::string& stressionid = "",bool boJsonBody=false);

//发送远程过程回调
bool SendToCallback(Session* pSession,const DataMem::MemOperate* pMemOper,SessionCallbackMem callback,const std::string &nodeType=PROXY_NODE,uint32 uiCmd = net::CMD_REQ_STORATE);
bool SendToModCallback(Session* pSession,const DataMem::MemOperate* pMemOper,SessionCallbackMem callback,int64 uiModFactor,const std::string &nodeType=PROXY_NODE,uint32 uiCmd = net::CMD_REQ_STORATE);
bool SendToCallback(Session* pSession,uint32 uiCmd,const std::string &strBody,SessionCallback callback,const std::string &nodeType);
bool SendToModCallback(Session* pSession,uint32 uiCmd,const std::string &strBody,SessionCallback callback,int64 uiModFactor,const std::string &nodeType);
bool SendToCallback(Session* pSession,uint32 uiCmd,const std::string &strBody,net::SessionCallback callback,const net::tagMsgShell& stMsgShell);
bool SendToModCallback(Session* pSession,uint32 uiCmd,const std::string &strBody,net::SessionCallback callback,int64 uiModFactor,const net::tagMsgShell& stMsgShell);

bool SendToCallback(net::Step* pUpperStep,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,const std::string &nodeType=PROXY_NODE,uint32 uiCmd = net::CMD_REQ_STORATE);
bool SendToCallback(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,StepParam *data,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,const std::string &nodeType=PROXY_NODE,uint32 uiCmd = net::CMD_REQ_STORATE);
bool SendToCallback(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,const std::string &nodeType=PROXY_NODE,uint32 uiCmd = net::CMD_REQ_STORATE);
bool SendToModCallback(net::Step* pUpperStep,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,int64 uiModFactor,const std::string &nodeType=PROXY_NODE,uint32 uiCmd = net::CMD_REQ_STORATE);
bool SendToCallback(net::Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,const std::string &nodeType);
bool SendToModCallback(net::Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,int64 uiModFactor,const std::string &nodeType);
bool SendToCallback(net::Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,const tagMsgShell& stMsgShell);
bool SendToModCallback(net::Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,int64 uiModFactor,const tagMsgShell& stMsgShell);
/*
 *  @brief 添加指定标识的消息外壳
 * @note 添加指定标识的消息外壳由Cmd类实例和Step类实例调用，该调用会在Step类中添加一个标识
 * 和消息外壳的对应关系，同时Worker中的连接属性也会添加一个标识。
 * @param strIdentify 连接标识符(IP:port.worker_index, e.g 192.168.11.12:3001.1)
 * @param stMsgShell  消息外壳
 * @return 是否添加成功
 */
bool AddMsgShell(const std::string& strIdentify, const net::tagMsgShell& stMsgShell);
/**
 * @brief 删除指定标识的消息外壳
 * @note 删除指定标识的消息外壳由Worker类实例调用，在IoError或IoTimeout时调用。
 */
void DelMsgShell(const std::string& strIdentify, const net::tagMsgShell& stMsgShell);

std::string GetClientAddr(const tagMsgShell& stMsgShell);
std::string GetConnectIdentify(const tagMsgShell& stMsgShell);
/**
 * @brief 解析消息到message
 */
bool ParseMsgBody(const MsgBody& oInMsgBody,google::protobuf::Message &message);
/*
 * 注册步骤
 * */
bool RegisterCallback(net::Step* pStep);
void DeleteCallback(net::Step* pStep);
bool RegisterCallback(net::MysqlStep* pMysqlStep);//注册MysqlStep
/**
 * @brief 登记会话
 * @param pSession 会话实例
 * @return 是否登记成功
 */
bool RegisterCallback(net::Session* pSession);
/**
 * @brief 删除回调步骤
 * @note 在RegisterCallback()成功，但执行pStep->Start()失败时被调用
 * @param pSession 会话实例
 */
void DeleteCallback(net::Session* pSession);
/**
 * @brief 获取会话实例
 * @param uiSessionId 会话ID
 * @return 会话实例（返回NULL表示不存在uiSessionId对应的会话实例）
 */
Session* GetSession(net::uint64 uiSessionId, const std::string& strSessionClass = "net::Session");
Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass = "net::Session");
/**
 * @brief 连接成功后发送
 * @note 当前Server往另一个Server发送数据而两Server之间没有可用连接时，框架层向对端发起连接（发起连接
 * 的过程是异步非阻塞的，connect()函数返回的时候并不知道连接是否成功），并将待发送数据存放于应用层待发
 * 送缓冲区。当连接成功时将待发送数据从应用层待发送缓冲区拷贝到应用层发送缓冲区并发送。此函数由框架层自
 * 动调用，业务逻辑层无须关注。
 * @param stMsgShell 消息外壳
 * @return 是否发送成功
 */
bool SendTo(const net::tagMsgShell& stMsgShell);
/**
 * @brief 发送数据
 * @note 使用指定连接将数据发送。如果能直接得知stMsgShell（比如刚从该连接接收到数据，欲回确认包），就
 * 应调用此函数发送。此函数是SendTo()函数中最高效的一个。
 * @param stMsgShell 消息外壳
 * @param oMsgHead 数据包头
 * @param oMsgBody 数据包体
 * @return 是否发送成功
 */
bool SendTo(const net::tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
bool SendTo(const net::tagMsgShell& stMsgShell,uint32 cmd,uint32 seq,const std::string &strBody);
bool SendTo(const net::tagMsgShell& stMsgShell, const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL);
/**
 * @brief 发送数据
 * @note 指定连接标识符将数据发送。此函数先查找与strIdentify匹配的stMsgShell，如果找到就调用
 * SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
 * 发送，如果未找到则调用SendToWithAutoConnect(const std::string& strIdentify,
 * const MsgHead& oMsgHead, const MsgBody& oMsgBody)连接后再发送。
 * @param strIdentify 连接标识符(IP:port.worker_index, e.g 192.168.11.12:3001.1)
 * @param oMsgHead 数据包头
 * @param oMsgBody 数据包体
 * @return 是否发送成功
 */
bool SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
bool SendTo(const std::string& strIdentify,uint32 cmd,uint32 seq,const std::string &strBody);
bool SendTo(const std::string& strIdentify,uint32 cmd,uint32 seq,const MsgBody& oMsgBody);
/**
 * @brief 根据路由id自动发送到指定的节点
 * @note 根据路由id自动发送到指定的节点
 * @param oMsgHead 数据包头
 * @param oMsgBody 数据包体
 * @return 是否发送成功
 */
bool SendToSession(const MsgHead& oMsgHead, const MsgBody& oMsgBody);
/**
 * @brief 根据路由发送到同一类型的节点
 * @note 根据路由发送到同一类型的节点
 * @param strNodeType 节点类型
 * @param oMsgHead 数据包头
 * @param oMsgBody 数据包体
 * @return 是否发送成功
 */
bool SendToSession(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
/**
 * @brief 发送到下一个同一类型的节点
 * @note 发送到下一个同一类型的节点，适用于对同一类型节点做轮询方式发送以达到简单的负载均衡。
 * @param strNodeType 节点类型
 * @param oMsgHead 数据包头
 * @param oMsgBody 数据包体
 * @return 是否发送成功
 */
bool SendToNext(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
/**
 * @brief 以取模方式选择发送到同一类型节点
 * @note 以取模方式选择发送到同一类型节点，实现简单有要求的负载均衡。
 * @param strNodeType 节点类型
 * @param uiModFactor 取模因子
 * @param oMsgHead 数据包头
 * @param oMsgBody 数据包体
 * @return 是否发送成功
 */
bool SendToWithMod(const std::string& strNodeType, uint32 uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
/**
 * @brief 以一致性哈希方式选择发送到同一类型节点
 * @note 以取模方式选择发送到同一类型节点，实现简单有要求的负载均衡。
 * @param strNodeType 节点类型
 * @param uiModFactor 取模因子
 * @param oMsgHead 数据包头
 * @param oMsgBody 数据包体
 * @return 是否发送成功
 */
bool SendToConHash(const std::string& strNodeType, uint32 uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
/**
 * @brief 注册redis回调
 * @param strIdentify redis节点标识(192.168.16.22:9988形式的IP+端口)
 * @param pRedisStep redis步骤实例
 * @return 是否注册成功
 */
bool RegisterCallback(const std::string& strIdentify, net::RedisStep* pRedisStep);
/**
 * @brief 注册redis回调
 * @param strHost redis节点IP
 * @param iPort redis端口
 * @param pRedisStep redis步骤实例
 * @return 是否注册成功
 */
bool RegisterCallback(const std::string& strHost, int iPort, net::RedisStep* pRedisStep);
/**
 * @brief 添加标识的节点类型属性
 * @note 添加标识的节点类型属性，用于以轮询方式向同一节点类型的节点发送数据，以
 * 实现简单的负载均衡。只有Server间的各连接才具有NodeType属性，客户端到Access节
 * 点的连接不具有NodeType属性。
 * @param strNodeType 节点类型
 * @param strIdentify 连接标识符
 */
void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
/**
 * @brief 删除标识的节点类型属性
 * @note 删除标识的节点类型属性，当一个节点下线，框架层会自动调用此函数删除标识
 * 的节点类型属性。
 * @param strNodeType 节点类型
 * @param strIdentify 连接标识符
 */
void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
/**
 * @brief 添加指定标识的redis context地址
 * @note 添加指定标识的redis context由Worker调用，该调用会在Step类中添加一个标识
 * 和redis context的对应关系。
 */
bool AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx);
/**
 * @brief 删除指定标识的redis context地址
 * @note 删除指定标识的到redis地址的对应关系（此函数被调用时，redis context的资源已被释放或将被释放）
 * 用。
 */
void DelRedisContextAddr(const redisAsyncContext* ctx);
/*
 * @brief 状态步骤
 * 开始发送(执行)/注册(但未执行)状态步骤
 * @param uiTimeOutMax 超时次数
 * @param uiToRetry 是否超时重发 1：是 0 否
 * @param dTimeout 超时时间（单位秒，默认使用配置时间）
 * @return 是否成功
 * */
bool Launch(StepState *step,uint32 uiTimeOutMax=3,uint8 uiToRetry = 1,double dTimeout = 0.0);
bool Register(StepState *step,uint32 uiTimeOutMax=3,uint8 uiToRetry = 1,double dTimeout = 0.0);

void SkipNonsenseLetters(std::string& word);
void SkipFormatLetters(std::string& word);
bool GetConfig(util::CJsonObject& oConf,const std::string &strConfFile);

bool ExecStep(Step* pStep,int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "",ev_tstamp dTimeout = 0.0);
/*
 * @brief 唤醒协程
 * @param nMaxTimes 唤醒协程次数 (最大执行协程次数，0则执行所有的协程)
 * @return 返回true 还有需要执行的协程，返回false没有还需要执行的协程
 * */
bool CoroutineResumeWithTimes(uint32 nMaxTimes=0);
/*
 * @brief 创建一个协程
 * @param func 协程函数
 * @param arg 协程参数（在协程执行完毕后自动销毁）
 * @return 是否成功
 * */
bool CoroutineNewWithArg(util::coroutine_func func,tagCoroutineArg *arg);
/*
 * @brief 创建一个协程
 * @param func 协程函数
 * @return ud 协程参数（在协程执行完毕后不会自动销毁）
 * @return 是否成功
 * */
int CoroutineNew(util::coroutine_func func,void *ud);
//获取正在进行的协程的id
int CoroutineRunning();
//返回协程状态
int CoroutineStatus(int coid);
//唤醒指定协程
bool CoroutineResume(int coid);
//自定义调用策略,轮流执行规则
bool CoroutineResume();
//在协程函数中放弃协程的本次执行
bool CoroutineYield();

}

#endif
