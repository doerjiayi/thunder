
/*******************************************************************************
 * Project:  Net
 * @file     Worker.cpp
 * @brief 
 * @author   cjy
 * @date:    2017年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
#include "hiredis_vip/async.h"
#include "hiredis_vip/adapters/libev.h"
#include "hiredis_vip/hircluster.h"
#include "unix/process_helper.h"
#include "unix/proctitle_helper.h"
#ifdef __cplusplus
}
#endif
#include "util/UnixTime.hpp"

#include <sched.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include "codec/ProtoCodec.hpp"
#include "codec/ClientMsgCodec.hpp"
#include "codec/HttpCodec.hpp"
#include "codec/CodecWebSocketJson.hpp"
#include "codec/CodecWebSocketPb.hpp"
#include "step/Step.hpp"
#include "step/RedisStep.hpp"
#include "step/HttpStep.hpp"
#include "session/Session.hpp"
#include "cmd/Cmd.hpp"
#include "cmd/Module.hpp"
#include "cmd/sys_cmd/CmdConnectWorker.hpp"
#include "cmd/sys_cmd/CmdToldWorker.hpp"
#include "cmd/sys_cmd/CmdUpdateNodeId.hpp"
#include "cmd/sys_cmd/CmdNodeNotice.hpp"
#include "cmd/sys_cmd/CmdBeat.hpp"
#include "cmd/sys_cmd/CmdUpdateConfig.hpp"
#include "step/sys_step/StepIoTimeout.hpp"
#include "Worker.hpp"

namespace net
{

tagSo::tagSo() : pSoHandle(NULL), pCmd(NULL), iVersion(0)
{
    strLoadTime = util::GetCurrentTime(20);
}

tagSo::~tagSo()
{
	SAFE_DELETE(pCmd);
//    if (pSoHandle != NULL)
//    {
//        dlclose(pSoHandle);
//        pSoHandle = NULL;
//    }
}

tagModule::tagModule() : pSoHandle(NULL), pModule(NULL), iVersion(0)
{
    strLoadTime = util::GetCurrentTime(20);
}

tagModule::~tagModule()
{
	SAFE_DELETE(pModule);
//    if (pSoHandle != NULL)
//    {
//        dlclose(pSoHandle);
//        pSoHandle = NULL;
//    }
}


void Worker::TerminatedCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Worker* pWorker = (Worker*)watcher->data;
        pWorker->OnTerminated(watcher);  // timeout，worker进程无响应或与Manager通信通道异常，被manager进程终止时返回
    }
}

void Worker::IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Worker* pWorker = (Worker*)watcher->data;
        pWorker->CheckParent();
    }
}

void Worker::IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagIoWatcherData* pData = (tagIoWatcherData*)watcher->data;
        Worker* pWorker = (Worker*)pData->pWorker;
        if (revents & EV_READ)
        {
            pWorker->IoRead(pData, watcher);
        }
        if (revents & EV_WRITE)
        {
            pWorker->IoWrite(pData, watcher);
        }
        if (revents & EV_ERROR)
        {
            pWorker->IoError(pData, watcher);
        }
    }
}

void Worker::IoTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagIoWatcherData* pData = (tagIoWatcherData*)watcher->data;
        Worker* pWorker = pData->pWorker;
        pWorker->IoTimeout(watcher);
    }
}

void Worker::PeriodicTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Worker* pWorker = (Worker*)(watcher->data);
        pWorker->CheckParent();

    }
    ev_timer_stop (loop, watcher);
    ev_timer_set (watcher, NODE_BEAT + ev_time() - ev_now(loop), 0);
    ev_timer_start (loop, watcher);
}

void Worker::StepTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Step* pStep = (Step*)watcher->data;
        ((Worker*)(g_pLabor))->StepTimeout(pStep, watcher);
    }
}

void Worker::SessionTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Session* pSession = (Session*)watcher->data;
        ((Worker*)g_pLabor)->SessionTimeout(pSession, watcher);
    }
}

void Worker::RedisConnectCallback(const redisAsyncContext *c, int status)
{
    if (c->userData != NULL)
    {
        Worker* pWorker = (Worker*)c->userData;
        pWorker->OnRedisConnect(c, status);
    }
}

void Worker::RedisDisconnectCallback(const redisAsyncContext *c, int status)
{
    if (c->userData != NULL)
    {
        Worker* pWorker = (Worker*)c->userData;
        pWorker->OnRedisDisconnect(c, status);
    }
}

void Worker::RedisCmdCallback(redisAsyncContext *c, void *reply, void *privdata)
{
    if (c->userData != NULL)
    {
        Worker* pWorker = (Worker*)c->userData;
        pWorker->OnRedisCmdResult(c, reply, privdata);
    }
}

void Worker::RedisClusterConnectCallback(const redisAsyncContext *c, int status)
{
    if (c->userData != NULL)
    {
        Worker* pWorker = (Worker*)c->userData;
        LOG4_INFO("%s",__FUNCTION__);
    }
}

void Worker::RedisClusterDisconnectCallback(const redisAsyncContext *c, int status)
{
    if (c->userData != NULL)
    {
        Worker* pWorker = (Worker*)c->userData;//目前当做集群是高可用的
        LOG4_INFO("%s",__FUNCTION__);
    }
}

void Worker::RedisClusterCmdCallback(redisClusterAsyncContext *acc, void *reply, void *privdata)
{
    if (privdata != NULL)
    {
        Worker* pWorker = (Worker*)privdata;
        pWorker->OnRedisClusterCmdResult(acc, reply, privdata);
    }
}

Worker::Worker(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, util::CJsonObject& oJsonConf)
    : m_ulSequence(0), m_bInitLogger(false), m_dIoTimeout(480.0), m_strWorkPath(strWorkPath), m_uiNodeId(0),
      m_iManagerControlFd(iControlFd), m_iManagerDataFd(iDataFd), m_iWorkerIndex(iWorkerIndex), m_iWorkerPid(0),
      m_dMsgStatInterval(60.0), m_iMsgPermitNum(60),
      m_iRecvNum(0), m_iRecvByte(0), m_iSendNum(0), m_iSendByte(0),
      m_loop(NULL), m_pCmdConnect(NULL)
{
	m_iWorkerPid = getpid();
    if (!Init(oJsonConf))
    {
        exit(-1);
    }
    if (!CreateEvents())
    {
        exit(-2);
    }
    PreloadCmd();
    LoadSo(oJsonConf["so"]);
    LoadModule(oJsonConf["module"]);
}

Worker::~Worker()
{
    Destroy();
}

void Worker::Run()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_run (m_loop, 0);
}

void Worker::OnTerminated(struct ev_signal* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iSignum = watcher->signum;
    delete watcher;
    Destroy();
    LOG4_FATAL("terminated by signal %d!", iSignum);
    exit(iSignum);
}

bool Worker::CheckParent()
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    pid_t iParentPid = getppid();
    if (iParentPid == 1)    // manager进程已不存在
    {
        LOG4_INFO("no manager duty exist, worker %d exit.", m_iWorkerIndex);
        //Destroy();
        exit(0);
    }
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    util::CJsonObject oJsonLoad;
    oJsonLoad.Add("load", int32(m_mapFdAttr.size() + m_mapCallbackStep.size()));
    oJsonLoad.Add("connect", int32(m_mapFdAttr.size()));
    oJsonLoad.Add("recv_num", m_iRecvNum);
    oJsonLoad.Add("recv_byte", m_iRecvByte);
    oJsonLoad.Add("send_num", m_iSendNum);
    oJsonLoad.Add("send_byte", m_iSendByte);
    oJsonLoad.Add("client", int32(m_mapFdAttr.size() - m_mapInnerFd.size()));
    LOG4_TRACE("%s", oJsonLoad.ToString().c_str());
    oMsgBody.set_body(oJsonLoad.ToString());
    oMsgHead.set_cmd(CMD_REQ_UPDATE_WORKER_LOAD);
    oMsgHead.set_seq(GetSequence());
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(m_iManagerControlFd);
    if (iter != m_mapFdAttr.end())
    {
    	int iErrno = 0;
    	iter->second->pSendBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
    	iter->second->pSendBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
    	iter->second->pSendBuff->WriteFD(m_iManagerControlFd, iErrno);
    	iter->second->pSendBuff->Compact(8192);
    }
    m_iRecvNum = 0;
    m_iRecvByte = 0;
    m_iSendNum = 0;
    m_iSendByte = 0;
//    uint32 uiLoad = m_mapFdAttr.size() + m_mapCallbackStep.size();
//    write(m_iManagerControlFd, &uiLoad, sizeof(uiLoad));    // 向父进程上报当前进程负载
    return(true);
}

bool Worker::SendToParent(const MsgHead& oMsgHead,const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(m_iManagerControlFd);
    if (iter != m_mapFdAttr.end())
    {
        int iErrno = 0;
        iter->second->pSendBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
        iter->second->pSendBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
        iter->second->pSendBuff->WriteFD(m_iManagerControlFd, iErrno);
        iter->second->pSendBuff->Compact(8192);
    }
    m_iRecvNum = 0;
    m_iRecvByte = 0;
    m_iSendNum = 0;
    m_iSendByte = 0;
    return(true);
}

bool Worker::IoRead(tagIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (watcher->fd == m_iManagerDataFd)
    {
        return(FdTransfer());
    }
    else
    {
        return(RecvDataAndDispose(pData, watcher));
    }
}

bool Worker::RecvDataAndDispose(tagIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iErrno = 0;
    int iReadLen = 0;
    std::unordered_map<int, tagConnectionAttr*>::iterator conn_iter;
    conn_iter = m_mapFdAttr.find(pData->iFd);
    if (conn_iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd attr for %d!", pData->iFd);
    }
    else
    {
    	tagConnectionAttr* pConn = conn_iter->second;
    	pConn->dActiveTime = ev_now(m_loop);
        if (pData->ulSeq != pConn->ulSeq)
        {
            LOG4_DEBUG("callback seq %lu not match the conn attr seq %lu",pData->ulSeq, pConn->ulSeq);
            ev_io_stop(m_loop, watcher);
            pData->pWorker = NULL;
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return(false);
        }
        conn_iter->second->pRecvBuff->Compact(8192);
        read_again:
        iReadLen = pConn->pRecvBuff->ReadFD(pData->iFd, iErrno);
		LOG4_TRACE("recv from fd %d ip %s identify %s, data len %d codec %d",pData->iFd, pConn->pRemoteAddr,pConn->strIdentify.c_str(),iReadLen, pConn->eCodecType);
        if (iReadLen > 0)
        {
            m_iRecvByte += iReadLen;
            conn_iter->second->ulMsgNumUnitTime++;      // TODO 这里要做发送消息频率限制，有待补充
            conn_iter->second->ulMsgNum++;
            MsgHead oInMsgHead, oOutMsgHead;
            MsgBody oInMsgBody, oOutMsgBody;
            StarshipCodec* pCodec = NULL;
            auto codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", conn_iter->second->eCodecType);
                if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)
                {
                    LOG4_DEBUG("if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)");
                    DestroyConnect(conn_iter);
                }
                return(false);
            }
            while (conn_iter->second->pRecvBuff->ReadableBytes() >= gc_uiClientMsgHeadSize)
            {
                oInMsgHead.Clear();
                oInMsgBody.Clear();
                E_CODEC_STATUS eCodecStatus = codec_iter->second->Decode(conn_iter->second, oInMsgHead, oInMsgBody);
                //网关类型节点的连接初始化时支持协议编解码器的替换（支持的是websocket json 或websocket pb与http,private的替换）
                if (eConnectStatus_init == pConn->ucConnectStatus)
                {
                    //网关默认配置websocket json(可修改为websocket pb)
                    if (CODEC_STATUS_ERR == eCodecStatus &&
                        (util::CODEC_WEBSOCKET_EX_JS == pConn->eCodecType
                        		|| util::CODEC_WEBSOCKET_EX_PB == pConn->eCodecType))
                    {
                        //切换为http协议
                        LOG4_DEBUG("failed to decode for codec %d,switch to CODEC_HTTP",pConn->eCodecType);
                        conn_iter->second->eCodecType = util::CODEC_HTTP;
                        auto codec_iter = m_mapCodec.find(pConn->eCodecType);
                        if (codec_iter == m_mapCodec.end())
                        {
                            LOG4_ERROR("no codec found for %d!", pConn->eCodecType);
                            if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)
                            {
                                LOG4_DEBUG("if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)");
                                DestroyConnect(conn_iter);
                            }
                            return(false);
                        }
                        eCodecStatus = codec_iter->second->Decode(pConn, oInMsgHead, oInMsgBody);
                    }
                    if (CODEC_STATUS_ERR == eCodecStatus && util::CODEC_HTTP == pConn->eCodecType)
                    {
                        //切换为私有协议编解码（与客户端通信协议） private pb
                        LOG4_DEBUG("failed to decode for codec %d,switch to CODEC_PRIVATE",pConn->eCodecType);
                        conn_iter->second->eCodecType = util::CODEC_PRIVATE;
                        auto codec_iter = m_mapCodec.find(pConn->eCodecType);
                        if (codec_iter == m_mapCodec.end())
                        {
                            LOG4_ERROR("no codec found for %d!", pConn->eCodecType);
                            if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)
                            {
                                LOG4_DEBUG("if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)");
                                DestroyConnect(conn_iter);
                            }
                            return(false);
                        }
                        eCodecStatus = codec_iter->second->Decode(pConn, oInMsgHead, oInMsgBody);
                    }
                }
                if (CODEC_STATUS_OK == eCodecStatus)
                {
                    ++m_iRecvNum;
                    conn_iter->second->dActiveTime = ev_now(m_loop);
                    bool bDisposeResult = false;
                    tagMsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = conn_iter->second->ulSeq;
                    // 基于TCP的自定义协议请求或带cmd、seq自定义头域的请求.http短连接不需要带cmd
                    if (oInMsgHead.cmd() > 0)
                    {
                    	if (util::CODEC_PRIVATE == pConn->eCodecType)
                    	{
                    		auto inner_iter = m_mapInnerFd.find(stMsgShell.iFd);
							if (inner_iter == m_mapInnerFd.end() && pConn->ulMsgNum > 1)   // 未经账号验证的客户端连接发送数据过来，直接断开
							{
								LOG4_WARN("invalid request, please login first!");
								DestroyConnect(conn_iter);
								return(false);
							}
                    	}
                        bDisposeResult = Dispose(stMsgShell, oInMsgHead, oInMsgBody, oOutMsgHead, oOutMsgBody); // 处理过程有可能会断开连接，所以下面要做连接是否存在检查
                        auto dispose_conn_iter = m_mapFdAttr.find(pData->iFd);
                        if (dispose_conn_iter == m_mapFdAttr.end() || pData->ulSeq != dispose_conn_iter->second->ulSeq)     // 连接已断开，资源已回收
                        {
                            return(true);
                        }
                        if (oOutMsgHead.ByteSize() > 0)
                        {
                            eCodecStatus = codec_iter->second->Encode(oOutMsgHead, oOutMsgBody, pConn->pSendBuff);
                            if (CODEC_STATUS_OK == eCodecStatus)
                            {
                                conn_iter->second->pSendBuff->WriteFD(pData->iFd, iErrno);
                                conn_iter->second->pSendBuff->Compact(8192);
                                if (iErrno != 0)
                                {
                                    LOG4_ERROR("error %d %s!",iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                                }
                            }
                            else if (CODEC_STATUS_ERR == eCodecStatus)
                            {
                                LOG4_ERROR("faild to Encode");
                            }
                        }
                    }
                    else  // url方式的http请求或者websocket
                    {
                    	if (pConn->pClientData != NULL && pConn->pClientData->ReadableBytes() > 0)
						{//长连接并且验证过的，不需要再验证。只填充附带数据。
							oInMsgBody.set_additional(pConn->pClientData->GetRawReadBuffer(),pConn->pClientData->ReadableBytes());
							oInMsgHead.set_msgbody_len(oInMsgBody.ByteSize());
						}
						else
						{
							// 如果是websocket连接，则需要验证连接
							if (util::CODEC_WEBSOCKET_EX_PB == pConn->eCodecType || util::CODEC_WEBSOCKET_EX_JS == pConn->eCodecType)
							{
								auto http_iter = m_mapHttpAttr.find(stMsgShell.iFd);
								if (http_iter == m_mapHttpAttr.end())   // 未经握手的websocket客户端连接发送数据过来，直接断开
								{
									LOG4_WARN("invalid request, please handshake first!");
									DestroyConnect(conn_iter);
									return(false);
								}
							}
							else if (util::CODEC_HTTP == pConn->eCodecType)//其他类型长连接(目前对外开放的只有http和websocket)
							{
								auto inner_iter = m_mapHttpAttr.find(stMsgShell.iFd);
								if (inner_iter == m_mapHttpAttr.end() && pConn->ulMsgNum > 1)   // 未经账号验证的客户端连接发送数据过来，直接断开
								{
									LOG4_WARN("invalid request, please login first!");
									DestroyConnect(conn_iter);
									return(false);
								}
							}
						}

                        HttpMsg oInHttpMsg;
                        HttpMsg oOutHttpMsg;
                        if (oInHttpMsg.ParseFromString(oInMsgBody.body()))
                        {
                            if (util::CODEC_WEBSOCKET_EX_PB != conn_iter->second->eCodecType &&
                                    util::CODEC_WEBSOCKET_EX_JS != conn_iter->second->eCodecType)
                            {
                                conn_iter->second->dKeepAlive = 10;   // 未带KeepAlive参数的http协议，默认10秒钟关闭
                                LOG4_TRACE("set dKeepAlive(%lf)",pConn->dKeepAlive);
                            }
                            else
                            {
                                LOG4_TRACE("set dKeepAlive(%lf)",pConn->dKeepAlive);//websocket保持长连接,dKeepAlive为0
                            }
                            for (int i = 0; i < oInHttpMsg.headers_size(); ++i)
                            {
                                if (std::string("Keep-Alive") == oInHttpMsg.headers(i).header_name())
                                {
                                    conn_iter->second->dKeepAlive = strtoul(oInHttpMsg.headers(i).header_value().c_str(), NULL, 10);
                                    LOG4_TRACE("set dKeepAlive(%lf)",pConn->dKeepAlive);
                                    AddIoTimeout(conn_iter->first, pConn->ulSeq, conn_iter->second, conn_iter->second->dKeepAlive);
                                    break;
                                }
                                else if (std::string("Connection") == oInHttpMsg.headers(i).header_name())
                                {
                                    if (std::string("keep-alive") == oInHttpMsg.headers(i).header_value())
                                    {
                                        conn_iter->second->dKeepAlive = 65.0;
                                        LOG4_TRACE("set dKeepAlive(%lf)",pConn->dKeepAlive);
                                        AddIoTimeout(conn_iter->first, pConn->ulSeq, conn_iter->second, 65.0);
                                        break;
                                    }
                                    else if (std::string("close") == oInHttpMsg.headers(i).header_value()
                                                    && HTTP_RESPONSE == oInHttpMsg.type())
                                    {   // 作为客户端请求远端http服务器，对方response后要求客户端关闭连接
                                        conn_iter->second->dKeepAlive = -1;
                                        LOG4_DEBUG("std::string(\"close\") == oInHttpMsg.headers(i).header_value()");
                                        DestroyConnect(conn_iter);
                                        break;
                                    }
                                    else
                                    {
                                        AddIoTimeout(conn_iter->first, pConn->ulSeq, pConn, pConn->dKeepAlive);
                                    }
                                }
                            }
                            bDisposeResult = Dispose(stMsgShell, oInHttpMsg, oOutHttpMsg);
                            // 处理过程有可能会断开连接，所以下面要做连接是否存在检查
							auto dispose_conn_iter = m_mapFdAttr.find(pData->iFd);
							if (dispose_conn_iter == m_mapFdAttr.end() || pData->ulSeq != dispose_conn_iter->second->ulSeq)     // 连接已断开，资源已回收
							{
								return(true);
							}

                            if (pConn->dKeepAlive < 0)
                            {
                                if (HTTP_RESPONSE == oInHttpMsg.type())
                                {
                                    LOG4_DEBUG("if (HTTP_RESPONSE == oInHttpMsg.type())");
                                    DestroyConnect(conn_iter);
                                }
                                else
                                {
                                    ((HttpCodec*)codec_iter->second)->AddHttpHeader("Connection", "close");
                                }
                            }
                            if (oOutHttpMsg.ByteSize() > 0)
                            {
                                eCodecStatus = ((HttpCodec*)codec_iter->second)->Encode(oOutHttpMsg, conn_iter->second->pSendBuff);
                                if (CODEC_STATUS_OK == eCodecStatus)
                                {
                                    conn_iter->second->pSendBuff->WriteFD(pData->iFd, iErrno);
                                    conn_iter->second->pSendBuff->Compact(8192);
                                    if (iErrno != 0)
                                    {
                                        LOG4_ERROR("error %d %s!",
                                                        iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                                    }
                                }
                                else if (CODEC_STATUS_ERR == eCodecStatus)
                                {
                                    LOG4_ERROR("faild to Encode");
                                }
                            }
                        }
                        else
                        {
                            LOG4_ERROR("oInHttpMsg.ParseFromString() error!");
                        }
                    }
                    if (!bDisposeResult)
                    {
                        break;
                    }
                }
                else if (CODEC_STATUS_ERR == eCodecStatus)
                {
                    if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)
                    {
                        LOG4_DEBUG("if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)");
                        DestroyConnect(conn_iter);
                    }
                    return(false);
                }
                else
                {
                    break;  // 数据尚未接收完整
                }
            }
            return(true);
        }
        else if (iReadLen == 0)
        {
            LOG4_DEBUG("fd %d closed by peer, errno %d %s!",
                            pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
            if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)
            {
                LOG4_DEBUG("if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)");
                DestroyConnect(conn_iter);
            }
            else//收到父进程通信fd关闭
            {
            	LOG4_ERROR("iReadLen == 0 pData->iFd(%u) m_iManagerControlFd(%u) m_iManagerDataFd(%u)",
            			pData->iFd,m_iManagerControlFd,m_iManagerDataFd);
            }
        }
        else
        {
            LOG4_TRACE("recv from fd %d errno %d: %s",pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
            if (EAGAIN == iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                ;
            }
            else if (EINTR == iErrno)//中断继续读
            {
                goto read_again;
            }
            else
            {
                LOG4_ERROR("recv from fd %d errno %d: %s",pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)
                {
                    LOG4_DEBUG("if (pData->iFd != m_iManagerControlFd && pData->iFd != m_iManagerDataFd)");
                    DestroyConnect(conn_iter);
                }
            }
        }
    }
    return(false);
}

bool Worker::FdTransfer()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    char szIpAddr[16] = {0};
    int iCodec = 0;
    int iAcceptFd = recv_fd_with_attr(m_iManagerDataFd, szIpAddr, 16, &iCodec);
    if (iAcceptFd <= 0)
    {
        if (iAcceptFd == 0)//父进程关闭后，子进程收到的消息
        {
            LOG4_ERROR("recv_fd from m_iManagerDataFd %d len %d", m_iManagerDataFd, iAcceptFd);
            Destroy();
            exit(2); // manager与worker通信fd已关闭，worker进程退出
        }
        else //if (errno != EAGAIN)
        {
            LOG4_ERROR("recv_fd from m_iManagerDataFd %d error %d : %s", m_iManagerDataFd,errno,strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
            Destroy();
            exit(2); // manager与worker通信fd已关闭，worker进程退出
        }
    }
    else
    {
        uint32 ulSeq = GetSequence();
        tagConnectionAttr* pConnAttr = CreateFdAttr(iAcceptFd, ulSeq, util::E_CODEC_TYPE(iCodec));
        x_sock_set_block(iAcceptFd, 0);
        if (pConnAttr)
        {
            snprintf(pConnAttr->pRemoteAddr, 32, szIpAddr);
            LOG4_DEBUG("pConnAttr->pClientAddr = %s, iCodec = %d", pConnAttr->pRemoteAddr, iCodec);
            auto iter =  m_mapFdAttr.find(iAcceptFd);
            if (iter != m_mapFdAttr.end())
            {
            	if(AddIoTimeout(iAcceptFd, ulSeq, iter->second, 1.5))     // 为了防止大量连接攻击，初始化连接只有一秒即超时，在第一次超时检查（或正常发送第一个http数据包）之后才采用正常配置的网络IO超时检查
				{
					if (!AddIoReadEvent(iter->second))
					{
						LOG4_DEBUG("if (!AddIoReadEvent(iter))");
						DestroyConnect(iter);
						return(false);
					}
					return(true);
				}
				else
				{
					LOG4_DEBUG("if(AddIoTimeout(iAcceptFd, ulSeq, iter->second, 1.5)) else");
					DestroyConnect(iter);
					return(false);
				}
            }
        }
        else    // 没有足够资源分配给新连接，直接close掉
        {
        	close(iAcceptFd);
        }
    }
    return(false);
}

bool Worker::IoWrite(tagIoWatcherData* pData, struct ev_io* watcher)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    auto attr_iter =  m_mapFdAttr.find(pData->iFd);
    if (attr_iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
    	tagConnectionAttr* pConn = attr_iter->second;
        if (pData->ulSeq != pConn->ulSeq || pData->iFd != pConn->iFd)
        {
            LOG4_DEBUG("callback seq %lu iFd %d not match the conn attr seq %lu ifd %d",
                            pData->ulSeq, pData->iFd,attr_iter->second->ulSeq,pConn->iFd);
            ev_io_stop(m_loop, watcher);
            pData->pWorker = NULL;
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return(false);
        }
        int iErrno = 0;
        int iNeedWriteLen = (int)attr_iter->second->pSendBuff->ReadableBytes();
        int iWriteLen = 0;
        iWriteLen = pConn->pSendBuff->WriteFD(pData->iFd, iErrno);
        pConn->pSendBuff->Compact(8192);
        if (iWriteLen < 0)
        {
            if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                LOG4_TRACE("if (EAGAIN != iErrno && EINTR != iErrno)");
                LOG4_ERROR("send to fd %d error %d: %s",
                                pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                DestroyConnect(attr_iter);
            }
            else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
            {
                attr_iter->second->dActiveTime = ev_now(m_loop);
                AddIoWriteEvent(pConn);
            }
        }
        else if (iWriteLen > 0)
        {
            m_iSendByte += iWriteLen;
            attr_iter->second->dActiveTime = ev_now(m_loop);
            if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
            {
                RemoveIoWriteEvent(pConn);
            }
            else    // 内容未写完，添加或保持监听fd写事件
            {
                AddIoWriteEvent(pConn);
            }
        }
        else    // iWriteLen == 0 写缓冲区为空
        {
//            LOG4_TRACE("pData->iFd %d, watcher->fd %d, iter->second->pWaitForSendBuff->ReadableBytes()=%d",
//                            pData->iFd, watcher->fd, attr_iter->second->pWaitForSendBuff->ReadableBytes());
            if (attr_iter->second->pWaitForSendBuff->ReadableBytes() > 0)    // 存在等待发送的数据，说明本次写事件是connect之后的第一个写事件
            {
                auto index_iter = m_mapSeq2WorkerIndex.find(attr_iter->second->ulSeq);
                if (index_iter != m_mapSeq2WorkerIndex.end())   // 系统内部Server间通信需由Manager转发
                {
                    tagMsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = attr_iter->second->ulSeq;
                    AddInnerFd(stMsgShell);
                    if (util::CODEC_PROTOBUF == attr_iter->second->eCodecType)  // 系统内部Server间通信
                    {
                        m_pCmdConnect->Start(stMsgShell, index_iter->second);
                    }
                    else        // 与系统外部Server通信，连接成功后直接将数据发送
                    {
                        SendTo(stMsgShell);
                    }
                    m_mapSeq2WorkerIndex.erase(index_iter);
                    LOG4_TRACE("RemoveIoWriteEvent(%d)", pData->iFd);
                    RemoveIoWriteEvent(pConn);    // 在m_pCmdConnect的两个回调之后再把等待发送的数据发送出去
                }
                else // 与系统外部Server通信，连接成功后直接将数据发送
                {
                    tagMsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = attr_iter->second->ulSeq;
                    SendTo(stMsgShell);
                }
            }
        }
        return(true);
    }
}

bool Worker::IoError(tagIoWatcherData* pData, struct ev_io* watcher)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    auto iter =  m_mapFdAttr.find(pData->iFd);
    if (iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
        LOG4_TRACE("if (iter == m_mapFdAttr.end()) else");
        if (pData->ulSeq != iter->second->ulSeq)
        {
            LOG4_DEBUG("callback seq %lu not match the conn attr seq %lu",pData->ulSeq, iter->second->ulSeq);
            ev_io_stop(m_loop, watcher);
            pData->pWorker = NULL;
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return(false);
        }
        DestroyConnect(iter);
        return(true);
    }
}

bool Worker::IoTimeout(struct ev_timer* watcher, bool bCheckBeat)
{
    LOG4_TRACE("%s()",__FUNCTION__);
    bool bRes = false;
    tagIoWatcherData* pData = (tagIoWatcherData*)watcher->data;
    if (pData == NULL)
    {
        LOG4_ERROR("pData is null in %s()", __FUNCTION__);
        ev_timer_stop(m_loop, watcher);
        delete watcher;
        watcher = NULL;
        return(false);
    }
    auto iter =  m_mapFdAttr.find(pData->iFd);
    if (iter == m_mapFdAttr.end())
    {
        LOG4_TRACE("%s() iFd(%d) not found",__FUNCTION__,pData->iFd);
        bRes = false;
    }
    else
    {
        if (iter->second->ulSeq != pData->ulSeq)      // 文件描述符数值相等，但已不是原来的文件描述符
        {
            LOG4_TRACE("%s() ulSeq(%u) not found",__FUNCTION__,pData->ulSeq);
            bRes = false;
        }
        else
        {
            LOG4_TRACE("%s() time(%lf) IoTimeout(%lf) bCheckBeat(%d) dKeepAlive(%lf) RemoteAddr(%s)",
						__FUNCTION__,ev_now(m_loop),m_dIoTimeout,bCheckBeat ? 1 : 0,iter->second->dKeepAlive,iter->second->pRemoteAddr);
            if (bCheckBeat && iter->second->dKeepAlive == 0)     // 需要发送心跳检查 或 完成心跳检查并未超时
            {
                ev_tstamp after = iter->second->dActiveTime - ev_now(m_loop) + m_dIoTimeout;
                if (after > 0)    // IO在定时时间内被重新刷新过，重新设置定时器
                {
                    ev_timer_stop (m_loop, watcher);
                    ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
                    ev_timer_start (m_loop, watcher);
                    LOG4_TRACE("ev_timer_set:after(%lf) ev_time(%lf) ev_now(%lf) IoTimeout(%lf)",after,ev_time(),ev_now(m_loop),m_dIoTimeout);
                    return(true);
                }
                else    // IO已超时，发送心跳检查
                {
                    tagMsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = iter->second->ulSeq;
                    StepIoTimeout* pStepIoTimeout = new StepIoTimeout(stMsgShell, watcher);
                    if (pStepIoTimeout != NULL)
                    {
                        if (RegisterCallback(pStepIoTimeout,2.0))
                        {
                            LOG4_TRACE("RegisterCallback(pStepIoTimeout) time(%lf)",ev_now(m_loop));
                            net::E_CMD_STATUS eStatus = pStepIoTimeout->Emit(ERR_OK);
                            if (STATUS_CMD_RUNNING != eStatus)
                            {
                                // pStepIoTimeout->Start()会发送心跳包，若干返回非running状态，则表明发包时已出错，
                                // 销毁连接过程在SentTo里已经完成，这里不需要再销毁连接
                            	LOG4_WARN("%s()",__FUNCTION__);
                                DeleteCallback(pStepIoTimeout);
                            }
                            else
                            {
                                return(true);
                            }
                        }
                        else
                        {
                            LOG4_WARN("RegisterCallback(pStepIoTimeout,2.0) failed");
                            delete pStepIoTimeout;
                            pStepIoTimeout = NULL;
                            DestroyConnect(iter);
                        }
                    }
                }
            }
            else        // 心跳检查过后的超时已是实际超时，关闭文件描述符并清理相关资源
            {
                if (iter->second->dKeepAlive > 0)
                {
                    ev_tstamp after = iter->second->dActiveTime - ev_now(m_loop) + iter->second->dKeepAlive;
                    if (after > 0)    // IO在定时时间内被重新刷新过，重新设置定时器
                    {
                        ev_timer_stop (m_loop, watcher);
                        ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
                        ev_timer_start (m_loop, watcher);
                        LOG4_TRACE("ev_timer_set(watcher) time(%lf) dKeepAlive(%lf)",ev_now(m_loop),iter->second->dKeepAlive);
                        return(true);
                    }
                }
                tagMsgShell stMsgShell;
                stMsgShell.iFd = pData->iFd;
                stMsgShell.ulSeq = iter->second->ulSeq;
                LOG4_TRACE("%s() ClientAddr(%s)",__FUNCTION__,GetClientAddr(stMsgShell).c_str());
                auto inner_iter = m_mapInnerFd.find(pData->iFd);
                if (inner_iter == m_mapInnerFd.end())   // 非内部服务器间的连接才会在超时中关闭
                {
                    LOG4_WARN("io timeout and send beat, but there is no beat response received!");
                    DestroyConnect(iter);
                }
            }
            bRes = true;
        }
    }
    return(bRes);
}

bool Worker::StepTimeout(Step* pStep, struct ev_timer* watcher)
{
    ev_tstamp after = pStep->GetActiveTime() - ev_now(m_loop) + pStep->GetTimeout();
    if (after > 0)    // 在定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, watcher);
        LOG4_TRACE("%s(pStep %p seq %lu): reset watcher %lf",__FUNCTION__,pStep, pStep->GetSequence(), after + ev_time() - ev_now(m_loop));
        return(true);
    }
    else    // 会话已超时
    {
        LOG4_TRACE("%s(pStep %p seq %lu): active_time %lf, now_time %lf, lifetime %lf",
                        __FUNCTION__,pStep, pStep->GetSequence(), pStep->GetActiveTime(), ev_now(m_loop), pStep->GetTimeout());
        E_CMD_STATUS eResult = pStep->Timeout();
        if (STATUS_CMD_RUNNING == eResult)      // 超时Timeout()里重新执行Start()，重新设置定时器
        {
            ev_tstamp after = pStep->GetTimeout();
            ev_timer_stop (m_loop, watcher);
            ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
            ev_timer_start (m_loop, watcher);
            return(true);
        }
        else
        {
            DeleteCallback(pStep);
            return(true);
        }
    }
}

bool Worker::SessionTimeout(Session* pSession, struct ev_timer* watcher)
{
    ev_tstamp after = pSession->GetActiveTime() - ev_now(m_loop) + pSession->GetTimeout();
    if (after > 0)    // 定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, watcher);
        return(true);
    }
    else    // 会话已超时
    {
        //LOG4_TRACE("%s(session_name: %s,  session_id: %s)",__FUNCTION__, pSession->GetSessionClass().c_str(), pSession->GetSessionId().c_str());
        if (STATUS_CMD_RUNNING == pSession->Timeout())   // 定时器时间到，执行定时操作，session时间刷新
        {
            ev_timer_stop (m_loop, watcher);
            ev_timer_set (watcher, pSession->GetTimeout() + ev_time() - ev_now(m_loop), 0);
            ev_timer_start (m_loop, watcher);
            return(true);
        }
        else
        {
            DeleteCallback(pSession);
            return(true);
        }
    }
}

bool Worker::OnRedisConnect(const redisAsyncContext *c, int status)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<redisAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        if (status == REDIS_OK)
        {
            attr_iter->second->bIsReady = true;
            int iCmdStatus;
            for (std::list<RedisStep*>::iterator step_iter = attr_iter->second->listWaitData.begin();
                            step_iter != attr_iter->second->listWaitData.end(); )
            {
                RedisStep* pRedisStep = (RedisStep*)(*step_iter);
                size_t args_size = pRedisStep->GetRedisCmd()->GetCmdArguments().size() + 1;
                const char* argv[args_size];
                size_t arglen[args_size];
                argv[0] = pRedisStep->GetRedisCmd()->GetCmd().c_str();
                arglen[0] = pRedisStep->GetRedisCmd()->GetCmd().size();
                std::vector<std::pair<std::string, bool> >::const_iterator c_iter = pRedisStep->GetRedisCmd()->GetCmdArguments().begin();
                for (size_t i = 1; c_iter != pRedisStep->GetRedisCmd()->GetCmdArguments().end(); ++c_iter, ++i)
                {
                    argv[i] = c_iter->first.c_str();
                    arglen[i] = c_iter->first.size();
                }
                iCmdStatus = redisAsyncCommandArgv((redisAsyncContext*)c, RedisCmdCallback, NULL, args_size, argv, arglen);
                if (iCmdStatus == REDIS_OK)
                {
                    LOG4_DEBUG("succeed in sending redis cmd: %s", pRedisStep->GetRedisCmd()->ToString().c_str());
                    attr_iter->second->listData.push_back(pRedisStep);
                    attr_iter->second->listWaitData.erase(step_iter++);
                }
                else    // 命令执行失败，不再继续执行，等待下一次回调
                {
                    break;
                }
            }
        }
        else
        {
            for (std::list<RedisStep *>::iterator step_iter = attr_iter->second->listWaitData.begin();
                            step_iter != attr_iter->second->listWaitData.end(); ++step_iter)
            {
                RedisStep* pRedisStep = (RedisStep*)(*step_iter);
                pRedisStep->Callback(c, status, NULL);
                delete pRedisStep;
            }
            attr_iter->second->listWaitData.clear();
            delete attr_iter->second;
            attr_iter->second = NULL;
            DelRedisContextAddr(c);
            m_mapRedisAttr.erase(attr_iter);
        }
    }
    return(true);
}

bool Worker::OnRedisDisconnect(const redisAsyncContext *c, int status)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::unordered_map<redisAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        for (std::list<RedisStep *>::iterator step_iter = attr_iter->second->listData.begin();
                        step_iter != attr_iter->second->listData.end(); ++step_iter)
        {
            LOG4_ERROR("OnRedisDisconnect callback error %d of redis cmd: %s",
                            c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());
            (*step_iter)->Callback(c, c->err, NULL);
            delete (*step_iter);
            (*step_iter) = NULL;
        }
        attr_iter->second->listData.clear();

        for (std::list<RedisStep *>::iterator step_iter = attr_iter->second->listWaitData.begin();
                        step_iter != attr_iter->second->listWaitData.end(); ++step_iter)
        {
            LOG4_ERROR("OnRedisDisconnect callback error %d of redis cmd: %s",
                            c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());
            (*step_iter)->Callback(c, c->err, NULL);
            delete (*step_iter);
            (*step_iter) = NULL;
        }
        attr_iter->second->listWaitData.clear();

        delete attr_iter->second;
        attr_iter->second = NULL;
        DelRedisContextAddr(c);
        m_mapRedisAttr.erase(attr_iter);
    }
    return(true);
}

bool Worker::OnRedisCmdResult(redisAsyncContext *c, void *reply, void *privdata)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::unordered_map<redisAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        std::list<RedisStep*>::iterator step_iter = attr_iter->second->listData.begin();
        if (NULL == reply)
        {
            std::unordered_map<const redisAsyncContext*, std::string>::iterator identify_iter = m_mapContextIdentify.find(c);
            if (identify_iter != m_mapContextIdentify.end())
            {
                LOG4_ERROR("redis %s error %d: %s", identify_iter->second.c_str(), c->err, c->errstr);
            }
            for ( ; step_iter != attr_iter->second->listData.end(); ++step_iter)
            {
                LOG4_ERROR("callback error %d of redis cmd: %s", c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());
                (*step_iter)->Callback(c, c->err, (redisReply*)reply);
                delete (*step_iter);
                (*step_iter) = NULL;
            }
            attr_iter->second->listData.clear();

            delete attr_iter->second;
            attr_iter->second = NULL;
            DelRedisContextAddr(c);
            m_mapRedisAttr.erase(attr_iter);
        }
        else
        {
            if (step_iter != attr_iter->second->listData.end())
            {
                LOG4_TRACE("callback of redis cmd: %s", (*step_iter)->GetRedisCmd()->ToString().c_str());
                /** @note 注意，若Callback返回STATUS_CMD_RUNNING，框架不回收并且不再管理该RedisStep，该RedisStep后续必须重新RegisterCallback或由开发者自己回收 */
                if (STATUS_CMD_RUNNING != (*step_iter)->Callback(c, REDIS_OK, (redisReply*)reply))
                {
                    delete (*step_iter);
                    (*step_iter) = NULL;
                }
                attr_iter->second->listData.erase(step_iter);
                //freeReplyObject(reply);
            }
            else
            {
                LOG4_ERROR("no redis callback data found!");
            }
        }
    }
    return(true);
}

bool Worker::OnRedisClusterCmdResult(redisClusterAsyncContext *acc, void *reply, void *privdata)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::unordered_map<redisClusterAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisClusterAttr.find(acc);
    if (attr_iter != m_mapRedisClusterAttr.end())
    {
        LOG4_DEBUG("%s() redisClusterAsyncContext(%s,%d)", __FUNCTION__,acc->cc->ip,acc->cc->port);
        redisAsyncContext cobj;//对逻辑层只是抛出错误信息，不直接使用连接对象
        cobj.err = acc->err;
        cobj.errstr = acc->errstr;
        redisAsyncContext *c = &cobj;
        std::list<RedisStep*>::iterator step_iter = attr_iter->second->listData.begin();
        if (NULL == reply)
        {
            for ( ; step_iter != attr_iter->second->listData.end(); ++step_iter)
            {
                LOG4_ERROR("callback error %d of redis cmd: %s", c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());
                (*step_iter)->Callback(c, c->err, (redisReply*)reply);
                delete (*step_iter);
                (*step_iter) = NULL;
            }
            attr_iter->second->listData.clear();

            delete attr_iter->second;
            attr_iter->second = NULL;
            m_mapRedisClusterAttr.erase(attr_iter);
        }
        else
        {
            if (step_iter != attr_iter->second->listData.end())
            {
                LOG4_TRACE("callback of redis cmd: %s", (*step_iter)->GetRedisCmd()->ToString().c_str());
                /** @note 注意，若Callback返回STATUS_CMD_RUNNING，框架不回收并且不再管理该RedisStep，该RedisStep后续必须重新RegisterCallback或由开发者自己回收 */
                if (STATUS_CMD_RUNNING != (*step_iter)->Callback(c, REDIS_OK, (redisReply*)reply))
                {
                    delete (*step_iter);
                    (*step_iter) = NULL;
                }
                attr_iter->second->listData.erase(step_iter);
                //freeReplyObject(reply);
            }
            else
            {
                LOG4_ERROR("no redis callback data found!");
            }
        }
    }
    else
    {
        LOG4_WARN("%s() redisClusterAsyncContext(%s,%d) not found", __FUNCTION__,acc->cc->ip,acc->cc->port);
    }
    return(true);
}

const std::string& Worker::GetWorkerIdentify()
{
	if (m_strWorkerIdentify.size() == 0)
	{
		char szWorkerIdentify[64] = {0};
		snprintf(szWorkerIdentify, 64, "%s:%d.%d", GetHostForServer().c_str(),GetPortForServer(), GetWorkerIndex());
		m_strWorkerIdentify = szWorkerIdentify;
	}
	return(m_strWorkerIdentify);
}

time_t Worker::GetNowTime() const
{
    return((time_t)ev_now(m_loop));
}

bool Worker::RegisterCallback(Step* pStep, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pStep, dTimeout);
    if (pStep == NULL)return(false);
    if (pStep->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功
    {
        LOG4_WARN("Step(seq %u) registered already.",pStep->GetSequence());
        return(true);
    }
    pStep->SetRegistered();
    pStep->SetActiveTime(ev_now(m_loop));
    auto ret = m_mapCallbackStep.insert(std::pair<uint32, Step*>(pStep->GetSequence(), pStep));
    if (ret.second)
    {
        ev_timer* pTimeoutWatcher = (ev_timer*)malloc(sizeof(ev_timer));
        if (pTimeoutWatcher == NULL)
        {
            return(false);
        }
        if (0.0 == dTimeout)
        {
            pStep->SetTimeout(m_dStepTimeout);
        }
        else
        {
            pStep->SetTimeout(dTimeout);
        }
        ev_timer_init (pTimeoutWatcher, StepTimeoutCallback, pStep->GetTimeout() + ev_time() - ev_now(m_loop), 0.0);
        pStep->m_pTimeoutWatcher = pTimeoutWatcher;
        pTimeoutWatcher->data = (void*)pStep;
        ev_timer_start (m_loop, pTimeoutWatcher);
        LOG4_TRACE("Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pStep->GetSequence(), pStep->GetActiveTime(), pStep->GetTimeout());
    }
    else
    {
        LOG4_WARN("Step(seq %u) register failed.",pStep->GetSequence());
    }
    return(ret.second);
}

bool Worker::RegisterCallback(uint32 uiSelfStepSeq, Step* pStep, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pStep, dTimeout);
    if (pStep == NULL)return(false);

	if (pStep->m_setNextStepSeq.find(uiSelfStepSeq) != pStep->m_setNextStepSeq.end())// 登记前置step
	{
		std::unordered_map<uint32, Step*>::iterator callback_iter = m_mapCallbackStep.find(uiSelfStepSeq);
		if (callback_iter != m_mapCallbackStep.end())
		{
			callback_iter->second->m_setPreStepSeq.insert(pStep->GetSequence());
		}
	}
    if (pStep->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功
    {
        return(true);
    }
    pStep->SetRegistered();
    pStep->SetActiveTime(ev_now(m_loop));

    auto ret = m_mapCallbackStep.insert(std::pair<uint32, Step*>(pStep->GetSequence(), pStep));
    if (ret.second)
    {
        ev_timer* pTimeoutWatcher = (ev_timer*)malloc(sizeof(ev_timer));
        if (pTimeoutWatcher == NULL)
        {
            return(false);
        }
        if (0.0 == dTimeout)
        {
            pStep->SetTimeout(m_dStepTimeout);
        }
        else
        {
            pStep->SetTimeout(dTimeout);
        }
        ev_timer_init (pTimeoutWatcher, StepTimeoutCallback, pStep->GetTimeout() + ev_time() - ev_now(m_loop), 0.0);
        pStep->m_pTimeoutWatcher = pTimeoutWatcher;
        pTimeoutWatcher->data = (void*)pStep;
        ev_timer_start (m_loop, pTimeoutWatcher);
        LOG4_TRACE("Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pStep->GetSequence(), pStep->GetActiveTime(), pStep->GetTimeout());
    }
    return(ret.second);
}

void Worker::DeleteCallback(Step* pStep)
{
    LOG4_TRACE("%s(Step* 0x%X)", __FUNCTION__, pStep);
    if (pStep == NULL)
    {
        return;
    }
    std::unordered_map<uint32, Step*>::iterator callback_iter;
    for (std::set<uint32>::iterator step_seq_iter = pStep->m_setPreStepSeq.begin();
                    step_seq_iter != pStep->m_setPreStepSeq.end(); )
    {
        callback_iter = m_mapCallbackStep.find(*step_seq_iter);
        if (callback_iter == m_mapCallbackStep.end())
        {
            pStep->m_setPreStepSeq.erase(step_seq_iter++);
        }
        else
        {
            LOG4_DEBUG("step %u had pre step %u running, delay delete callback.", pStep->GetSequence(), *step_seq_iter);
            pStep->DelayTimeout();
            return;
        }
    }
    if (pStep->m_pTimeoutWatcher != NULL)
    {
        ev_timer_stop (m_loop, pStep->m_pTimeoutWatcher);
    }
    callback_iter = m_mapCallbackStep.find(pStep->GetSequence());
    if (callback_iter != m_mapCallbackStep.end())
    {
        LOG4_TRACE("delete step(seq %u)", pStep->GetSequence());
        delete pStep;
        m_mapCallbackStep.erase(callback_iter);
    }
}

void Worker::DeleteCallback(uint32 uiSelfStepSeq, Step* pStep)
{
    LOG4_TRACE("%s(self_seq[%u], Step* 0x%X)", __FUNCTION__, uiSelfStepSeq, pStep);
    if (pStep == NULL)
    {
        return;
    }
    std::unordered_map<uint32, Step*>::iterator callback_iter;
    for (auto step_seq_iter = pStep->m_setPreStepSeq.begin();step_seq_iter != pStep->m_setPreStepSeq.end(); )//检查前面的步骤，有则延长自己
    {
        callback_iter = m_mapCallbackStep.find(*step_seq_iter);
        if (callback_iter == m_mapCallbackStep.end())
        {
            LOG4_TRACE("try to erase seq[%u] from pStep->m_setPreStepSeq", *step_seq_iter);
            pStep->m_setPreStepSeq.erase(step_seq_iter++);
        }
        else
        {
            if (*step_seq_iter != uiSelfStepSeq)
            {
                LOG4_DEBUG("step[%u] try to delete step[%u], but step[%u] had pre step[%u] running, delay delete callback.",
                                uiSelfStepSeq, pStep->GetSequence(), pStep->GetSequence(), *step_seq_iter);
                pStep->DelayTimeout();
                return;
            }
            else
            {
                step_seq_iter++;
            }
        }
    }
    if (pStep->m_pTimeoutWatcher != NULL)
    {
        ev_timer_stop (m_loop, pStep->m_pTimeoutWatcher);
    }
    callback_iter = m_mapCallbackStep.find(pStep->GetSequence());
    if (callback_iter != m_mapCallbackStep.end())
    {
        LOG4_TRACE("step[%u] try to delete step[%u,%p]", uiSelfStepSeq, pStep->GetSequence(),pStep);
        delete pStep;
        m_mapCallbackStep.erase(callback_iter);
    }
}

bool Worker::RegisterCallback(Session* pSession)
{
    LOG4_TRACE("%s(Session* 0x%X, lifetime %lf)", __FUNCTION__, &pSession, pSession->GetTimeout());
    if (pSession == NULL)
    {
        return(false);
    }
    if (pSession->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功
    {
        LOG4_WARN("Session(session_id %s) registered already.", pSession->GetSessionId().c_str());
        return(true);
    }
    pSession->SetRegistered();
    pSession->SetActiveTime(ev_now(m_loop));

    std::pair<std::unordered_map<std::string, Session*>::iterator, bool> ret;
    std::unordered_map<std::string, std::unordered_map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(pSession->GetSessionClass());
    if (name_iter == m_mapCallbackSession.end())
    {
        std::unordered_map<std::string, Session*> mapSession;
        ret = mapSession.insert(std::pair<std::string, Session*>(pSession->GetSessionId(), pSession));
        if(!ret.second)
        {
            LOG4_WARN("insert Session(session_id %s).SessionClass(%s) register failed.", pSession->GetSessionId().c_str(),
                            pSession->GetSessionClass().c_str());
        }
        else
        {
            LOG4_TRACE("inserted Session(session_id %s).SessionClass(%s)", pSession->GetSessionId().c_str(),
                            pSession->GetSessionClass().c_str());
        }
        m_mapCallbackSession.insert(std::pair<std::string, std::unordered_map<std::string, Session*> >(pSession->GetSessionClass(), mapSession));
    }
    else
    {
        LOG4_TRACE("try insert Session(session_id %s).SessionClass(%s)", pSession->GetSessionId().c_str(),
                        pSession->GetSessionClass().c_str());
        ret = name_iter->second.insert(std::pair<std::string, Session*>(pSession->GetSessionId(), pSession));
        if(!ret.second)
        {
            LOG4_WARN("insert Session(session_id %s).SessionClass(%s) register failed.", pSession->GetSessionId().c_str(),
                            pSession->GetSessionClass().c_str());
        }
        else
        {
            LOG4_TRACE("inserted Session(session_id %s).SessionClass(%s)", pSession->GetSessionId().c_str(),
                            pSession->GetSessionClass().c_str());
        }
    }
    if (ret.second)
    {
        if (pSession->GetTimeout() != 0)
        {
            ev_timer* pTimeoutWatcher = (ev_timer*)malloc(sizeof(ev_timer));
            if (pTimeoutWatcher == NULL)
            {
                return(false);
            }
            ev_timer_init (pTimeoutWatcher, SessionTimeoutCallback, pSession->GetTimeout() + ev_time() - ev_now(m_loop), 0.0);
            pSession->m_pTimeoutWatcher = pTimeoutWatcher;
            pTimeoutWatcher->data = (void*)pSession;
            ev_timer_start (m_loop, pTimeoutWatcher);
        }
        LOG4_TRACE("Session(session_id %s) register successful.", pSession->GetSessionId().c_str());
    }
    else
    {
        LOG4_WARN("Session(session_id %s) register failed.", pSession->GetSessionId().c_str());
    }
    return(ret.second);
}

void Worker::DeleteCallback(Session* pSession)
{
    LOG4_TRACE("%s(Session* 0x%X)", __FUNCTION__, &pSession);
    if (pSession == NULL)
    {
        return;
    }
    if (pSession->m_pTimeoutWatcher != NULL)
    {
        ev_timer_stop (m_loop, pSession->m_pTimeoutWatcher);
    }
    std::unordered_map<std::string, std::unordered_map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(pSession->GetSessionClass());
    if (name_iter != m_mapCallbackSession.end())
    {
        std::unordered_map<std::string, Session*>::iterator id_iter = name_iter->second.find(pSession->GetSessionId());
        if (id_iter != name_iter->second.end())
        {
            LOG4_TRACE("delete session(session_id %s)", pSession->GetSessionId().c_str());
            delete id_iter->second;
            id_iter->second = NULL;
            name_iter->second.erase(id_iter);
        }
    }
}

bool Worker::RegisterCallback(const redisAsyncContext* pRedisContext, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (pRedisStep == NULL)
    {
        return(false);
    }
    if (pRedisStep->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功
    {
        return(true);
    }
    pRedisStep->SetRegistered();
    /* redis回调暂不作超时处理
    ev_timer* timeout_watcher = new ev_timer();
    if (timeout_watcher == NULL)
    {
        return(false);
    }
    tagIoWatcherData* pData = new tagIoWatcherData();
    if (pData == NULL)
    {
        LOG4_ERROR("new tagIoWatcherData error!");
        delete timeout_watcher;
        return(false);
    }
    ev_timer_init (timeout_watcher, IoTimeoutCallback, 0.5 + ev_time() - ev_now(m_loop), 0.0);
    pData->ullSeq = pStep->GetSequence();
    pData->pWorker = this;
    timeout_watcher->data = (void*)pData;
    ev_timer_start (m_loop, timeout_watcher);
    */

    std::unordered_map<redisAsyncContext*, tagRedisAttr*>::iterator iter = m_mapRedisAttr.find((redisAsyncContext*)pRedisContext);
    if (iter == m_mapRedisAttr.end())
    {
        LOG4_ERROR("redis attr not exist!");
        return(false);
    }
    else
    {
        LOG4_TRACE("iter->second->bIsReady = %d", iter->second->bIsReady);
        if (iter->second->bIsReady)
        {
            int status;
            size_t args_size = pRedisStep->GetRedisCmd()->GetCmdArguments().size() + 1;
            const char* argv[args_size];
            size_t arglen[args_size];
            argv[0] = pRedisStep->GetRedisCmd()->GetCmd().c_str();
            arglen[0] = pRedisStep->GetRedisCmd()->GetCmd().size();
            std::vector<std::pair<std::string, bool> >::const_iterator c_iter = pRedisStep->GetRedisCmd()->GetCmdArguments().begin();
            for (size_t i = 1; c_iter != pRedisStep->GetRedisCmd()->GetCmdArguments().end(); ++c_iter, ++i)
            {
                argv[i] = c_iter->first.c_str();
                arglen[i] = c_iter->first.size();
            }
            status = redisAsyncCommandArgv((redisAsyncContext*)pRedisContext, RedisCmdCallback, NULL, args_size, argv, arglen);
            if (status == REDIS_OK)
            {
                LOG4_DEBUG("succeed in sending redis cmd: %s", pRedisStep->GetRedisCmd()->ToString().c_str());
                iter->second->listData.push_back(pRedisStep);
                return(true);
            }
            else
            {
                LOG4_ERROR("redis status %d!", status);
                return(false);
            }
        }
        else
        {
            LOG4_TRACE("listWaitData.push_back()");
            iter->second->listWaitData.push_back(pRedisStep);
            return(true);
        }
    }
}

bool Worker::ResetTimeout(Step* pStep, struct ev_timer* watcher)
{
    ev_tstamp after = ev_now(m_loop) + pStep->GetTimeout();
    ev_timer_stop (m_loop, watcher);
    ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
    ev_timer_start (m_loop, watcher);
    LOG4_TRACE("%s() pStep %u after(%lf) set time(%lf) ev_now(%lf)",
    		__FUNCTION__,pStep->GetSequence(),after,after + ev_time() - ev_now(m_loop),ev_now(m_loop));
    return(true);
}

Session* Worker::GetSession(uint64 uiSessionId, const std::string& strSessionClass)
{
    std::unordered_map<std::string, std::unordered_map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(strSessionClass);
    if (name_iter == m_mapCallbackSession.end())
    {
        return(NULL);
    }
    else
    {
        char szSession[32] = {0};
        snprintf(szSession, sizeof(szSession), "%llu", uiSessionId);
        std::unordered_map<std::string, Session*>::iterator id_iter = name_iter->second.find(szSession);
        if (id_iter == name_iter->second.end())
        {
            LOG4_TRACE("szSession(%s).strSessionClass(%s) not exist",szSession,strSessionClass.c_str());
            return(NULL);
        }
        else
        {
            id_iter->second->SetActiveTime(ev_now(m_loop));
            return(id_iter->second);
        }
    }
}

Session* Worker::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    std::unordered_map<std::string, std::unordered_map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(strSessionClass);
    if (name_iter == m_mapCallbackSession.end())
    {
        return(NULL);
    }
    else
    {
        std::unordered_map<std::string, Session*>::iterator id_iter = name_iter->second.find(strSessionId);
        if (id_iter == name_iter->second.end())
        {
            return(NULL);
        }
        else
        {
            id_iter->second->SetActiveTime(ev_now(m_loop));
            return(id_iter->second);
        }
    }
}

bool Worker::Disconnect(const tagMsgShell& stMsgShell, bool bMsgShellNotice)
{
    std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter != m_mapFdAttr.end())
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
            LOG4_TRACE("if (iter->second->ulSeq == stMsgShell.ulSeq)");
            return(DestroyConnect(iter, bMsgShellNotice));
        }
    }
    return(false);
}

bool Worker::Disconnect(const std::string& strIdentify, bool bMsgShellNotice)
{
    std::unordered_map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        return(true);
    }
    else
    {
        return(Disconnect(shell_iter->second, bMsgShellNotice));
    }
}

bool Worker::SetProcessName(const util::CJsonObject& oJsonConf)
{
    char szProcessName[64] = {0};
    snprintf(szProcessName, sizeof(szProcessName), "%s_W%d", oJsonConf("server_name").c_str(), m_iWorkerIndex);
    ngx_setproctitle(szProcessName);
    return(true);
}

bool Worker::Init(util::CJsonObject& oJsonConf)
{
    char szProcessName[64] = {0};
    snprintf(szProcessName, sizeof(szProcessName), "%s_W%d", oJsonConf("server_name").c_str(), m_iWorkerIndex);
    ngx_setproctitle(szProcessName);
    oJsonConf.Get("io_timeout", m_dIoTimeout);
    if (!oJsonConf.Get("step_timeout", m_dStepTimeout))
    {
        m_dStepTimeout = 0.5;
    }
    oJsonConf.Get("node_type", m_strNodeType);
    oJsonConf.Get("inner_host", m_strHostForServer);
    oJsonConf.Get("inner_port", m_iPortForServer);
    m_oCustomConf = oJsonConf["custom"];
    oJsonConf["permission"]["uin_permit"].Get("stat_interval", m_dMsgStatInterval);//只有Gate类型的才有的配置
    oJsonConf["permission"]["uin_permit"].Get("permit_num", m_iMsgPermitNum);
    if (!InitLogger(oJsonConf))
    {
        return(false);
    }
    StarshipCodec* pCodec = new ProtoCodec(util::CODEC_PROTOBUF);
    m_mapCodec.insert(std::make_pair(util::CODEC_PROTOBUF, pCodec));
    pCodec = new HttpCodec(util::CODEC_HTTP);
    m_mapCodec.insert(std::make_pair(util::CODEC_HTTP, pCodec));
    pCodec = new ClientMsgCodec(util::CODEC_PRIVATE);
    m_mapCodec.insert(std::make_pair(util::CODEC_PRIVATE, pCodec));
    pCodec = new CodecWebSocketJson(util::CODEC_WEBSOCKET_EX_JS);
    m_mapCodec.insert(std::make_pair(util::CODEC_WEBSOCKET_EX_JS, pCodec));
    pCodec = new CodecWebSocketPb(util::CODEC_WEBSOCKET_EX_PB);
    m_mapCodec.insert(std::make_pair(util::CODEC_WEBSOCKET_EX_PB, pCodec));
    m_pCmdConnect = new CmdConnectWorker();
    if (m_pCmdConnect == NULL)
    {
        return(false);
    }
    bool bCpuAffinity = false;
	oJsonConf.Get("cpu_affinity", bCpuAffinity);
	if (bCpuAffinity)
	{
		/* get logical cpu number */
		int iCpuNum = sysconf(_SC_NPROCESSORS_CONF);                        ///< cpu数量
		cpu_set_t stCpuMask;                                                        ///< cpu set
		CPU_ZERO(&stCpuMask);
		CPU_SET(m_iWorkerIndex % iCpuNum, &stCpuMask);
		LOG4_INFO("sched_setaffinity iCpuNum(%d) iWorkerIndex(%d).",iCpuNum,m_iWorkerIndex);
		if (sched_setaffinity(0, sizeof(cpu_set_t), &stCpuMask) == -1)
		{
			LOG4_WARN("sched_setaffinity failed.");
		}
	}
    return(true);
}

bool Worker::InitLogger(const util::CJsonObject& oJsonConf)
{
    if (m_bInitLogger)  // 已经被初始化过，只修改日志级别
    {
        int32 iLogLevel = 0;
        oJsonConf.Get("log_level", iLogLevel);
        m_oLogger.setLogLevel(iLogLevel);
        return(true);
    }
    else
    {
        int32 iMaxLogFileSize = 0;
        int32 iMaxLogFileNum = 0;
        int32 iLogLevel = 0;
        int32 iLoggingPort = 9000;
        std::string strLoggingHost;
        std::string strLogname = m_strWorkPath + std::string("/") + oJsonConf("log_path")
                        + std::string("/") + getproctitle() + std::string(".log");
        std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
        std::ostringstream ssServerName;
        ssServerName << getproctitle() << " " << GetWorkerIdentify();
        oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
        oJsonConf.Get("max_log_file_num", iMaxLogFileNum);
        if (oJsonConf.Get("log_level", iLogLevel))
        {
            switch (iLogLevel)
            {
                case log4cplus::DEBUG_LOG_LEVEL:
                    break;
                case log4cplus::INFO_LOG_LEVEL:
                    break;
                case log4cplus::TRACE_LOG_LEVEL:
                    break;
                case log4cplus::WARN_LOG_LEVEL:
                    break;
                case log4cplus::ERROR_LOG_LEVEL:
                    break;
                case log4cplus::FATAL_LOG_LEVEL:
                    break;
                default:
                    iLogLevel = log4cplus::INFO_LOG_LEVEL;
            }
        }
        else
        {
            iLogLevel = log4cplus::INFO_LOG_LEVEL;
        }
        log4cplus::initialize();
        log4cplus::SharedAppenderPtr append(new log4cplus::RollingFileAppender(
                        strLogname, iMaxLogFileSize, iMaxLogFileNum));
        append->setName(strLogname);
        std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(strParttern));
        append->setLayout(layout);
        m_oLogger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(strLogname));
        m_oLogger.setLogLevel(iLogLevel);
        m_oLogger.addAppender(append);
        if (oJsonConf.Get("socket_logging_host", strLoggingHost) && oJsonConf.Get("socket_logging_port", iLoggingPort))
        {
            log4cplus::SharedAppenderPtr socket_append(new log4cplus::SocketAppender(
                            strLoggingHost, iLoggingPort, ssServerName.str()));
            socket_append->setName(ssServerName.str());
            socket_append->setLayout(layout);
            socket_append->setThreshold(log4cplus::INFO_LOG_LEVEL);
            m_oLogger.addAppender(socket_append);
        }
        LOG4_INFO("%s program begin...", getproctitle());
        m_bInitLogger = true;
        return(true);
    }
}

bool Worker::CreateEvents()
{
    m_loop = ev_loop_new(EVBACKEND_EPOLL);
    if (m_loop == NULL)
    {
        return(false);
    }

    signal(SIGPIPE, SIG_IGN);
    // 注册信号事件
    ev_signal* int_signal_watcher = new ev_signal();
    ev_signal_init (int_signal_watcher, TerminatedCallback, SIGINT);
    int_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, int_signal_watcher);

    ev_signal* kill_signal_watcher = new ev_signal();
	ev_signal_init (kill_signal_watcher, TerminatedCallback, SIGKILL);
	kill_signal_watcher->data = (void*)this;
	ev_signal_start (m_loop, kill_signal_watcher);

	ev_signal* term_signal_watcher = new ev_signal();
	ev_signal_init (term_signal_watcher, TerminatedCallback, SIGTERM);
	term_signal_watcher->data = (void*)this;
	ev_signal_start (m_loop, term_signal_watcher);

    AddPeriodicTaskEvent();
    // 注册闲时处理事件         注册idle事件在Server空闲时会导致CPU占用过高，暂时弃用之，改用定时器实现
//    ev_idle* idle_watcher = new ev_idle();
//    ev_idle_init (idle_watcher, IdleCallback);
//    idle_watcher->data = (void*)this;
//    ev_idle_start (m_loop, idle_watcher);

    // 注册网络IO事件
    uint32 ulSeq = GetSequence();
    if (CreateFdAttr(m_iManagerControlFd, ulSeq))
    {
        tagMsgShell stMsgShell;
        stMsgShell.iFd = m_iManagerControlFd;
        stMsgShell.ulSeq = ulSeq;
        std::unordered_map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(m_iManagerControlFd);
        if (!AddIoReadEvent(iter->second))
        {
            LOG4_TRACE("if (!AddIoReadEvent(conn_iter))");
            DestroyConnect(iter);
            return(false);
        }
        AddInnerFd(stMsgShell);
//        if (!AddIoErrorEvent(m_iManagerControlFd))
//        {
//            DestroyConnect(iter);
//            return(false);
//        }
    }
    else
    {
        return(false);
    }

    // 注册网络IO事件
    ulSeq = GetSequence();
    if (CreateFdAttr(m_iManagerDataFd, ulSeq))
    {
        tagMsgShell stMsgShell;
        stMsgShell.iFd = m_iManagerDataFd;
        stMsgShell.ulSeq = ulSeq;
        std::unordered_map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(m_iManagerDataFd);
        if (!AddIoReadEvent(iter->second))
        {
            LOG4_TRACE("if (!AddIoReadEvent(conn_iter))");
            DestroyConnect(iter);
            return(false);
        }
        AddInnerFd(stMsgShell);
//        if (!AddIoErrorEvent(m_iManagerDataFd))
//        {
//            DestroyConnect(iter);
//            return(false);
//        }
    }
    else
    {
        return(false);
    }
    return(true);
}

void Worker::PreloadCmd()
{
    Cmd* pCmdToldWorker = new CmdToldWorker();
    pCmdToldWorker->SetCmd(CMD_REQ_TELL_WORKER);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdToldWorker->GetCmd(), pCmdToldWorker));

    Cmd* pCmdUpdateNodeId = new CmdUpdateNodeId();
    pCmdUpdateNodeId->SetCmd(CMD_REQ_REFRESH_NODE_ID);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdUpdateNodeId->GetCmd(), pCmdUpdateNodeId));

    Cmd* pCmdNodeNotice = new CmdNodeNotice();
    pCmdNodeNotice->SetCmd(CMD_REQ_NODE_REG_NOTICE);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdNodeNotice->GetCmd(), pCmdNodeNotice));

    Cmd* pCmdBeat = new CmdBeat();
    pCmdBeat->SetCmd(CMD_REQ_BEAT);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdBeat->GetCmd(), pCmdBeat));

    Cmd* pCmdUpdateConfig = new CmdUpdateConfig();
    pCmdUpdateConfig->SetCmd(CMD_REQ_SERVER_CONFIG);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdUpdateConfig->GetCmd(), pCmdUpdateConfig));
}

void Worker::Destroy()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    m_mapHttpAttr.clear();
    for (auto&& cmd_iter:m_mapCmd)
    {
    	SAFE_DELETE(cmd_iter.second);
    }
    m_mapCmd.clear();

    for (auto&& so_iter:m_mapSo)
    {
    	SAFE_DELETE(so_iter.second);
    }
    m_mapSo.clear();

    for (auto&& module_iter:m_mapModule)
    {
    	SAFE_DELETE(module_iter.second);
    }
    m_mapModule.clear();

    for (auto attr_iter = m_mapFdAttr.begin();attr_iter != m_mapFdAttr.end(); ++attr_iter)
    {
        LOG4_TRACE("for (std::unordered_map<int, tagConnectionAttr*>::iterator attr_iter = m_mapFdAttr.begin();");
        DestroyConnect(attr_iter);
    }

    for (auto&& codec_iter:m_mapCodec)
    {
    	SAFE_DELETE(codec_iter.second);
    }
    m_mapCodec.clear();

    if (m_loop != NULL)
    {
        ev_loop_destroy(m_loop);
        m_loop = NULL;
    }
}

void Worker::ResetLogLevel(log4cplus::LogLevel iLogLevel)
{
    m_oLogger.setLogLevel(iLogLevel);
}

bool Worker::AddMsgShell(const std::string& strIdentify, const tagMsgShell& stMsgShell)
{
    LOG4_TRACE("%s(%s, fd %d, seq %u)", __FUNCTION__, strIdentify.c_str(), stMsgShell.iFd, stMsgShell.ulSeq);
    std::unordered_map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        m_mapMsgShell.insert(std::pair<std::string, tagMsgShell>(strIdentify, stMsgShell));
    }
    else
    {
        if ((stMsgShell.iFd != shell_iter->second.iFd || stMsgShell.ulSeq != shell_iter->second.ulSeq))
        {
            LOG4_DEBUG("%s() connect to %s was exist, replace old fd %d with new fd %d",
                            __FUNCTION__,strIdentify.c_str(), shell_iter->second.iFd, stMsgShell.iFd);
            std::unordered_map<int, tagConnectionAttr*>::iterator fd_iter = m_mapFdAttr.find(shell_iter->second.iFd);
            if (GetWorkerIdentify() != strIdentify)//外部连接
            {
//                DestroyConnect(fd_iter);
                LOG4_TRACE("%s() GetWorkerIdentify() != strIdentify. replace old stMsgShell(%d,%u) with new stMsgShell(%d,%u)",
                                __FUNCTION__,shell_iter->second.iFd,shell_iter->second.ulSeq,
                                stMsgShell.iFd,stMsgShell.ulSeq);
                shell_iter->second = stMsgShell;
            }
            else
            {
                LOG4_TRACE("%s() replace old stMsgShell(%d,%u) with new stMsgShell(%d,%u)",
                                __FUNCTION__,shell_iter->second.iFd,shell_iter->second.ulSeq,
                                stMsgShell.iFd,stMsgShell.ulSeq);
                shell_iter->second = stMsgShell;
            }
        }
    }
    SetConnectIdentify(stMsgShell, strIdentify);
    return(true);
}

void Worker::DelMsgShell(const std::string& strIdentify,const tagMsgShell& stMsgShell)
{
    std::unordered_map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        LOG4_TRACE("%s() strIdentify(%s) don't has stMsgShell",__FUNCTION__,strIdentify.c_str());
    }
    else
    {
        if(stMsgShell.iFd && stMsgShell.ulSeq)
        {
            if(stMsgShell.iFd == shell_iter->second.iFd && stMsgShell.ulSeq == shell_iter->second.ulSeq)
            {
                LOG4_TRACE("%s() strIdentify(%s) del map stMsgShell(%d,%u)",
                                __FUNCTION__,strIdentify.c_str(),shell_iter->second.iFd,shell_iter->second.ulSeq);
                m_mapMsgShell.erase(shell_iter);
            }
            else
            {
                LOG4_TRACE("%s() strIdentify(%s) del stMsgShell(%d,%u) and map stMsgShell(%d,%u) are not the same",
                                __FUNCTION__,strIdentify.c_str(),stMsgShell.iFd,stMsgShell.ulSeq,shell_iter->second.iFd,shell_iter->second.ulSeq);
            }
        }
        else
        {
            LOG4_TRACE("%s() strIdentify(%s) del map stMsgShell(%d,%u)",
                            __FUNCTION__,strIdentify.c_str(),shell_iter->second.iFd,shell_iter->second.ulSeq);
            m_mapMsgShell.erase(shell_iter);
        }
    }

    // 连接虽然断开，但不应清除节点标识符，这样可以保证下次有数据发送时可以重新建立连接
//    std::unordered_map<std::string, std::string>::iterator identify_iter = m_mapIdentifyNodeType.find(strIdentify);
//    if (identify_iter != m_mapIdentifyNodeType.end())
//    {
//        std::unordered_map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
//        node_type_iter = m_mapNodeIdentify.find(identify_iter->second);
//        if (node_type_iter != m_mapNodeIdentify.end())
//        {
//            std::set<std::string>::iterator id_iter = node_type_iter->second.second.find(strIdentify);
//            if (id_iter != node_type_iter->second.second.end())
//            {
//                node_type_iter->second.second.erase(id_iter);
//                node_type_iter->second.first = node_type_iter->second.second.begin();
//            }
//        }
//        m_mapIdentifyNodeType.erase(identify_iter);
//    }
}

void Worker::AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s(%s, %s)", __FUNCTION__, strNodeType.c_str(), strIdentify.c_str());
    std::unordered_map<std::string, std::string>::iterator iter = m_mapIdentifyNodeType.find(strIdentify);
    if (iter == m_mapIdentifyNodeType.end())
    {
        m_mapIdentifyNodeType.insert(iter,std::pair<std::string, std::string>(strIdentify, strNodeType));

        T_MAP_NODE_TYPE_IDENTIFY::iterator node_type_iter;
        node_type_iter = m_mapNodeIdentify.find(strNodeType);
        if (node_type_iter == m_mapNodeIdentify.end())
        {
            std::set<std::string,std::less<std::string> > setIdentify;
            setIdentify.insert(strIdentify);
            std::pair<T_MAP_NODE_TYPE_IDENTIFY::iterator, bool> insert_node_result;
            insert_node_result = m_mapNodeIdentify.insert(
				std::pair< std::string,std::pair<std::set<std::string>::iterator,std::set<std::string,std::less<std::string>>>>
            (strNodeType, std::make_pair(setIdentify.begin(), setIdentify)));
            //这里的setIdentify是临时变量，setIdentify.begin()将会成非法地址
            if (insert_node_result.second == false)
            {
                return;
            }
            insert_node_result.first->second.first = insert_node_result.first->second.second.begin();
        }
        else
        {
            std::set<std::string>::iterator id_iter = node_type_iter->second.second.find(strIdentify);
            if (id_iter == node_type_iter->second.second.end())
            {
                node_type_iter->second.second.insert(strIdentify);
                node_type_iter->second.first = node_type_iter->second.second.begin();
            }
        }
    }
    int iPosIpPortSeparator = strIdentify.find(':');//strHost:strPort
	if (iPosIpPortSeparator == std::string::npos)
	{
		LOG4_ERROR("strIdentify error: %s", strIdentify.c_str());
		return ;
	}
	std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
	std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, std::string::npos);
	int iPort = atoi(strPort.c_str());
	if (iPort == 0)
	{
		LOG4_ERROR("strIdentify error: %s", strIdentify.c_str());
		return ;
	}
    m_mapChannelConHash[strNodeType].addNode(ServerEntry(strIdentify,strHost,iPort));
}

void Worker::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s(%s, %s)", __FUNCTION__, strNodeType.c_str(), strIdentify.c_str());
    std::unordered_map<std::string, std::string>::iterator identify_iter = m_mapIdentifyNodeType.find(strIdentify);
    if (identify_iter != m_mapIdentifyNodeType.end())
    {
        std::unordered_map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
        node_type_iter = m_mapNodeIdentify.find(identify_iter->second);
        if (node_type_iter != m_mapNodeIdentify.end())
        {
            std::set<std::string>::iterator id_iter = node_type_iter->second.second.find(strIdentify);
            if (id_iter != node_type_iter->second.second.end())
            {
                node_type_iter->second.second.erase(id_iter);
                node_type_iter->second.first = node_type_iter->second.second.begin();
            }
        }
        m_mapIdentifyNodeType.erase(identify_iter);
    }
    m_mapChannelConHash[strNodeType].removeNode(strIdentify);
}

void Worker::GetNodeIdentifys(const std::string& strNodeType, std::vector<std::string>& strIdentifys)
{
    strIdentifys.clear();
    //std::unordered_map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >
    T_MAP_NODE_TYPE_IDENTIFY::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        return;
    }
    const std::pair<std::set<std::string>::iterator, std::set<std::string> >& identifySetPair = node_type_iter->second;
    const std::set<std::string>& identifySet = identifySetPair.second;
    std::set<std::string>::const_iterator identify_iter = identifySet.begin();
    std::set<std::string>::const_iterator identify_iterEnd = identifySet.end();
    for(;identify_iter != identify_iterEnd;++identify_iter)
    {
        strIdentifys.push_back(*identify_iter);
    }
}

bool Worker::RegisterCallback(const std::string& strIdentify, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, strIdentify.c_str());
    int iPosIpPortSeparator = strIdentify.find(',');
    if (iPosIpPortSeparator != std::string::npos)
    {
        return(AutoRedisCluster(strIdentify,pRedisStep));
    }
    iPosIpPortSeparator = strIdentify.find(':');
    if (iPosIpPortSeparator == std::string::npos)
    {
    	LOG4_ERROR("strIdentify error: %s", strIdentify.c_str());
        return(false);
    }
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    if (iPort == 0)
    {
    	LOG4_ERROR("strIdentify error: %s", strIdentify.c_str());
        return(false);
    }
    std::unordered_map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(strIdentify);
    if (ctx_iter != m_mapRedisContext.end())
    {
        LOG4_DEBUG("redis context %s", strIdentify.c_str());
        return(RegisterCallback(ctx_iter->second, pRedisStep));
    }
    else
    {
        LOG4_DEBUG("g_pLabor->AutoRedisCmd(%s, %d)", strHost.c_str(), iPort);
        return(AutoRedisCmd(strHost, iPort, pRedisStep));
    }
}

bool Worker::RegisterCallback(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s(%s, %d)", __FUNCTION__, strHost.c_str(), iPort);
    char szIdentify[32] = {0};
    snprintf(szIdentify, sizeof(szIdentify), "%s:%d", strHost.c_str(), iPort);
    std::unordered_map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(szIdentify);
    if (ctx_iter != m_mapRedisContext.end())
    {
        LOG4_TRACE("redis context %s", szIdentify);
        return(RegisterCallback(ctx_iter->second, pRedisStep));
    }
    else
    {
        LOG4_TRACE("g_pLabor->AutoRedisCmd(%s, %d)", strHost.c_str(), iPort);
        return(AutoRedisCmd(strHost, iPort, pRedisStep));
    }
}

bool Worker::AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx)
{
    LOG4_TRACE("%s(%s, %d, 0x%X)", __FUNCTION__, strHost.c_str(), iPort, ctx);
    char szIdentify[32] = {0};
    snprintf(szIdentify, 32, "%s:%d", strHost.c_str(), iPort);
    std::unordered_map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(szIdentify);
    if (ctx_iter == m_mapRedisContext.end())
    {
        m_mapRedisContext.insert(std::pair<std::string, const redisAsyncContext*>(szIdentify, ctx));
        std::unordered_map<const redisAsyncContext*, std::string>::iterator identify_iter = m_mapContextIdentify.find(ctx);
        if (identify_iter == m_mapContextIdentify.end())
        {
            m_mapContextIdentify.insert(std::pair<const redisAsyncContext*, std::string>(ctx, szIdentify));
        }
        else
        {
            identify_iter->second = szIdentify;
        }
        return(true);
    }
    else
    {
        return(false);
    }
}

void Worker::DelRedisContextAddr(const redisAsyncContext* ctx)
{
    std::unordered_map<const redisAsyncContext*, std::string>::iterator identify_iter = m_mapContextIdentify.find(ctx);
    if (identify_iter != m_mapContextIdentify.end())
    {
        std::unordered_map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(identify_iter->second);
        if (ctx_iter != m_mapRedisContext.end())
        {
            m_mapRedisContext.erase(ctx_iter);
        }
        m_mapContextIdentify.erase(identify_iter);
    }
}

bool Worker::RegisterCallback(MysqlStep* pMysqlStep)
{
#define iMysqlContextMapMaxSize (10)
	LOG4_TRACE("%s(%s, %d)", __FUNCTION__, pMysqlStep->m_strHost.c_str(), pMysqlStep->m_iPort);
	if (pMysqlStep->CurTask().size() == 0 || pMysqlStep->m_strHost.size() == 0 || 0 == pMysqlStep->m_iPort)//MysqlStep必须含mysql访问任务
    {
        LOG4_ERROR("%s() CurTask(%d) strHost(%d) iPort(%d)",
                __FUNCTION__,pMysqlStep->CurTask().size(),pMysqlStep->m_strHost.size(),pMysqlStep->m_iPort);
        return(false);
    }
	char szIdentify[64] = {0};
	snprintf(szIdentify, sizeof(szIdentify), "%s:%d:%s", pMysqlStep->m_strHost.c_str(),
			pMysqlStep->m_iPort,pMysqlStep->m_strDbName.c_str());
	MysqlContextMap::iterator ctx_iter = m_mapMysqlContext.find(szIdentify);
	if (ctx_iter != m_mapMysqlContext.end())//每个连接符对应最多10连接
	{
		if (ctx_iter->second.second.size() < iMysqlContextMapMaxSize)
		{//new conn
			return(AutoMysqlCmd(pMysqlStep));
		}
		util::MysqlAsyncConn* pMysqlConn(NULL);
		if (ctx_iter->second.first == ctx_iter->second.second.end())
		{
			ctx_iter->second.first = ctx_iter->second.second.begin();
		}
		pMysqlConn = *ctx_iter->second.first;
		++ctx_iter->second.first;
		LOG4_TRACE("mysql conn %s", szIdentify);
		return(RegisterCallback(pMysqlConn, pMysqlStep));
	}
	else
	{//new conn
		LOG4_TRACE("AutoMysqlCmd(%s, %d)", pMysqlStep->m_strHost.c_str(), pMysqlStep->m_iPort);
		return(AutoMysqlCmd(pMysqlStep));
	}
}

bool Worker::RegisterCallback(util::MysqlAsyncConn* pMysqlContext, MysqlStep* pMysqlStep)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (pMysqlStep == NULL)
    {
        return(false);
    }
    static uint64 uiAddTaskCounter = 0;
	++uiAddTaskCounter;
    if (!pMysqlStep->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功,任务会继续投递
    {
		pMysqlStep->SetRegistered();
		LOG4_TRACE("register pMysqlStep ok");
    }
    if (pMysqlStep->m_strCmd.size() > 0)
    {
    	LOG4_TRACE("succeed in add mysql cmd: %s:%u uiAddTaskCounter:%llu",pMysqlStep->m_strCmd.c_str(),pMysqlStep->m_uiCmdType,uiAddTaskCounter);
    	util::SqlTask *task = new util::SqlTask(pMysqlStep->m_strCmd,(util::eSqlTaskOper)pMysqlStep->m_uiCmdType,new CustomMysqlHandler(pMysqlStep));
		pMysqlContext->add_task(task);
		pMysqlStep->ClearTask();
    }
	return(true);
}

bool Worker::AddMysqlContextAddr(MysqlStep* pMysqlStep, util::MysqlAsyncConn* ctx)
{
    LOG4_TRACE("%s(%s, %d, 0x%X)", __FUNCTION__, pMysqlStep->m_strHost.c_str(), pMysqlStep->m_iPort, ctx);
    char szIdentify[64] = {0};
	snprintf(szIdentify, sizeof(szIdentify), "%s:%d:%s", pMysqlStep->m_strHost.c_str(),
			pMysqlStep->m_iPort,pMysqlStep->m_strDbName.c_str());
    MysqlContextMap::iterator ctx_iter = m_mapMysqlContext.find(szIdentify);
    if (ctx_iter == m_mapMysqlContext.end())
    {
    	std::set<util::MysqlAsyncConn*> set;
    	set.insert(ctx);
    	m_mapMysqlContext.insert(std::pair<std::string, std::pair<std::set<util::MysqlAsyncConn*>::iterator,std::set<util::MysqlAsyncConn*> > >
    	(szIdentify,std::make_pair(set.begin(),set)));
        std::unordered_map<util::MysqlAsyncConn*, std::string>::iterator identify_iter = m_mapMysqlContextIdentify.find(ctx);
        if (identify_iter == m_mapMysqlContextIdentify.end())
        {
        	m_mapMysqlContextIdentify.insert(std::pair<util::MysqlAsyncConn*, std::string>(ctx, szIdentify));
        }
        else
        {
            identify_iter->second = szIdentify;
        }
        return(true);
    }
    else
    {
    	ctx_iter->second.second.insert(ctx);
    	ctx_iter->second.first = ctx_iter->second.second.begin();
        return(false);
    }
}

void Worker::DelMysqlContextAddr(util::MysqlAsyncConn* ctx)
{
    std::unordered_map<util::MysqlAsyncConn*, std::string>::iterator identify_iter = m_mapMysqlContextIdentify.find(ctx);
    if (identify_iter != m_mapMysqlContextIdentify.end())
    {
    	MysqlContextMap::iterator ctx_iter = m_mapMysqlContext.find(identify_iter->second);
        if (ctx_iter != m_mapMysqlContext.end())
        {
        	ctx_iter->second.second.erase(ctx);
        }
        m_mapMysqlContextIdentify.erase(identify_iter);
    }
}

bool Worker::SendTo(const tagMsgShell& stMsgShell)
{
    LOG4_TRACE("%s(fd %d, seq %lu) pWaitForSendBuff", __FUNCTION__, stMsgShell.iFd, stMsgShell.ulSeq);
    std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd %d found in m_mapFdAttr", stMsgShell.iFd);
        return(false);
    }
    else
    {
    	tagConnectionAttr* pConn = iter->second;
        if (pConn->ulSeq == stMsgShell.ulSeq && pConn->iFd == stMsgShell.iFd)
        {
            int iErrno = 0;
            int iWriteLen = 0;
            int iNeedWriteLen = (int)(pConn->pWaitForSendBuff->ReadableBytes());
            int iWriteIdx = pConn->pSendBuff->GetWriteIndex();
            iWriteLen = pConn->pSendBuff->Write(pConn->pWaitForSendBuff, pConn->pWaitForSendBuff->ReadableBytes());
            if (iWriteLen == iNeedWriteLen)
            {
                iNeedWriteLen = (int)pConn->pSendBuff->ReadableBytes();
                iWriteLen = pConn->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
                iter->second->pSendBuff->Compact(8192);
                if (iWriteLen < 0)
                {
                    if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                    {
                        LOG4_ERROR("send to fd %d error %d: %s",stMsgShell.iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        DestroyConnect(iter);
                        return(false);
                    }
                    else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                    {
                        iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(pConn);
                    }
                }
                else if (iWriteLen > 0)
                {
                    m_iSendByte += iWriteLen;
                    iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        RemoveIoWriteEvent(pConn);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        AddIoWriteEvent(pConn);
                    }
                }
                return(true);
            }
            else
            {
                LOG4_ERROR("write to send buff error, iWriteLen = %d!", iWriteLen);
                iter->second->pSendBuff->SetWriteIndex(iWriteIdx);
                return(false);
            }
        }
        else
        {
        	LOG4_ERROR("pConn ulSeq(%llu) iFd(%d) stMsgShell ulSeq(%llu) iFd(%d) not match",pConn->ulSeq,pConn->iFd,stMsgShell.ulSeq,stMsgShell.iFd);
        }
    }
    return(false);
}

bool Worker::ParseMsgBody(const MsgBody& oInMsgBody,google::protobuf::Message &message)
{
    if (oInMsgBody.sbody().size() > 0)//websocket json
    {
        google::protobuf::util::JsonParseOptions oOption;
        google::protobuf::util::Status oStatus = google::protobuf::util::JsonStringToMessage(oInMsgBody.sbody(),&message, oOption);
        if(!oStatus.ok())
        {
            LOG4_ERROR("%s() json sbody to MsgBody error(%s)!sbody(%s) message.descriptor(%s)",
                                __FUNCTION__,oStatus.ToString().c_str(),oInMsgBody.sbody().c_str(),
                                message.GetDescriptor()->DebugString().c_str());
            return (false);
        }
        return true;
    }
    else if (oInMsgBody.body().size() > 0)//privage pb
    {
        if (!message.ParseFromString(oInMsgBody.body()))
        {
            LOG4_ERROR("%s() oInMsgBody body to message error!message.descriptor(%s)",
                            __FUNCTION__,message.GetDescriptor()->DebugString().c_str());
            return (false);
        }
        return true;
    }
    LOG4_DEBUG("%s() empty msg body and sbody",__FUNCTION__);//如果请求消息体为空，则请求内容为空
    return true;
}

bool Worker::SendToClient(const tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const google::protobuf::Message &message,const std::string& additional,uint64 sessionid,const std::string& strSession,bool boJsonBody)
{
    MsgBody oMsgBody;
    MsgHead oOutMsgHead;
	oOutMsgHead.set_seq(oInMsgHead.seq());
	oOutMsgHead.set_cmd(oInMsgHead.cmd()+1);
    BuildMsgBody(oOutMsgHead,oMsgBody,message,additional,sessionid,strSession,boJsonBody);//会设置oOutMsgHead.msgbody_len
    LOG4_TRACE("%s oInMsgHead(cmd:%u),oOutMsgHead(cmd:%u msgbody_len:%u)",__FUNCTION__,oInMsgHead.cmd(),oOutMsgHead.cmd(),oOutMsgHead.msgbody_len());
    return SendTo(stMsgShell,oOutMsgHead,oMsgBody);
}

bool Worker::SendToClient(const std::string& strIdentify,const MsgHead& oInMsgHead,const google::protobuf::Message &message,const std::string& additional,uint64 sessionid,const std::string& strSession,bool boJsonBody)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    MsgBody oMsgBody;
    MsgHead oOutMsgHead;
    oOutMsgHead.set_seq(oInMsgHead.seq());
    oOutMsgHead.set_cmd(oInMsgHead.cmd()+1);
    BuildMsgBody(oOutMsgHead,oMsgBody,message,additional,sessionid,strSession,boJsonBody);//会设置oOutMsgHead.msgbody_len
    std::unordered_map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        LOG4_TRACE("no tagMsgShell match %s.", strIdentify.c_str());
        return(AutoSend(strIdentify, oOutMsgHead, oMsgBody));
    }
    else
    {
        return(SendTo(shell_iter->second, oOutMsgHead, oMsgBody));
    }
}

bool Worker::SendToClient(const tagMsgShell& stInMsgShell,const MsgHead& oInMsgHead,const std::string &strBody)
{
	MsgHead oOutMsgHead;
	MsgBody oOutMsgBody;
	oOutMsgBody.set_body(strBody);
	oOutMsgHead.set_seq(oInMsgHead.seq());
	oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
	oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
	if (!SendTo(stInMsgShell, oOutMsgHead, oOutMsgBody))
	{
		LOG4_ERROR("send to tagMsgShell(fd %d, seq %u) error!", stInMsgShell.iFd, stInMsgShell.ulSeq);
		return false;
	}
	return true;
}

bool Worker::SendToClient(const tagMsgShell& stInMsgShell,const HttpMsg& oInHttpMsg,const std::string &strBody,int iCode,const std::unordered_map<std::string,std::string> &heads)
{
	HttpMsg oHttpMsg;
	for(const auto & iter:heads)
	{
		::HttpMsg_Header* head = oHttpMsg.add_headers();
		head->set_header_name(iter.first);
		head->set_header_value(iter.second);
	}
	oHttpMsg.set_type(HTTP_RESPONSE);
	oHttpMsg.set_status_code(iCode);
	oHttpMsg.set_http_major(oInHttpMsg.http_major());
	oHttpMsg.set_http_minor(oInHttpMsg.http_minor());
	oHttpMsg.set_body(strBody);
	if (!SendTo(stInMsgShell, oHttpMsg))
	{
		LOG4_ERROR("send to tagMsgShell(fd %d, seq %u) error!", stInMsgShell.iFd, stInMsgShell.ulSeq);
		return false;
	}
	return true;
}

bool Worker::BuildMsgBody(MsgHead& oMsgHead,MsgBody &oMsgBody,const google::protobuf::Message &message,const std::string& additional,uint64 sessionid,const std::string& strSession,bool boJsonBody)
{
    oMsgBody.Clear();
    if (boJsonBody)
    {
        std::string strJson;
        google::protobuf::util::JsonPrintOptions oOption;
        google::protobuf::util::Status oStatus = google::protobuf::util::MessageToJsonString(message,&strJson,oOption);
        if(!oStatus.ok())
        {
            LOG4_ERROR("MessageToJsonString failed error(%u,%s)",oStatus.error_code(),oStatus.error_message().ToString().c_str());
            return false;
        }
        oMsgBody.set_sbody(strJson);
    }
    oMsgBody.set_body(message.SerializeAsString());

    if(additional.size() > 0)oMsgBody.set_additional(additional);
    if(sessionid > 0)oMsgBody.set_session_id(sessionid);
    if(strSession.size() > 0)oMsgBody.set_session(strSession);

    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    return true;
}

bool Worker::SendToCallback(net::Session* pSession,const DataMem::MemOperate* pMemOper,SessionCallbackMem callback,const std::string &nodeType,uint32 uiCmd,int64 uiModFactor)
{
	if (!pSession)
	{
		LOG4_ERROR("pSession empty!");
		return false;
	}
	StepNode* pStep = new StepNode(pMemOper);
    if (pStep == NULL)
    {
        LOG4_ERROR("new StepNode() error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (!RegisterCallback(pStep))
    {
        LOG4_ERROR("RegisterCallback(pStep) error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    pStep->SetCallBack(callback,pSession,nodeType,uiCmd,uiModFactor);
    if (net::STATUS_CMD_RUNNING != pStep->Emit(ERR_OK))
    {
        DeleteCallback(pStep);
        return(false);
    }
    return true;
}

bool Worker::SendToCallback(net::Step* pUpperStep,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,const std::string &nodeType,uint32 uiCmd,int64 uiModFactor)
{
	if (!pUpperStep)
	{
		LOG4_ERROR("pUpperStep null!");
		return false;
	}
	if (!pUpperStep->IsRegistered())
	{
		if (!RegisterCallback(pUpperStep))
		{
			LOG4_ERROR("RegisterCallback(pUpperStep) error!");
			delete pUpperStep;
			pUpperStep = NULL;
			return(false);
		}
		LOG4_TRACE("RegisterCallback(pUpperStep)");
	}
	else
	{
		pUpperStep->DelayDel();//已经注册的需要延迟删除
	}
	StepNode* pStep = new StepNode(pMemOper);
    if (pStep == NULL)
    {
        LOG4_ERROR("new StepNode() error!");
        return(false);
    }
    if (!RegisterCallback(pStep))
    {
        LOG4_ERROR("RegisterCallback(pStep) error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    pStep->SetCallBack(callback,pUpperStep,nodeType,uiCmd,uiModFactor);
	pUpperStep->AddPreStepSeq(pStep);
    if (net::STATUS_CMD_RUNNING != pStep->Emit(ERR_OK))
    {
        DeleteCallback(pStep);
        return(false);
    }
    return true;
}

bool Worker::SendToCallback(net::Session* pSession,uint32 uiCmd,const std::string &strBody,SessionCallback callback,const std::string &nodeType,int64 uiModFactor)
{
	StepNode* pStep = new StepNode(strBody);
    if (pStep == NULL)
    {
        LOG4_ERROR("new StepNode() error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (!RegisterCallback(pStep))
    {
        LOG4_ERROR("RegisterCallback(pStep) error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    pStep->SetCallBack(callback,pSession,nodeType,uiCmd,uiModFactor);
    if (net::STATUS_CMD_RUNNING != pStep->Emit(ERR_OK))
    {
        DeleteCallback(pStep);
        return(false);
    }
    return true;
}

bool Worker::SendToCallback(net::Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,const std::string &nodeType,int64 uiModFactor)
{
	if (!pUpperStep)
	{
		LOG4_ERROR("pUpperStep null!");
		return false;
	}
	if (!pUpperStep->IsRegistered())
	{
		if (!RegisterCallback(pUpperStep))
		{
			LOG4_ERROR("RegisterCallback(pUpperStep) error!");
			delete pUpperStep;
			pUpperStep = NULL;
			return(false);
		}
		LOG4_TRACE("RegisterCallback(pUpperStep)");
	}
	else
	{
		pUpperStep->DelayDel();//已经注册的需要延迟删除
	}
	StepNode* pStep = new StepNode(strBody);
    if (pStep == NULL)
    {
        LOG4_ERROR("new StepNode() error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (!RegisterCallback(pStep))
    {
        LOG4_ERROR("RegisterCallback(pStep) error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    pStep->SetCallBack(callback,pUpperStep,nodeType,uiCmd,uiModFactor);
    pUpperStep->AddPreStepSeq(pStep);
    if (net::STATUS_CMD_RUNNING != pStep->Emit(ERR_OK))
    {
        DeleteCallback(pStep);
        return(false);
    }
    return true;
}

bool Worker::SendToCallback(net::Session* pSession,uint32 uiCmd,const std::string &strBody,SessionCallback callback,const tagMsgShell& stMsgShell,int64 uiModFactor)
{
	StepNode* pStep = new StepNode(strBody);
    if (pStep == NULL)
    {
        LOG4_ERROR("new StepNode() error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (!RegisterCallback(pStep))
    {
        LOG4_ERROR("RegisterCallback(pStep) error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    pStep->SetCallBack(callback,pSession,stMsgShell,uiCmd,uiModFactor);
    if (net::STATUS_CMD_RUNNING != pStep->Emit(ERR_OK))
    {
        DeleteCallback(pStep);
        return(false);
    }
    return true;
}

bool Worker::SendToCallback(net::Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,const tagMsgShell& stMsgShell,int64 uiModFactor)
{
	if (!pUpperStep)
	{
		LOG4_ERROR("pUpperStep null!");
		return false;
	}
	if (!pUpperStep->IsRegistered())
	{
		if (!RegisterCallback(pUpperStep))
		{
			LOG4_ERROR("RegisterCallback(pUpperStep) error!");
			delete pUpperStep;
			pUpperStep = NULL;
			return(false);
		}
		LOG4_TRACE("RegisterCallback(pUpperStep)");
	}
	else
	{
		pUpperStep->DelayDel();//已经注册的需要延迟删除
	}
	StepNode* pStep = new StepNode(strBody);
    if (pStep == NULL)
    {
        LOG4_ERROR("new StepNode() error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (!RegisterCallback(pStep))
    {
        LOG4_ERROR("RegisterCallback(pStep) error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    pStep->SetCallBack(callback,pUpperStep,stMsgShell,uiCmd,uiModFactor);
    pUpperStep->AddPreStepSeq(pStep);
    if (net::STATUS_CMD_RUNNING != pStep->Emit(ERR_OK))
    {
        DeleteCallback(pStep);
        return(false);
    }
    return true;
}

bool Worker::SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(fd %d, fd_seq %lu, cmd %u, msg_seq %u)",
                    __FUNCTION__, stMsgShell.iFd, stMsgShell.ulSeq, oMsgHead.cmd(), oMsgHead.seq());
    std::unordered_map<int, tagConnectionAttr*>::iterator conn_iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (conn_iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd %d found in m_mapFdAttr", stMsgShell.iFd);
        return(false);
    }
    else
    {
    	tagConnectionAttr* pConn = conn_iter->second;
        if (pConn->ulSeq == stMsgShell.ulSeq && pConn->iFd == stMsgShell.iFd)
        {
            auto codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", conn_iter->second->eCodecType);
                DestroyConnect(conn_iter);
                return(false);
            }
            E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
            if (util::CODEC_PROTOBUF == pConn->eCodecType)//内部协议需要检查连接过程
            {
                LOG4_TRACE("connect status %u", pConn->ucConnectStatus);
                if (eConnectStatus_ok != conn_iter->second->ucConnectStatus)   // 连接尚未完成
                {
                    if (oMsgHead.cmd() <= CMD_RSP_TELL_WORKER)   // 创建连接的过程
                    {
                        LOG4_TRACE("codec_iter->second->Encode,oMsgHead.cmd(%u),connect status %u",
                                        oMsgHead.cmd(),conn_iter->second->ucConnectStatus);
                        eCodecStatus = codec_iter->second->Encode(oMsgHead, oMsgBody, conn_iter->second->pSendBuff);
                        if(oMsgHead.cmd() == CMD_RSP_TELL_WORKER)
                        {
                            conn_iter->second->ucConnectStatus = eConnectStatus_ok;
                        }
                        else
                        {
                            conn_iter->second->ucConnectStatus = eConnectStatus_connecting;
                        }
                    }
                    else    // 创建连接过程中的其他数据发送请求
                    {
                        LOG4_TRACE("codec_iter->second->Encode,oMsgHead.cmd(%u),connect status %u",
                                                                oMsgHead.cmd(),conn_iter->second->ucConnectStatus);
                        eCodecStatus = codec_iter->second->Encode(oMsgHead, oMsgBody, conn_iter->second->pWaitForSendBuff);
                        if (CODEC_STATUS_OK == eCodecStatus)//其他请求在连接过程中先不发送
                        {
                            return(true);
                        }
                        return(false);
                    }
                }
                else//连接已完成
                {
                    LOG4_TRACE("codec_iter->second->Encode,oMsgHead.cmd(%u),connect status %u",
                                                            oMsgHead.cmd(),conn_iter->second->ucConnectStatus);
                    eCodecStatus = codec_iter->second->Encode(oMsgHead, oMsgBody, conn_iter->second->pSendBuff);
                }
            }
            else
            {
                LOG4_TRACE("codec_iter->second->Encode,oMsgHead.cmd(%u),connect status %u",
                                                                            oMsgHead.cmd(),conn_iter->second->ucConnectStatus);
                eCodecStatus = codec_iter->second->Encode(oMsgHead, oMsgBody, conn_iter->second->pSendBuff);
            }
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                ++m_iSendNum;
                int iErrno = 0;
                int iNeedWriteLen = (int)conn_iter->second->pSendBuff->ReadableBytes();
                LOG4_TRACE("try send cmd[%d] seq[%lu] len %d to fd %d ip %s identify %s",
					oMsgHead.cmd(), oMsgHead.seq(), iNeedWriteLen, stMsgShell.iFd,pConn->pRemoteAddr, pConn->strIdentify.c_str());
                int iWriteLen = pConn->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
                pConn->pSendBuff->Compact(8192);
                if (iWriteLen < 0)
                {
                    if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                    {
                        LOG4_ERROR("send to fd %d error %d: %s",
                                        stMsgShell.iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        DestroyConnect(conn_iter);
                        return(false);
                    }
                    else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                    {
                        LOG4_TRACE("write len %d, errno %d: %s",
                                        iWriteLen, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        pConn->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(pConn);
                    }
                    else
                    {
                        LOG4_TRACE("write len %d, errno %d: %s",
                                        iWriteLen, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        pConn->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(pConn);
                    }
                }
                else if (iWriteLen > 0)
                {
                    m_iSendByte += iWriteLen;
                    conn_iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        LOG4_TRACE("cmd[%d] seq[%lu] to fd %d ip %s identify %s need write %d and had written len %d",
								oMsgHead.cmd(), oMsgHead.seq(), stMsgShell.iFd,conn_iter->second->pRemoteAddr, conn_iter->second->strIdentify.c_str(),
								iNeedWriteLen, iWriteLen);
                        RemoveIoWriteEvent(pConn);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        LOG4_TRACE("cmd[%d] seq[%lu] need write %d and had written len %d",
                                        oMsgHead.cmd(), oMsgHead.seq(), iNeedWriteLen, iWriteLen);
                        AddIoWriteEvent(pConn);
                    }
                }
                return(true);
            }
            else
            {
                LOG4_WARN("codec_iter->second->Encode failed,oMsgHead.cmd(%u),connect status %u",oMsgHead.cmd(),conn_iter->second->ucConnectStatus);
                return(false);
            }
        }
        else
        {
            LOG4_ERROR("fd %d sequence %lu not match the iFd %d sequence %lu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second->iFd,conn_iter->second->ulSeq);
            return(false);
        }
    }
}

bool Worker::SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::unordered_map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        LOG4_TRACE("no tagMsgShell match %s.", strIdentify.c_str());
        return(AutoSend(strIdentify, oMsgHead, oMsgBody));
    }
    else
    {
        return(SendTo(shell_iter->second, oMsgHead, oMsgBody));
    }
}

bool Worker::SendToSession(const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
	bool bSendResult = false;
	if (oMsgBody.has_session_id())
	{
		char szIdentify[32] = {0};
		snprintf(szIdentify, sizeof(szIdentify), "%u", oMsgBody.session_id());
		bSendResult = SendTo(szIdentify, oOutMsgHead, m_oReqMsgBody);
	}
	else if (oMsgBody.has_session())
	{
		bSendResult = SendTo(oMsgBody.session(), oMsgHead, oMsgBody);
	}
	else
	{
		LOG4_WARN("no session id");
	}
	return bSendResult;
}

bool Worker::SendToSession(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
	if (oMsgBody.has_session_id())
	{
		return SendToWithMod(strNodeType, oMsgBody.session_id(), oMsgHead, oMsgBody);
	}
	else if (oMsgBody.has_session())
	{
		unsigned int uiSessionFactor = 0;
		for (unsigned int i = 0; i < oMsgBody.session().size(); ++i)
		{
			uiSessionFactor += oMsgBody.session()[i];
		}
		return SendToWithMod(strNodeType, uiSessionFactor, oMsgHead, oMsgBody);
	}
	else
	{
		return SendToNext(strNodeType, oMsgHead, oMsgBody);
	}
}


bool Worker::SendToNext(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(node_type: %s)", __FUNCTION__, strNodeType.c_str());
    std::unordered_map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no tagMsgShell match %s!", strNodeType.c_str());
        return(false);
    }
    else
    {
        if (node_type_iter->second.first != node_type_iter->second.second.end())
        {
            std::set<std::string>::iterator id_iter = node_type_iter->second.first;
            node_type_iter->second.first++;
            return(SendTo(*id_iter, oMsgHead, oMsgBody));
        }
        else
        {
            std::set<std::string>::iterator id_iter = node_type_iter->second.second.begin();
            if (id_iter != node_type_iter->second.second.end())
            {
                node_type_iter->second.first = id_iter;
                node_type_iter->second.first++;
                return(SendTo(*id_iter, oMsgHead, oMsgBody));
            }
            else
            {
                LOG4_ERROR("no tagMsgShell match and no node identify found for %s!", strNodeType.c_str());
                return(false);
            }
        }
    }
}

bool Worker::SendToWithMod(const std::string& strNodeType, uint32 uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
	LOG4_TRACE("%s(nody_type: %s, mod_factor: %u)", __FUNCTION__, strNodeType.c_str(), uiModFactor);
#ifdef USE_CONHASH
    return SendToConHash(strNodeType,uiModFactor,oMsgHead,oMsgBody);
#endif
    std::unordered_map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no tagMsgShell match %s!", strNodeType.c_str());
        return(false);
    }
    else
    {
        if (node_type_iter->second.second.size() == 0)
        {
            LOG4_ERROR("no tagMsgShell match %s!", strNodeType.c_str());
            return(false);
        }
        else
        {
            std::set<std::string>::iterator id_iter;
            int target_identify = uiModFactor % node_type_iter->second.second.size();
            int i = 0;
            for (i = 0, id_iter = node_type_iter->second.second.begin();
                            i < node_type_iter->second.second.size();
                            ++i, ++id_iter)
            {
                if (i == target_identify && id_iter != node_type_iter->second.second.end())
                {
                    return(SendTo(*id_iter, oMsgHead, oMsgBody));// SendTo(identify: 192.168.11.66:16068.0)
                }
            }
            return(false);
        }
    }
}


bool Worker::SendToConHash(const std::string& strNodeType, uint32 uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    std::string strIdentify =  m_mapChannelConHash[strNodeType].lookupNodeIdentify(uiModFactor);
    LOG4_TRACE("%s(nody_type: %s, mod_factor: %u),strIdentify:%s", __FUNCTION__, strNodeType.c_str(), uiModFactor,strIdentify.c_str());
    if (strIdentify.size() == 0)
    {
    	LOG4_ERROR("%s no strIdentify match %s!", __FUNCTION__,strIdentify.c_str());
		return(false);
    }
    return SendTo(strIdentify, oMsgHead, oMsgBody);
}


bool Worker::SendToNodeType(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(node_type: %s)", __FUNCTION__, strNodeType.c_str());
    std::unordered_map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no tagMsgShell match %s!", strNodeType.c_str());
        return(false);
    }
    else
    {
        int iSendNum = 0;
        for (std::set<std::string>::iterator id_iter = node_type_iter->second.second.begin();
                        id_iter != node_type_iter->second.second.end(); ++id_iter)
        {
            if (*id_iter != GetWorkerIdentify())
            {
                SendTo(*id_iter, oMsgHead, oMsgBody);
            }
            ++iSendNum;
        }
        if (0 == iSendNum)
        {
            LOG4_ERROR("no tagMsgShell match and no node identify found for %s!", strNodeType.c_str());
            return(false);
        }
    }
    return(true);
}

bool Worker::SendTo(const tagMsgShell& stMsgShell, const HttpMsg& oHttpMsg, HttpStep* pHttpStep)
{
    LOG4_TRACE("%s(fd %d, seq %lu)", __FUNCTION__, stMsgShell.iFd, stMsgShell.ulSeq);
    std::unordered_map<int, tagConnectionAttr*>::iterator conn_iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (conn_iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd %d found in m_mapFdAttr", stMsgShell.iFd);
        return(false);
    }
    else
    {
    	tagConnectionAttr* pConn = conn_iter->second;
        if (pConn->ulSeq == stMsgShell.ulSeq && pConn->iFd == stMsgShell.iFd)
        {
            auto codec_iter = m_mapCodec.find(pConn->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", pConn->eCodecType);
                DestroyConnect(conn_iter);
                return(false);
            }
            E_CODEC_STATUS eCodecStatus;
            if(util::CODEC_WEBSOCKET_EX_PB == pConn->eCodecType)
            {
                if (conn_iter->second->pWaitForSendBuff->ReadableBytes() > 0)   // 正在连接
                {
                    eCodecStatus = ((CodecWebSocketPb*)codec_iter->second)->Encode(oHttpMsg, pConn->pWaitForSendBuff);
                }
                else
                {
                    eCodecStatus = ((CodecWebSocketPb*)codec_iter->second)->Encode(oHttpMsg, pConn->pSendBuff);
                }
            }
            else if(util::CODEC_WEBSOCKET_EX_JS == conn_iter->second->eCodecType)
            {
                if (conn_iter->second->pWaitForSendBuff->ReadableBytes() > 0)   // 正在连接
                {
                    eCodecStatus = ((CodecWebSocketJson*)codec_iter->second)->Encode(oHttpMsg, pConn->pWaitForSendBuff);
                }
                else
                {
                    eCodecStatus = ((CodecWebSocketJson*)codec_iter->second)->Encode(oHttpMsg, pConn->pSendBuff);
                }
            }
            else if (util::CODEC_HTTP == conn_iter->second->eCodecType)
            {
                if (conn_iter->second->pWaitForSendBuff->ReadableBytes() > 0)   // 正在连接
                {
                    eCodecStatus = ((HttpCodec*)codec_iter->second)->Encode(oHttpMsg, pConn->pWaitForSendBuff);
                }
                else
                {
                    eCodecStatus = ((HttpCodec*)codec_iter->second)->Encode(oHttpMsg, pConn->pSendBuff);
                }
            }
            else
            {
                LOG4_ERROR("the codec for fd %d is not http or websocket codec(%d)!",
                                                stMsgShell.iFd,pConn->eCodecType);
                return(false);
            }

            if (CODEC_STATUS_OK == eCodecStatus && pConn->pSendBuff->ReadableBytes() > 0)
            {
                ++m_iSendNum;
                if ((conn_iter->second->pIoWatcher != NULL) && (pConn->pIoWatcher->events & EV_WRITE))
                {   // 正在监听fd的写事件，说明发送缓冲区满，此时直接返回，等待EV_WRITE事件再执行WriteFD
                    return(true);
                }
                LOG4_TRACE("fd[%d], seq[%u], pConn->pSendBuff 0x%x", stMsgShell.iFd, stMsgShell.ulSeq, pConn->pSendBuff);
                int iErrno = 0;
                int iNeedWriteLen = (int)pConn->pSendBuff->ReadableBytes();
                int iWriteLen = pConn->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
                pConn->pSendBuff->Compact(8192);
                if (iWriteLen < 0)
                {
                    if (EAGAIN != iErrno && EINTR != iErrno)  // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                    {
                        LOG4_ERROR("send to fd %d error %d: %s",
                                        stMsgShell.iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        DestroyConnect(conn_iter);
                        return(false);
                    }
                    else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                    {
                        LOG4_TRACE("write len %d, error %d: %s",
                                        iWriteLen, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        conn_iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(pConn);
                    }
                    else
                    {
                        LOG4_TRACE("write len %d, error %d: %s",
                                        iWriteLen, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        conn_iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(pConn);
                    }
                }
                else if (iWriteLen > 0)
                {
                    if (pHttpStep != NULL)
                    {
                        std::unordered_map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
                        if (http_step_iter == m_mapHttpAttr.end())
                        {
                            std::list<uint32> listHttpStepSeq;
                            listHttpStepSeq.push_back(pHttpStep->GetSequence());
                            m_mapHttpAttr.insert(std::pair<int32, std::list<uint32> >(stMsgShell.iFd, listHttpStepSeq));
                        }
                        else
                        {
                            http_step_iter->second.push_back(pHttpStep->GetSequence());
                        }
                    }
                    else
                    {
                        std::unordered_map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
                        if (http_step_iter == m_mapHttpAttr.end())
                        {
                            std::list<uint32> listHttpStepSeq;
                            m_mapHttpAttr.insert(std::pair<int32, std::list<uint32> >(stMsgShell.iFd, listHttpStepSeq));
                        }
                    }
                    m_iSendByte += iWriteLen;
                    conn_iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        LOG4_TRACE("need write len %d, and had writen len %d", iNeedWriteLen, iWriteLen);
                        RemoveIoWriteEvent(pConn);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        AddIoWriteEvent(pConn);
                    }
                }
                return(true);
            }
            else
            {
                return(false);
            }
        }
        else
        {
            LOG4_ERROR("fd %d sequence %lu not match the ifd %d sequence %lu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second->iFd,conn_iter->second->ulSeq);
            return(false);
        }
    }
}

bool Worker::SentTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, HttpStep* pHttpStep)
{
    char szIdentify[256] = {0};
    snprintf(szIdentify, sizeof(szIdentify), "%s:%d%s", strHost.c_str(), iPort, strUrlPath.c_str());
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, szIdentify);
    return(AutoSend(strHost, iPort, strUrlPath, oHttpMsg, pHttpStep));
    // 向外部发起http请求不复用连接
//    std::unordered_map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(szIdentify);
//    if (shell_iter == m_mapMsgShell.end())
//    {
//        LOG4_TRACE("no tagMsgShell match %s.", szIdentify);
//        return(AutoSend(strHost, iPort, strUrlPath, oHttpMsg, pHttpStep));
//    }
//    else
//    {
//        return(SendTo(shell_iter->second, oHttpMsg, pHttpStep));
//    }
}

bool Worker::SetConnectIdentify(const tagMsgShell& stMsgShell, const std::string& strIdentify)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (stMsgShell.iFd == 0 || strIdentify.size() == 0)
    {
        LOG4_WARN("%s() stMsgShell.iFd(%u) == 0 || strIdentify.size(%u) == 0",
                        __FUNCTION__,stMsgShell.iFd,strIdentify.size());
        return(false);
    }
    std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("%s() no fd %d found in m_mapFdAttr",__FUNCTION__, stMsgShell.iFd);
        return(false);
    }
    else
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
        	iter->second->strIdentify = strIdentify;
        	return(true);
        }
        else
        {
            LOG4_ERROR("%s() fd %d sequence %lu not match the sequence %lu in m_mapFdAttr",
                            __FUNCTION__,stMsgShell.iFd, stMsgShell.ulSeq, iter->second->ulSeq);
            return(false);
        }
    }
}

bool Worker::AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, strIdentify.c_str());
    int iPosIpPortSeparator = strIdentify.find(':');
    if (iPosIpPortSeparator == std::string::npos)
    {
        return(false);
    }
    int iPosPortWorkerIndexSeparator = strIdentify.rfind('.');
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, iPosPortWorkerIndexSeparator - (iPosIpPortSeparator + 1));
    std::string strWorkerIndex = strIdentify.substr(iPosPortWorkerIndexSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    if (iPort == 0)
    {
        return(false);
    }
    int iWorkerIndex = atoi(strWorkerIndex.c_str());
    if (iWorkerIndex > 200)
    {
        return(false);
    }
    struct sockaddr_in stAddr;
    int iFd = -1;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    bzero(&(stAddr.sin_zero), 8);
    iFd = socket(AF_INET, SOCK_STREAM, 0);
    if (iFd == -1)
    {
        return(false);
    }
    {
		x_sock_set_block(iFd, 0);
		int reuse = 1;
		int iTcpNoDelay = 1;
		if (::setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
		{
			LOG4_WARN("fail to set SO_REUSEADDR");
		}
#ifndef ENABLE_NAGLE
		if (setsockopt(iFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
		{
			LOG4_WARN("fail to set TCP_NODELAY");
		}
#endif
	}
    uint32 ulSeq = GetSequence();
    if (CreateFdAttr(iFd, ulSeq))
    {
        std::unordered_map<int, tagConnectionAttr*>::iterator conn_iter =  m_mapFdAttr.find(iFd);
        snprintf(conn_iter->second->pRemoteAddr, 32, strIdentify.c_str());
        if(AddIoTimeout(iFd, ulSeq, conn_iter->second, 1.5))
        {
        	tagConnectionAttr* pConn = conn_iter->second;
        	pConn->ucConnectStatus = 0;
            if (!AddIoReadEvent(conn_iter->second))
            {
                LOG4_TRACE("if (!AddIoReadEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
            if (!AddIoWriteEvent(pConn))
            {
                LOG4_TRACE("if (!AddIoWriteEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
            auto codec_iter = m_mapCodec.find(pConn->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", conn_iter->second->eCodecType);
                DestroyConnect(conn_iter);
                return(false);
            }
            E_CODEC_STATUS eCodecStatus = codec_iter->second->Encode(oMsgHead, oMsgBody, pConn->pWaitForSendBuff);
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                ++m_iSendNum;
            }
            else
            {
                return(false);
            }
//            LOG4_TRACE("fd %d, iter->second->pWaitForSendBuff->ReadableBytes()=%d",
//                            iFd, iter->second->pWaitForSendBuff->ReadableBytes());
            m_mapSeq2WorkerIndex.insert(std::pair<uint32, int>(ulSeq, iWorkerIndex));
            tagMsgShell stMsgShell;
            stMsgShell.iFd = iFd;
            stMsgShell.ulSeq = ulSeq;
            AddMsgShell(strIdentify, stMsgShell);
            connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
            return(true);
        }
        else
        {
            LOG4_TRACE("if(AddIoTimeout(iFd, ulSeq, conn_iter->second, 1.5)) else");
            DestroyConnect(conn_iter);
            return(false);
        }
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

bool Worker::AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, HttpStep* pHttpStep)
{
    LOG4_TRACE("%s(%s, %d, %s)", __FUNCTION__, strHost.c_str(), iPort, strUrlPath.c_str());
    struct sockaddr_in stAddr;
    if (!Host2Addr(strHost,iPort,stAddr))
    {
        LOG4_WARN("Host2Addr failed strHost(%s) iPort(%d)",strHost.c_str(),iPort);
        return(false);
    }
    tagMsgShell stMsgShell;
    stMsgShell.iFd = socket(AF_INET, SOCK_STREAM, 0);
    if (stMsgShell.iFd == -1)
    {
        return(false);
    }
    {
		x_sock_set_block(stMsgShell.iFd, 0);
		int reuse = 1;
		int iTcpNoDelay = 1;
		if (::setsockopt(stMsgShell.iFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
		{
			LOG4_WARN("fail to set SO_REUSEADDR");
		}
#ifndef ENABLE_NAGLE
		if (setsockopt(stMsgShell.iFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
		{
			LOG4_WARN("fail to set TCP_NODELAY");
		}
#endif
	}
    stMsgShell.ulSeq = GetSequence();
    tagConnectionAttr* pConnAttr = CreateFdAttr(stMsgShell.iFd, stMsgShell.ulSeq);
    if (pConnAttr)
    {
        pConnAttr->eCodecType = util::CODEC_HTTP;
        snprintf(pConnAttr->pRemoteAddr, 32, strHost.c_str());
        std::unordered_map<int, tagConnectionAttr*>::iterator conn_iter =  m_mapFdAttr.find(stMsgShell.iFd);
        if(AddIoTimeout(stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second, 2.5))
        {
            conn_iter->second->dKeepAlive = 10;
            LOG4_TRACE("set dKeepAlive(%lf)",conn_iter->second->dKeepAlive);
            if (!AddIoReadEvent(conn_iter->second))
            {
                LOG4_TRACE("if (!AddIoReadEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
            if (!AddIoWriteEvent(conn_iter->second))
            {
                LOG4_TRACE("if (!AddIoWriteEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
            auto codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", conn_iter->second->eCodecType);
                DestroyConnect(conn_iter);
                return(false);
            }
            E_CODEC_STATUS eCodecStatus = ((HttpCodec*)codec_iter->second)->Encode(oHttpMsg, conn_iter->second->pWaitForSendBuff);
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                ++m_iSendNum;
            }
            else
            {
                return(false);
            }
//            LOG4_TRACE("fd %d, iter->second->pWaitForSendBuff->ReadableBytes()=%d",
//                            iFd, iter->second->pWaitForSendBuff->ReadableBytes());
            connect(stMsgShell.iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
            if(pHttpStep)
            {
                std::unordered_map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
                if (http_step_iter == m_mapHttpAttr.end())
                {
                    std::list<uint32> listHttpStepSeq;
                    listHttpStepSeq.push_back(pHttpStep->GetSequence());
                    m_mapHttpAttr.insert(std::pair<int32, std::list<uint32> >(stMsgShell.iFd, listHttpStepSeq));
                }
                else
                {
                    http_step_iter->second.push_back(pHttpStep->GetSequence());
                }
            }
            else
            {
                std::unordered_map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
                if (http_step_iter == m_mapHttpAttr.end())
                {
                    std::list<uint32> listHttpStepSeq;
                    m_mapHttpAttr.insert(std::pair<int32, std::list<uint32> >(stMsgShell.iFd, listHttpStepSeq));
                }
            }
            return(true);
            // 向外部发起http请求不复用连接
//            char szIdentify[256] = {0};
//            snprintf(szIdentify, sizeof(szIdentify), "%s:%d%s", strHost.c_str(), iPort, strUrlPath.c_str());
//            return(AddMsgShell(szIdentify, stMsgShell));
        }
        else
        {
            LOG4_TRACE("if(AddIoTimeout(stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second, 2.5)) else");
            DestroyConnect(conn_iter);
            return(false);
        }
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(stMsgShell.iFd);
        return(false);
    }
}

bool Worker::AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s() redisAsyncConnect(%s, %d)", __FUNCTION__, strHost.c_str(), iPort);
    redisAsyncContext *c = redisAsyncConnect(strHost.c_str(), iPort);
    if (c->err)
    {
        /* Let *c leak for now... */
        LOG4_ERROR("error: %s", c->errstr);
        return(false);
    }
    c->userData = this;
    tagRedisAttr* pRedisAttr = new tagRedisAttr();
    pRedisAttr->ulSeq = GetSequence();
    pRedisAttr->listWaitData.push_back(pRedisStep);
    pRedisStep->SetRegistered();
    m_mapRedisAttr.insert(std::pair<redisAsyncContext*, tagRedisAttr*>(c, pRedisAttr));
//    LOG4_TRACE("redisLibevAttach(0x%X, 0x%X)", m_loop, c);
    redisLibevAttach(m_loop, c);
//    LOG4_TRACE("redisAsyncSetConnectCallback(0x%X, 0x%X)", c, RedisConnectCallback);
    redisAsyncSetConnectCallback(c, RedisConnectCallback);
//    LOG4_TRACE("redisAsyncSetDisconnectCallback(0x%X, 0x%X)", c, RedisDisconnectCallback);
    redisAsyncSetDisconnectCallback(c, RedisDisconnectCallback);
//    LOG4_TRACE("RedisStep::AddRedisContextAddr(%s, %d, 0x%X)", strHost.c_str(), iPort, c);
    AddRedisContextAddr(strHost, iPort, c);
    return(true);
}

bool Worker::AutoRedisCluster(const std::string& sAddrList, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, sAddrList.c_str());
    //sAddrList "192.168.18.78:6000,192.168.18.78:6001,192.168.18.78:6002,192.168.18.78:6003,192.168.18.78:6004,192.168.18.78:6005"
    if (sAddrList.size() == 0 || NULL == pRedisStep)
    {
        return false;
    }
    redisClusterAsyncContext *acc(NULL);
    std::unordered_map<std::string,redisClusterAsyncContext*>::iterator it = m_mapRedisClusterContext.find(sAddrList);
    if (it == m_mapRedisClusterContext.end())
    {
        acc = redisClusterAsyncConnect(sAddrList.c_str(),HIRCLUSTER_FLAG_NULL);
        if (acc->err)
        {
            LOG4_ERROR("error: %s", acc->errstr);
            return(false);
        }
        LOG4_TRACE("%s new redisClusterAsyncConnect(%s,%d)", __FUNCTION__,acc->cc->ip,acc->cc->port);
        m_mapRedisClusterContext.insert(std::make_pair(sAddrList,acc));
        m_mapRedisClusterContextIdentify.insert(std::make_pair(acc,sAddrList));
        redisClusterLibevAttach(acc,m_loop);
        redisClusterAsyncSetConnectCallback(acc,RedisClusterConnectCallback);//void connectCallback(const redisAsyncContext *c, int status)
        redisClusterAsyncSetDisconnectCallback(acc,RedisClusterDisconnectCallback);//void disconnectCallback(const redisAsyncContext *c, int status)
    }
    else
    {
        acc = it->second;
        LOG4_TRACE("%s use redisClusterAsyncConnect(%s,%d)", __FUNCTION__,acc->cc->ip,acc->cc->port);
    }
    {//format cmd ,直接发送，在客户端api内已有发送缓存
        size_t args_size = pRedisStep->GetRedisCmd()->GetCmdArguments().size() + 1;
        const char* argv[args_size];
        size_t arglen[args_size];
        argv[0] = pRedisStep->GetRedisCmd()->GetCmd().c_str();
        arglen[0] = pRedisStep->GetRedisCmd()->GetCmd().size();
        std::vector<std::pair<std::string, bool> >::const_iterator c_iter = pRedisStep->GetRedisCmd()->GetCmdArguments().begin();
        for (size_t i = 1; c_iter != pRedisStep->GetRedisCmd()->GetCmdArguments().end(); ++c_iter, ++i)
        {
            argv[i] = c_iter->first.c_str();
            arglen[i] = c_iter->first.size();
        }
        //redisAsyncContext->userData 为传入的privdata参数,RedisClusterCmdCallback 的privdata也是传入的privdata参数
        int iCmdStatus = redisClusterAsyncCommandArgv(acc, RedisClusterCmdCallback, this, args_size, argv, arglen);
        if (iCmdStatus == REDIS_OK)
        {
            LOG4_DEBUG("succeed in sending redis cmd: %s",pRedisStep->GetRedisCmd()->ToString().c_str());
            tagRedisAttr* ptagRedisAttr(NULL);
            std::unordered_map<redisClusterAsyncContext*, tagRedisAttr*>::iterator acc_iter = m_mapRedisClusterAttr.find(acc);
            if (acc_iter == m_mapRedisClusterAttr.end())
            {
                ptagRedisAttr = new tagRedisAttr();
                ptagRedisAttr->bIsReady = true;
                m_mapRedisClusterAttr.insert(std::make_pair(acc,ptagRedisAttr));
            }
            else
            {
                ptagRedisAttr = acc_iter->second;
            }
            ptagRedisAttr->listData.push_back(pRedisStep);
        }
        else    // 命令执行失败，不再继续执行，等待下一次回调
        {
            LOG4_WARN("failed in sending redis cmd: %s,err:%d,errstr:%s", pRedisStep->GetRedisCmd()->ToString().c_str(),acc->err,acc->errstr);
            redisAsyncContext cobj;//对逻辑层只是抛出错误信息，不直接使用连接对象
            cobj.err = acc->err;
            cobj.errstr = acc->errstr;
            redisAsyncContext *c = &cobj;
            if (STATUS_CMD_RUNNING != pRedisStep->Callback(c, acc->err, NULL))
            {
                delete pRedisStep;
                pRedisStep = NULL;
            }
        }
    }
    return true;
}

bool Worker::AutoMysqlCmd(MysqlStep* pMysqlStep)
{
	LOG4_TRACE("%s() AutoMysqlCmd(%s, %d)", __FUNCTION__, pMysqlStep->m_strHost.c_str(), pMysqlStep->m_iPort);
	struct ev_loop *loop = m_loop;
	util::MysqlAsyncConn * pConn = util::mysqlAsyncConnect(pMysqlStep->m_strHost.c_str(),
			pMysqlStep->m_iPort,pMysqlStep->m_strUser.c_str(),pMysqlStep->m_strPasswd.c_str(),
			pMysqlStep->m_strDbName.c_str(),pMysqlStep->m_dbcharacterset.c_str(),loop);
	pMysqlStep->SetRegistered();
	AddMysqlContextAddr(pMysqlStep,pConn);
	return RegisterCallback(pConn, pMysqlStep);
}

bool Worker::AutoConnect(const std::string& strIdentify)
{
    LOG4_DEBUG("%s(%s)", __FUNCTION__, strIdentify.c_str());
    int iPosIpPortSeparator = strIdentify.find(':');
    if (iPosIpPortSeparator == std::string::npos)
    {
        return(false);
    }
    int iPosPortWorkerIndexSeparator = strIdentify.rfind('.');
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, iPosPortWorkerIndexSeparator - (iPosIpPortSeparator + 1));
    std::string strWorkerIndex = strIdentify.substr(iPosPortWorkerIndexSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    if (iPort == 0)
    {
        return(false);
    }
    int iWorkerIndex = atoi(strWorkerIndex.c_str());
    if (iWorkerIndex > 200)
    {
        return(false);
    }
    struct sockaddr_in stAddr;
    int iFd = -1;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    bzero(&(stAddr.sin_zero), 8);
    iFd = socket(AF_INET, SOCK_STREAM, 0);
    if (iFd == -1)
    {
        return(false);
    }
    {
		x_sock_set_block(iFd, 0);
		int reuse = 1;
		int iTcpNoDelay = 1;
		if (::setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
		{
			LOG4_WARN("fail to set SO_REUSEADDR");
		}
#ifndef ENABLE_NAGLE
		if (setsockopt(iFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
		{
			LOG4_WARN("fail to set TCP_NODELAY");
		}
#endif
	}
    uint32 ulSeq = GetSequence();
    if (CreateFdAttr(iFd, ulSeq))
    {
        std::unordered_map<int, tagConnectionAttr*>::iterator conn_iter =  m_mapFdAttr.find(iFd);
        if(AddIoTimeout(iFd, ulSeq, conn_iter->second, 1.5))
        {
        	tagConnectionAttr* pConn = conn_iter->second;
        	pConn->ucConnectStatus = 0;
            if (!AddIoReadEvent(pConn))
            {
                LOG4_ERROR("if (!AddIoReadEvent(pConn))");
                DestroyConnect(conn_iter);
                return(false);
            }
//            if (!AddIoErrorEvent(iFd))
//            {
//                DestroyConnect(iter);
//                return(false);
//            }
            if (!AddIoWriteEvent(pConn))
            {
            	LOG4_ERROR("if (!AddIoWriteEvent(pConn))");
                DestroyConnect(conn_iter);
                return(false);
            }
            m_mapSeq2WorkerIndex.insert(std::pair<uint32, int>(ulSeq, iWorkerIndex));
            tagMsgShell stMsgShell;
            stMsgShell.iFd = iFd;
            stMsgShell.ulSeq = ulSeq;
            AddMsgShell(strIdentify, stMsgShell);
            connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
            return(true);
        }
        else
        {
            LOG4_TRACE("if(AddIoTimeout(iFd, ulSeq, conn_iter->second, 1.5)) else");
            DestroyConnect(conn_iter);
            return(false);
        }
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

bool Worker::Host2Addr(const std::string & strHost,int iPort,struct sockaddr_in &stAddr,bool boRefresh)
{
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    if (stAddr.sin_addr.s_addr == 4294967295 || stAddr.sin_addr.s_addr == 0)
    {
        if (boRefresh)
        {
            struct hostent *he = gethostbyname(strHost.c_str());
            if (he != NULL)
            {
                stAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)(he->h_addr)));
                tagSockaddr sockaddr;
                sockaddr.sockaddr = stAddr.sin_addr.s_addr;
                sockaddr.uiLastTime = GetNowTime();
                std::unordered_map<std::string,tagSockaddr>::iterator iter = m_mapHosts.find(strHost);
                if (m_mapHosts.end() == iter)
                {
                    m_mapHosts.insert(std::make_pair(strHost,sockaddr));
                }
                else
                {
                    iter->second.sockaddr = stAddr.sin_addr.s_addr;
                    iter->second.uiLastTime = GetNowTime();
                }
            }
            else
            {
                LOG4_ERROR("gethostbyname(%s) error!", strHost.c_str());
                return(false);
            }
        }
        else
        {
            std::unordered_map<std::string,tagSockaddr>::iterator iter = m_mapHosts.find(strHost);
            if (iter == m_mapHosts.end())
            {
                struct hostent *he = gethostbyname(strHost.c_str());
                if (he != NULL)
                {
                    stAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)(he->h_addr)));
                    tagSockaddr sockaddr;
                    sockaddr.sockaddr = stAddr.sin_addr.s_addr;
                    sockaddr.uiLastTime = GetNowTime();
                    m_mapHosts.insert(std::make_pair(strHost,sockaddr));
                }
                else
                {
                    LOG4_ERROR("gethostbyname(%s) error!", strHost.c_str());
                    return(false);
                }
            }
            else if (iter->second.uiLastTime + 120 <= GetNowTime())
            {
                struct hostent *he = gethostbyname(strHost.c_str());
                if (he != NULL)
                {
                    stAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)(he->h_addr)));
                    iter->second.sockaddr = stAddr.sin_addr.s_addr;
                    iter->second.uiLastTime = GetNowTime();
                }
                else
                {
                    LOG4_ERROR("gethostbyname(%s) error!", strHost.c_str());
                    return(false);
                }
            }
            else
            {
                stAddr.sin_addr.s_addr = iter->second.sockaddr;
            }
        }
    }
    bzero(&(stAddr.sin_zero), 8);
    return true;
}
bool Worker::HttpsGet(const std::string & strUrl, std::string & strResponse,
        const std::string& strUserpwd,util::CurlClient::eContentType eType, const std::string& strCaPath,int iPort)
{
    util::CurlClient curlClient;
    CURLcode res = curlClient.GetHttps(strUrl,strResponse,strUserpwd,eType,strCaPath,iPort);
    if (CURLE_OK != res)
    {
        LOG4_WARN("%s() CURLcode(%d,%s) strUrl:%s",__FUNCTION__,res,curl_easy_strerror(res),strUrl.c_str());
        return false;
    }
    return true;
}
bool Worker::HttpsPost(const std::string & strUrl, const std::string & strFields,
        std::string & strResponse,const std::string& strUserpwd,util::CurlClient::eContentType eType,const std::string& strCaPath,int iPort)
{
	util::CurlClient curlClient;
    CURLcode res = curlClient.PostHttps(strUrl,strFields,strResponse,strUserpwd,eType,strCaPath,iPort);
    if (CURLE_OK != res)
    {
        LOG4_WARN("%s() CURLcode(%d,%s) strUrl:%s",__FUNCTION__,res,curl_easy_strerror(res),strUrl.c_str());
        return false;
    }
    return true;
}
void Worker::AddInnerFd(const tagMsgShell& stMsgShell)
{
    auto iter = m_mapInnerFd.find(stMsgShell.iFd);
    if (iter == m_mapInnerFd.end())
    {
        m_mapInnerFd.insert(std::pair<int, uint32>(stMsgShell.iFd, stMsgShell.ulSeq));
    }
    else
    {
        iter->second = stMsgShell.ulSeq;
    }
    LOG4_TRACE("%s() now m_mapInnerFd.size() = %u", __FUNCTION__, m_mapInnerFd.size());
}

bool Worker::GetMsgShell(const std::string& strIdentify, tagMsgShell& stMsgShell)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::unordered_map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        LOG4_DEBUG("no tagMsgShell match %s.", strIdentify.c_str());
        return(false);
    }
    else
    {
        stMsgShell = shell_iter->second;
        return(true);
    }
}

bool Worker::SetClientData(const tagMsgShell& stMsgShell, util::CBuffer* pBuff)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
            iter->second->pClientData->Write(pBuff, pBuff->ReadableBytes());
            return(true);
        }
        else
        {
            return(false);
        }
    }
}

bool Worker::HadClientData(const tagMsgShell& stMsgShell)
{
    std::unordered_map<int, tagConnectionAttr*>::iterator conn_iter;
    conn_iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (conn_iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
        if (stMsgShell.ulSeq != conn_iter->second->ulSeq)
        {
            return(false);
        }
        if (conn_iter->second->pClientData != NULL)
        {
            if (conn_iter->second->pClientData->ReadableBytes() > 0)
            {
                return(true);
            }
            else
            {
                return(false);
            }
        }
        else
        {
            return(false);
        }
    }
}

bool Worker::GetClientData(const tagMsgShell& stMsgShell, util::CBuffer* pBuff)
{
	LOG4_TRACE("%s()", __FUNCTION__);
	std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
	if (iter == m_mapFdAttr.end())
	{
		return(false);
	}
	else
	{
		if (iter->second->ulSeq == stMsgShell.ulSeq)
		{
			pBuff->Write(iter->second->pClientData->GetRawReadBuffer(), iter->second->pClientData->ReadableBytes());
			return(true);
		}
		else
		{
			return(false);
		}
	}

	return(false);
}


std::string Worker::GetClientAddr(const tagMsgShell& stMsgShell)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        return("");
    }
    else
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
			return(iter->second->pRemoteAddr);
        }
        else
        {
            return("");
        }
    }
}

std::string Worker::GetConnectIdentify(const tagMsgShell& stMsgShell)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        return("");
    }
    else
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
            return iter->second->strIdentify;
        }
        else
        {
            return("");
        }
    }
}

bool Worker::AbandonConnect(const std::string& strIdentify)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::unordered_map<std::string, tagMsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        LOG4_DEBUG("no tagMsgShell match %s.", strIdentify.c_str());
        return(false);
    }
    else
    {
        std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(shell_iter->second.iFd);
        if (iter == m_mapFdAttr.end())
        {
            return(false);
        }
        else
        {
            if (iter->second->ulSeq == shell_iter->second.ulSeq)
            {
                iter->second->strIdentify = "";
                iter->second->pClientData->Clear();
                m_mapMsgShell.erase(shell_iter);
                return(true);
            }
            else
            {
                return(false);
            }
        }
        return(true);
    }
}

bool Worker::ExecStep(uint32 uiCallerStepSeq, uint32 uiCalledStepSeq,int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
    LOG4_TRACE("%s(caller[%u], called[%u])", __FUNCTION__, uiCallerStepSeq, uiCalledStepSeq);
    std::unordered_map<uint32, Step*>::iterator step_iter = m_mapCallbackStep.find(uiCalledStepSeq);
    if (step_iter == m_mapCallbackStep.end())
    {
        LOG4_WARN("step %u is not in the callback list.", uiCalledStepSeq);
    }
    else
    {
    	int nRet = step_iter->second->Emit(iErrno, strErrMsg, strErrShow);
        if (net::STATUS_CMD_RUNNING != nRet)
        {
            DeleteCallback(uiCallerStepSeq, step_iter->second);
            step_iter = m_mapCallbackStep.find(uiCallerStepSeq);// 处理调用者step的NextStep
            if (step_iter != m_mapCallbackStep.end())
            {
                if (step_iter->second->m_pNextStep != NULL && step_iter->second->m_pNextStep->GetSequence() == uiCalledStepSeq)
                {
                    step_iter->second->m_pNextStep = NULL;
                }
            }
        }
        if (nRet != net::STATUS_CMD_FAULT)return true;
    }
    return false;
}

bool Worker::ExecStep(uint32 uiStepSeq,int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
    LOG4_TRACE("%s(uiStepSeq[%u])", __FUNCTION__,uiStepSeq);
    std::unordered_map<uint32, Step*>::iterator step_iter = m_mapCallbackStep.find(uiStepSeq);
    if (step_iter == m_mapCallbackStep.end())
    {
        LOG4_WARN("step %u is not in the callback list.", uiStepSeq);
    }
    else
    {
    	int nRet = step_iter->second->Emit(iErrno, strErrMsg, strErrShow);
        if (net::STATUS_CMD_RUNNING != nRet)
        {
            DeleteCallback(step_iter->second);
        }
        if (nRet != net::STATUS_CMD_FAULT)return true;
    }
    return false;
}

bool Worker::ExecStep(Step* pStep,int iErrno, const std::string& strErrMsg, const std::string& strErrShow,ev_tstamp dTimeout)
{
	if (!pStep)
	{
		LOG4_ERROR("%s() null pStep",__FUNCTION__);
		return false;
	}
	if (!pStep->IsRegistered())
	{
		if (!RegisterCallback(pStep,dTimeout))
		{
			LOG4_ERROR("%s() RegisterCallback error",__FUNCTION__);
			SAFE_DELETE(pStep);
			return(false);
		}
		LOG4_TRACE("%s(RegisterCallback[%u])", __FUNCTION__,pStep->GetSequence());
	}
    LOG4_TRACE("%s(uiStepSeq[%u])", __FUNCTION__,pStep->GetSequence());
    std::unordered_map<uint32, Step*>::iterator step_iter = m_mapCallbackStep.find(pStep->GetSequence());
    if (step_iter == m_mapCallbackStep.end())
    {
        LOG4_WARN("step %u is not in the callback list.", pStep->GetSequence());
    }
    else
    {
    	int nRet = step_iter->second->Emit(iErrno, strErrMsg, strErrShow);
        if (net::STATUS_CMD_RUNNING != nRet)DeleteCallback(step_iter->second);
        if (net::STATUS_CMD_FAULT != nRet)return true;
        LOG4_ERROR("step Emit failed.");
    }
    return false;
}

bool Worker::ExecStep(RedisStep* pRedisStep)
{
	if (NULL == pRedisStep)
	{
		LOG4_ERROR("NULL == pRedisStep");
		return(false);
	}
	LOG4_TRACE("%s()",__FUNCTION__);
	if (net::STATUS_CMD_RUNNING == pRedisStep->Emit(ERR_OK))//RedisStep注册在其Emit内实现
	{
		return(true);
	}
	delete pRedisStep;
	pRedisStep = NULL;
	return false;
}

Step* Worker::GetStep(uint32 uiStepSeq)
{
	auto iter = m_mapCallbackStep.find(uiStepSeq);
	if (iter == m_mapCallbackStep.end())
	{
		return NULL;
	}
	return iter->second;
}

void Worker::LoadSo(util::CJsonObject& oSoConf,bool boForce)
{
    LOG4_TRACE("%s():oSoConf(%s)", __FUNCTION__,oSoConf.ToString().c_str());
    int iCmd = 0;
    int iVersion = 0;
    bool bIsload = false;
    std::string strSoPath;
    std::unordered_map<int, tagSo*>::iterator cmd_iter;
    tagSo* pSo = NULL;
    for (int i = 0; i < oSoConf.GetArraySize(); ++i)
    {
        oSoConf[i].Get("load", bIsload);
        if (bIsload)
        {
            strSoPath = m_strWorkPath + std::string("/") + oSoConf[i]("so_path");
            if (oSoConf[i].Get("cmd", iCmd) && oSoConf[i].Get("version", iVersion))
            {
                if (0 != access(strSoPath.c_str(), F_OK))
                {
                    LOG4_WARN("%s not exist!", strSoPath.c_str());
                    continue;
                }
                cmd_iter = m_mapSo.find(iCmd);
                if (cmd_iter == m_mapSo.end())
                {
                    LOG4_INFO("try to load:%s", strSoPath.c_str());
                    LoadSoAndGetCmd(iCmd, strSoPath, oSoConf[i]("entrance_symbol"), iVersion);
                }
                else
                {
                    if (iVersion != cmd_iter->second->iVersion || boForce)
                    {
                        LoadSoAndGetCmd(iCmd, strSoPath, oSoConf[i]("entrance_symbol"), iVersion);
                    }
                    else
                    {
                        LOG4_INFO("same version(%d).no need  to load %s",cmd_iter->second->iVersion,
                                        strSoPath.c_str());
                    }
                }
            }
            else
            {
                LOG4_WARN("cmd or version not found.failed to load %s", strSoPath.c_str());
            }
        }
        else        // 卸载动态库
        {
            strSoPath = m_strWorkPath + std::string("/") + oSoConf[i]("so_path");
            if (oSoConf[i].Get("cmd", iCmd))
            {
                LOG4_INFO("unload %s", strSoPath.c_str());
                UnloadSoAndDeleteCmd(iCmd);
            }
            else
            {
                LOG4_WARN("cmd not exist.failed to unload %s", strSoPath.c_str());
            }
        }
    }
}

void Worker::ReloadSo(util::CJsonObject& oCmds)
{
    LOG4_DEBUG("%s():oCmds(%s)", __FUNCTION__,oCmds.ToString().c_str());
    int iCmd = 0;
    int iVersion = 0;
    std::string strSoPath;
    std::string strSymbol;
    std::unordered_map<int, tagSo*>::iterator cmd_iter;
    tagSo* pSo = NULL;
    for (int i = 0; i < oCmds.GetArraySize(); ++i)
    {
        std::string cmd = oCmds[i].ToString();
//        std::string::iterator it = std::remove(cmd.begin(), cmd.end(), '\"');
//        cmd.erase(it, cmd.end());
        iCmd = atoi(cmd.c_str());
        cmd_iter = m_mapSo.find(iCmd);
        if (cmd_iter != m_mapSo.end())
        {
            strSoPath = cmd_iter->second->strSoPath;
            strSymbol = cmd_iter->second->strSymbol;
            iVersion = cmd_iter->second->iVersion;
            if (0 != access(strSoPath.c_str(), F_OK))
            {
                LOG4_WARN("%s not exist!", strSoPath.c_str());
                continue;
            }
            LoadSoAndGetCmd(iCmd, strSoPath, strSymbol, iVersion);
        }
        else
        {
            LOG4_WARN("no such cmd %s", cmd.c_str());
        }
    }
}

tagSo* Worker::LoadSoAndGetCmd(int iCmd, const std::string& strSoPath, const std::string& strSymbol, int iVersion)
{
    LOG4_TRACE("%s() iCmd:%d", __FUNCTION__,iCmd);
    UnloadSoAndDeleteCmd(iCmd);
    tagSo* pSo = NULL;
    void* pHandle = NULL;
    pHandle = dlopen(strSoPath.c_str(),RTLD_NOW|RTLD_NODELETE);
    char* dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("cannot load dynamic lib %s!" , dlsym_error);
        if (pHandle != NULL)
        {
            dlclose(pHandle);
        }
        return(pSo);
    }
    CreateCmd* pCreateCmd = (CreateCmd*)dlsym(pHandle, strSymbol.c_str());
    dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("dlsym error %s!" , dlsym_error);
        dlclose(pHandle);
        return(pSo);
    }
    Cmd* pCmd = pCreateCmd();
    LOG4_TRACE("%s() strSoPath:%s pHandle:%p pCreateCmd:%p pCmd:%p",__FUNCTION__,strSoPath.c_str(),pHandle,pCreateCmd,pCmd);
    if (pCmd != NULL)
    {
        pSo = new tagSo();
        if (pSo != NULL)
        {
            pSo->pSoHandle = pHandle;
            pSo->pCmd = pCmd;
            pSo->strSoPath = strSoPath;
            pSo->strSymbol = strSymbol;
            pSo->iVersion = iVersion;
            pSo->pCmd->SetCmd(iCmd);
            if (!pSo->pCmd->Init())
            {
                LOG4_FATAL("Cmd %d %s init error",iCmd, strSoPath.c_str());
                delete pSo;
                return NULL;
            }
            LOG4_INFO("succeed in loading(%s) strLoadTime(%s) pHandle:%p pCreateCmd:%p pCmd:%p",
                    strSoPath.c_str(),pSo->strLoadTime.c_str(),pHandle,pCreateCmd,pCmd);
            m_mapSo.insert(std::make_pair(iCmd,pSo));
            return pSo;
        }
        else
        {
            LOG4_FATAL("new tagSo() error for %s!",strSoPath.c_str());
            delete pCmd;
            dlclose(pHandle);
        }
    }
    return(pSo);
}

void Worker::UnloadSoAndDeleteCmd(int iCmd)
{
    LOG4_TRACE("%s() iCmd:%d", __FUNCTION__,iCmd);
    std::unordered_map<int, tagSo*>::iterator mapSoIt = m_mapSo.find(iCmd);
    if (mapSoIt != m_mapSo.end())
    {
        LOG4_INFO("succeed in unloading(%s) strLoadTime(%s),strNowTime(%s)",
                                mapSoIt->second->strSoPath.c_str(),mapSoIt->second->strLoadTime.c_str(),util::GetCurrentTime(20).c_str());
        void* pSoHandle = mapSoIt->second->pSoHandle;
        delete mapSoIt->second;
        if(pSoHandle)
        {
            LOG4_TRACE("%s() dlclose iCmd:%d", __FUNCTION__,iCmd);
            dlclose(pSoHandle);
            pSoHandle = NULL;
        }
        m_mapSo.erase(mapSoIt);
    }
}

void Worker::LoadModule(util::CJsonObject& oModuleConf,bool boForce)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::string strModulePath;
    int iVersion = 0;
    bool bIsload = false;
    std::string strSoPath;
    std::unordered_map<std::string, tagModule*>::iterator module_iter;
    tagModule* pModule = NULL;
    LOG4_TRACE("oModuleConf.GetArraySize() = %d,oModuleConf(%s)", oModuleConf.GetArraySize(),
                    oModuleConf.ToString().c_str());
    for (int i = 0; i < oModuleConf.GetArraySize(); ++i)
    {
        oModuleConf[i].Get("load", bIsload);
        if (bIsload)
        {
            if (oModuleConf[i].Get("url_path", strModulePath) && oModuleConf[i].Get("version", iVersion))
            {
                LOG4_TRACE("url_path = %s", strModulePath.c_str());
                strSoPath = m_strWorkPath + std::string("/") + oModuleConf[i]("so_path");
                if (0 != access(strSoPath.c_str(), F_OK))
                {
                    LOG4_WARN("%s not exist!", strSoPath.c_str());
                    continue;
                }
                module_iter = m_mapModule.find(strModulePath);
                if (module_iter == m_mapModule.end())
                {
                    LoadSoAndGetModule(strModulePath, strSoPath, oModuleConf[i]("entrance_symbol"), iVersion);
                }
                else
                {
                    if (iVersion != module_iter->second->iVersion || boForce)
                    {
                        LoadSoAndGetModule(strModulePath, strSoPath, oModuleConf[i]("entrance_symbol"), iVersion);
                    }
                    else
                    {
                        LOG4_INFO("already exist same version:%s", strSoPath.c_str());
                    }
                }
            }
        }
        else        // 卸载动态库
        {
            if (oModuleConf[i].Get("url_path", strModulePath))
            {
                strSoPath = m_strWorkPath + std::string("/") + oModuleConf[i]("so_path");
                UnloadSoAndDeleteModule(strModulePath);
                LOG4_INFO("unload %s", strSoPath.c_str());
            }
        }
    }
}

void Worker::ReloadModule(util::CJsonObject& oUrlPaths)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<std::string, tagModule*>::iterator module_iter;
    tagModule* pModule = NULL;
    LOG4_TRACE("oUrlPaths.GetArraySize() = %d,oUrlPaths(%s)", oUrlPaths.GetArraySize(),
                    oUrlPaths.ToString().c_str());
    std::string strSoPath;
    std::string strSymbol;
    int iVersion(0);
    for (int i = 0; i < oUrlPaths.GetArraySize(); ++i)
    {
        std::string url_path = oUrlPaths[i].ToString();
        std::string::iterator it = std::remove(url_path.begin(), url_path.end(), '\"');
        url_path.erase(it, url_path.end());
        LOG4_TRACE("url_path = %s", url_path.c_str());
        module_iter = m_mapModule.find(url_path);
        if (module_iter != m_mapModule.end())
        {
            strSoPath = module_iter->second->strSoPath;
            strSymbol = module_iter->second->strSymbol;
            iVersion = module_iter->second->iVersion;
            if (0 != access(strSoPath.c_str(), F_OK))
            {
                LOG4_WARN("%s not exist!", strSoPath.c_str());
                continue;
            }
            LoadSoAndGetModule(url_path, strSoPath, strSymbol, iVersion);
        }
        else
        {
            LOG4_WARN("no such url_path %s", url_path.c_str());
        }
    }
}

tagModule* Worker::LoadSoAndGetModule(const std::string& strModulePath, const std::string& strSoPath, const std::string& strSymbol, int iVersion)
{
    LOG4_TRACE("%s() strModulePath:%s", __FUNCTION__,strModulePath.c_str());
    UnloadSoAndDeleteModule(strModulePath);
    tagModule* pSo = NULL;
    void* pHandle = NULL;
    pHandle = dlopen(strSoPath.c_str(), RTLD_NOW|RTLD_NODELETE);
    char* dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("cannot load dynamic lib %s!" , dlsym_error);
        return(pSo);
    }
    CreateCmd* pCreateModule = (CreateCmd*)dlsym(pHandle, strSymbol.c_str());
    dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("dlsym error %s!" , dlsym_error);
        dlclose(pHandle);
        return(pSo);
    }
    Module* pModule = (Module*)pCreateModule();
    LOG4_TRACE("%s() strSoPath:%s pHandle:%p pCreateModule:%p pModule:%p",
            __FUNCTION__,strSoPath.c_str(),pHandle,pCreateModule,pModule);
    if (pModule != NULL)
    {
        pSo = new tagModule();
        if (pSo != NULL)
        {
            pSo->pSoHandle = pHandle;
            pSo->pModule = pModule;
            pSo->strSoPath = strSoPath;
            pSo->strSymbol = strSymbol;
            pSo->iVersion = iVersion;
            pSo->pModule->SetModulePath(strModulePath);
            if (!pSo->pModule->Init())
            {
                LOG4_FATAL("Module %s %s init error", strModulePath.c_str(), strSoPath.c_str());
                delete pSo;
                return NULL;
            }
            m_mapModule.insert(std::make_pair(strModulePath,pSo));
        }
        else
        {
            LOG4_FATAL("new tagSo() error for %s!",strSoPath.c_str());
            delete pModule;
            dlclose(pHandle);
        }
    }
    return(pSo);
}

void Worker::UnloadSoAndDeleteModule(const std::string& strModulePath)
{
    LOG4_TRACE("%s() strModulePath:%s", __FUNCTION__,strModulePath.c_str());
    std::unordered_map<std::string, tagModule*>::iterator mapMoIt = m_mapModule.find(strModulePath);
    if (mapMoIt != m_mapModule.end())
    {
        LOG4_INFO("succeed in unloading(%s) strLoadTime(%s),strNowTime(%s)",
                mapMoIt->second->strSoPath.c_str(),mapMoIt->second->strLoadTime.c_str(),util::GetCurrentTime(20).c_str());
        void* pSoHandle = mapMoIt->second->pSoHandle;
        delete mapMoIt->second;
        if(pSoHandle)
        {
            LOG4_TRACE("%s() dlclose strModulePath:%d", __FUNCTION__,strModulePath.c_str());
            dlclose(pSoHandle);
            pSoHandle = NULL;
        }
        m_mapModule.erase(mapMoIt);
    }
}

bool Worker::AddPeriodicTaskEvent()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_timer* timeout_watcher = new ev_timer();
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("new timeout_watcher error!");
        return(false);
    }
    ev_timer_init (timeout_watcher, PeriodicTaskCallback, NODE_BEAT + ev_time() - ev_now(m_loop), 0.);
    timeout_watcher->data = (void*)this;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

bool Worker::AddIoReadEvent(tagConnectionAttr* pConn)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
	if (NULL == pConn->pIoWatcher)
	{
		io_watcher = new ev_io();
		if (io_watcher == NULL)
		{
			LOG4_ERROR("new io_watcher error!");
			return(false);
		}
		tagIoWatcherData* pData = new tagIoWatcherData();
		if (pData == NULL)
		{
			LOG4_ERROR("new tagIoWatcherData error!");
			delete io_watcher;
			return(false);
		}
		pData->iFd = pConn->iFd;
		pData->ulSeq = pConn->ulSeq;
		pData->pWorker = this;
		ev_io_init (io_watcher, IoCallback, pData->iFd, EV_READ);
		io_watcher->data = (void*)pData;
		pConn->pIoWatcher = io_watcher;
		ev_io_start (m_loop, io_watcher);
	}
	else
	{
		io_watcher = pConn->pIoWatcher;
		ev_io_stop(m_loop, io_watcher);
		ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_READ);
		ev_io_start (m_loop, io_watcher);
	}
    return(true);
}

bool Worker::AddIoWriteEvent(tagConnectionAttr* pConn)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
	if (NULL == pConn->pIoWatcher)
	{
		io_watcher = new ev_io();
		if (io_watcher == NULL)
		{
			LOG4_ERROR("new io_watcher error!");
			return(false);
		}
		tagIoWatcherData* pData = new tagIoWatcherData();
		if (pData == NULL)
		{
			LOG4_ERROR("new tagIoWatcherData error!");
			delete io_watcher;
			return(false);
		}
		pData->iFd = pConn->iFd;
		pData->ulSeq = pConn->ulSeq;
		pData->pWorker = this;
		ev_io_init (io_watcher, IoCallback, pData->iFd, EV_WRITE);
		io_watcher->data = (void*)pData;
		pConn->pIoWatcher = io_watcher;
		ev_io_start (m_loop, io_watcher);
	}
	else
	{
		io_watcher = pConn->pIoWatcher;
		ev_io_stop(m_loop, io_watcher);
		ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_WRITE);
		ev_io_start (m_loop, io_watcher);
	}
    return(true);
}

//bool Worker::AddIoErrorEvent(int iFd)
//{
//    LOG4_TRACE("%s()", __FUNCTION__);
//    ev_io* io_watcher = NULL;
//    std::unordered_map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(iFd);
//    if (iter != m_mapFdAttr.end())
//    {
//        if (NULL == iter->second->pIoWatcher)
//        {
//            io_watcher = new ev_io();
//            if (io_watcher == NULL)
//            {
//                LOG4_ERROR("new io_watcher error!");
//                return(false);
//            }
//            tagIoWatcherData* pData = new tagIoWatcherData();
//            if (pData == NULL)
//            {
//                LOG4_ERROR("new tagIoWatcherData error!");
//                delete io_watcher;
//                return(false);
//            }
//            pData->iFd = iFd;
//            pData->ullSeq = iter->second->ullSeq;
//            pData->pWorker = this;
//            ev_io_init (io_watcher, IoCallback, iFd, EV_ERROR);
//            io_watcher->data = (void*)pData;
//            iter->second->pIoWatcher = io_watcher;
//            ev_io_start (m_loop, io_watcher);
//        }
//        else
//        {
//            io_watcher = iter->second->pIoWatcher;
//            ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_ERROR);
//        }
//    }
//    return(true);
//}

bool Worker::RemoveIoWriteEvent(tagConnectionAttr* pConn)
{
    LOG4_TRACE("%s()", __FUNCTION__);
	if (NULL != pConn->pIoWatcher)
	{
		if (pConn->pIoWatcher->events & EV_WRITE)
		{
			ev_io* io_watcher = pConn->pIoWatcher;
			ev_io_stop(m_loop, io_watcher);
			ev_io_set(io_watcher, io_watcher->fd, io_watcher->events & ~EV_WRITE);
			ev_io_start (m_loop, pConn->pIoWatcher);
		}
	}
    return(true);
}

bool Worker::DelEvents(ev_io** io_watcher_addr)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (io_watcher_addr == NULL)
    {
        return(false);
    }
    if (*io_watcher_addr == NULL)
    {
        return(false);
    }
    LOG4_TRACE("%s() ev_io_stop io_watcher:%p", __FUNCTION__,*io_watcher_addr);
    ev_io_stop (m_loop, *io_watcher_addr);
    tagIoWatcherData* pData = (tagIoWatcherData*)(*io_watcher_addr)->data;
    delete pData;
    (*io_watcher_addr)->data = NULL;
    delete (*io_watcher_addr);
    (*io_watcher_addr) = NULL;
    io_watcher_addr = NULL;
    return(true);
}

bool Worker::AddIoTimeout(int iFd, uint32 ulSeq, tagConnectionAttr* pConnAttr, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s() RemoteAddr(%s) dTimeout(%lf)", __FUNCTION__,pConnAttr->pRemoteAddr ? pConnAttr->pRemoteAddr :"",dTimeout);
    if (pConnAttr->pTimeWatcher != NULL)
    {
        LOG4_TRACE("%s() timer reset after(%lf)",__FUNCTION__,m_dIoTimeout + ev_time() - ev_now(m_loop));
        ev_timer_stop (m_loop, pConnAttr->pTimeWatcher);
        ev_timer_set (pConnAttr->pTimeWatcher, m_dIoTimeout + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, pConnAttr->pTimeWatcher);
        return(true);
    }
    else
    {
        ev_timer* timeout_watcher = new ev_timer();
        if (timeout_watcher == NULL)
        {
            LOG4_ERROR("new timeout_watcher error!");
            return(false);
        }
        tagIoWatcherData* pData = new tagIoWatcherData();
        if (pData == NULL)
        {
            LOG4_ERROR("new tagIoWatcherData error!");
            delete timeout_watcher;
            return(false);
        }
        LOG4_TRACE("%s() timer after(%lf)",__FUNCTION__,dTimeout + ev_time() - ev_now(m_loop));
        ev_timer_init (timeout_watcher, IoTimeoutCallback, dTimeout + ev_time() - ev_now(m_loop), 0.);
        pData->iFd = iFd;
        pData->ulSeq = ulSeq;
        pData->pWorker = this;
        timeout_watcher->data = (void*)pData;
        if (pConnAttr != NULL)
        {
            pConnAttr->pTimeWatcher = timeout_watcher;
        }
        ev_timer_start (m_loop, timeout_watcher);
        return(true);
    }
}

tagConnectionAttr* Worker::CreateFdAttr(int iFd, uint32 ulSeq, util::E_CODEC_TYPE eCodecType)
{
    LOG4_DEBUG("%s(fd[%d], seq[%u], codec[%d])", __FUNCTION__, iFd, ulSeq, eCodecType);
    std::unordered_map<int, tagConnectionAttr*>::iterator fd_attr_iter;
    fd_attr_iter = m_mapFdAttr.find(iFd);
    if (fd_attr_iter == m_mapFdAttr.end())
    {
        tagConnectionAttr* pConnAttr = new tagConnectionAttr();
        if (pConnAttr == NULL)
        {
            LOG4_ERROR("new pConnAttr for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->iFd = iFd;
        pConnAttr->pRecvBuff = new util::CBuffer();
        if (pConnAttr->pRecvBuff == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pRecvBuff for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->pSendBuff = new util::CBuffer();
        if (pConnAttr->pSendBuff == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pSendBuff for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->pWaitForSendBuff = new util::CBuffer();
        if (pConnAttr->pWaitForSendBuff == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pWaitForSendBuff for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->pClientData = new util::CBuffer();
        if (pConnAttr->pClientData == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pClientData for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->dActiveTime = ev_now(m_loop);
        pConnAttr->ulSeq = ulSeq;
        pConnAttr->eCodecType = eCodecType;
        m_mapFdAttr.insert(std::pair<int, tagConnectionAttr*>(iFd, pConnAttr));
        return(pConnAttr);
    }
    else
    {
        LOG4_ERROR("fd %d is exist!", iFd);
        return(NULL);
    }
}

bool Worker::DestroyConnect(std::unordered_map<int, tagConnectionAttr*>::iterator iter, bool bMsgShellNotice)
{
    if (iter == m_mapFdAttr.end())
    {
        return(false);
    }
    LOG4_DEBUG("%s disconnect, identify %s", iter->second->pRemoteAddr, iter->second->strIdentify.c_str());
    int iResult = close(iter->first);
    if (0 != iResult)
    {
        if (EINTR != errno)
        {
            LOG4_ERROR("close(%d) failed, result %d and errno %d", iter->first, iResult, errno);
            return(false);
        }
        else
        {
            LOG4_DEBUG("close(%d) failed, result %d and errno %d", iter->first, iResult, errno);
        }
    }
    tagMsgShell stMsgShell;
    stMsgShell.iFd = iter->first;
    stMsgShell.ulSeq = iter->second->ulSeq;
    std::unordered_map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(iter->first);
    if (inner_iter != m_mapInnerFd.end())
    {
        LOG4_TRACE("%s() m_mapInnerFd.size() = %u", __FUNCTION__, m_mapInnerFd.size());
        m_mapInnerFd.erase(inner_iter);
    }
    std::unordered_map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
    if (http_step_iter != m_mapHttpAttr.end())
    {
        m_mapHttpAttr.erase(http_step_iter);
    }
    DelMsgShell(iter->second->strIdentify,stMsgShell);
    if (bMsgShellNotice)
    {
        MsgShellNotice(stMsgShell, iter->second->strIdentify, iter->second->pClientData);
    }
    DelEvents(&(iter->second->pIoWatcher));
	if (iter->second->pTimeWatcher != NULL)//移除io定时器（即刻回收连接资源）
	{
		LOG4_TRACE("%s() timer ev_timer_stop",__FUNCTION__);
		ev_timer_stop (m_loop, iter->second->pTimeWatcher);
		if (iter->second->pTimeWatcher->data)
		{
			tagIoWatcherData* pData = (tagIoWatcherData*)iter->second->pTimeWatcher->data;
			delete pData;
			iter->second->pTimeWatcher->data = NULL;
		}
		delete iter->second->pTimeWatcher;
		iter->second->pTimeWatcher = NULL;
	}
    delete iter->second;
    iter->second = NULL;
    m_mapFdAttr.erase(iter);
    return(true);
}

void Worker::MsgShellNotice(const tagMsgShell& stMsgShell, const std::string& strIdentify, util::CBuffer* pClientData)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<int32, tagSo*>::iterator cmd_iter;
    cmd_iter = m_mapSo.find(CMD_REQ_DISCONNECT);
    if (cmd_iter != m_mapSo.end() && cmd_iter->second != NULL)
    {
        MsgHead oMsgHead;
        MsgBody oMsgBody;
        oMsgBody.set_body(strIdentify);
        if (pClientData != NULL && pClientData->ReadableBytes() > 0)
        {
			oMsgBody.set_additional(pClientData->GetRawReadBuffer(), pClientData->ReadableBytes());
        }
        oMsgHead.set_cmd(CMD_REQ_DISCONNECT);
        oMsgHead.set_seq(GetSequence());
        oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
        cmd_iter->second->pCmd->AnyMessage(stMsgShell, oMsgHead, oMsgBody);
    }
}
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
bool Worker::Dispose(const tagMsgShell& stMsgShell,const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,MsgHead& oOutMsgHead, MsgBody& oOutMsgBody)
{
    LOG4_DEBUG("%s(cmd %u, seq %lu)",
                    __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq());
    oOutMsgHead.Clear();
    oOutMsgBody.Clear();
    if (gc_uiCmdReq & oInMsgHead.cmd())    // 新请求
    {
    	uint32 uiCmd = gc_uiCmdBit & oInMsgHead.cmd();
        std::unordered_map<int32, Cmd*>::iterator cmd_iter;
        cmd_iter = m_mapCmd.find(uiCmd);
        if (cmd_iter != m_mapCmd.end() && cmd_iter->second != NULL)
        {
            cmd_iter->second->AnyMessage(stMsgShell, oInMsgHead, oInMsgBody);
        }
        else
        {
            std::unordered_map<int, tagSo*>::iterator cmd_so_iter;
            cmd_so_iter = m_mapSo.find(uiCmd);
            if (cmd_so_iter != m_mapSo.end() && cmd_so_iter->second != NULL)
            {
                cmd_so_iter->second->pCmd->AnyMessage(stMsgShell, oInMsgHead, oInMsgBody);
            }
            else        // 没有对应的cmd，是由接入层管理者发送的请求
            {
                if (CMD_REQ_SET_LOG_LEVEL == uiCmd)
                {
                    LogLevel oLogLevel;
                    if(!oLogLevel.ParseFromString(oInMsgBody.body()))
                    {
                        LOG4_WARN("failed to parse oLogLevel,body(%s)",oInMsgBody.body().c_str());
                    }
                    else
                    {
                        LOG4_INFO("CMD_REQ_SET_LOG_LEVEL:log level set to %d", oLogLevel.log_level());
                        m_oLogger.setLogLevel(oLogLevel.log_level());
                    }
                }
                else if (CMD_REQ_RELOAD_SO == uiCmd)
                {
                    util::CJsonObject oSoConfJson;
                    if(!oSoConfJson.Parse(oInMsgBody.body()))
                    {
                        LOG4_WARN("failed to parse oSoConfJson:(%s)",oInMsgBody.body().c_str());
                    }
                    else
                    {
                    	bool boForce(false);
                    	if (0x08000000 & oInMsgHead.cmd())
                    	{
                    		boForce = true;
                    	}
                        LOG4_INFO("CMD_REQ_RELOAD_SO:update so conf to oSoConfJson(%s) %s",
                        		oSoConfJson.ToString().c_str(),boForce?"force operation":"normal operation");
                        LoadSo(oSoConfJson,boForce);
                    }
                }
                else if (CMD_REQ_RELOAD_MODULE == uiCmd)
                {
                    util::CJsonObject oModuleConfJson;
                    if(!oModuleConfJson.Parse(oInMsgBody.body()))
                    {
                        LOG4_WARN("failed to parse oModuleConfJson:(%s)",oInMsgBody.body().c_str());
                    }
                    else
                    {
                    	bool boForce(false);
                    	if (0x08000000 & oInMsgHead.cmd())
                    	{
                    		boForce = true;
                    	}
                        LOG4_INFO("CMD_REQ_RELOAD_MODULE:update module conf to oModuleConfJson(%s) %s",
                        		oModuleConfJson.ToString().c_str(),boForce?"force operation":"normal operation");
                        LoadModule(oModuleConfJson,boForce);
                    }
                }
                else if (CMD_REQ_RELOAD_LOGIC_CONFIG == oInMsgHead.cmd())
                {
                    util::CJsonObject oConfJson;
                    if(!oConfJson.Parse(oInMsgBody.body()))
                    {
                        LOG4_WARN("failed to parse oConfJson:(%s)",oInMsgBody.body().c_str());
                    }
                    else
                    {
                        util::CJsonObject oCmds;
                        if(oConfJson.Get("cmd",oCmds))
                        {
                            LOG4_INFO("reload so conf to oCmds(%s)", oCmds.ToString().c_str());
                            ReloadSo(oCmds);
                        }
                        util::CJsonObject oUrlPaths;
                        if(oConfJson.Get("url_path",oUrlPaths))
                        {
                            LOG4_INFO("reload module conf to oUrlPaths(%s)", oUrlPaths.ToString().c_str());
                            ReloadModule(oUrlPaths);
                        }
                    }
                }
                else
                {
                    //std::unordered_map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(stMsgShell.iFd);
                    //if (inner_iter != m_mapInnerFd.end())   // 内部服务往客户端发送  if (std::string("0.0.0.0") == strFromIp)
                    snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oInMsgHead.cmd());
                    LOG4_ERROR(m_pErrBuff);
                    OrdinaryResponse oRes;
                    oRes.set_err_no(ERR_UNKNOWN_CMD);
                    oRes.set_err_msg(m_pErrBuff);
                    oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                    oOutMsgHead.set_seq(oInMsgHead.seq());
                    if(!BuildMsgBody(oOutMsgHead,oOutMsgBody,oRes))
                    {
                        LOG4_ERROR("failed to BuildMsgBody as CMD_RSP_SYS_ERROR response,seq(%u)",oInMsgHead.seq());
                    }
                }
            }
        }
    }
    else    // 回调
    {
        std::unordered_map<uint32, Step*>::iterator step_iter;
        step_iter = m_mapCallbackStep.find(oInMsgHead.seq());
        if (step_iter != m_mapCallbackStep.end())   // 步骤回调
        {
            LOG4_TRACE("receive message, cmd = %d",oInMsgHead.cmd());
            if (step_iter->second != NULL)
            {
                E_CMD_STATUS eResult;
                step_iter->second->SetActiveTime(ev_now(m_loop));
                LOG4_TRACE("cmd %u, seq %u, step_seq %u, step addr 0x%x, active_time %lf",
                                oInMsgHead.cmd(), oInMsgHead.seq(), step_iter->second->GetSequence(),
                                step_iter->second, step_iter->second->GetActiveTime());
                eResult = step_iter->second->Callback(stMsgShell, oInMsgHead, oInMsgBody);
//                LOG4_TRACE("cmd %u, seq %u, step_seq %u, step addr 0x%x",
//                                oInMsgHead.cmd(), oInMsgHead.seq(), step_iter->second->GetSequence(),
//                                step_iter->second);
                if (eResult != STATUS_CMD_RUNNING)
                {
                    DeleteCallback(step_iter->second);
                }
            }
        }
        else
        {
            snprintf(m_pErrBuff, gc_iErrBuffLen, "no callback or the callback for seq %lu had been timeout!", oInMsgHead.seq());
            LOG4_WARN(m_pErrBuff);
        }
    }
    return(true);
}
/**
 * @brief 收到完整的hhtp包后处理
 * @param stMsgShell 数据包来源消息外壳
 * @param oInHttpMsg 接收的HTTP包
 * @param oOutHttpMsg 待发送的HTTP包
 * @return 是否继续解析数据包（注意不是处理结果）
 */
bool Worker::Dispose(const tagMsgShell& stMsgShell,
                const HttpMsg& oInHttpMsg, HttpMsg& oOutHttpMsg)
{
    LOG4_DEBUG("%s() oInHttpMsg.type() = %d, oInHttpMsg.path() = %s",
                    __FUNCTION__, oInHttpMsg.type(), oInHttpMsg.path().c_str());
    oOutHttpMsg.Clear();
    if (HTTP_REQUEST == oInHttpMsg.type())    // 新请求
    {
        std::unordered_map<std::string, tagModule*>::iterator module_iter;
        module_iter = m_mapModule.find(oInHttpMsg.path());
        if (module_iter == m_mapModule.end())
        {
            module_iter = m_mapModule.find("/util/switch");
            if (module_iter == m_mapModule.end())
            {
                snprintf(m_pErrBuff, gc_iErrBuffLen, "no module to dispose %s!", oInHttpMsg.path().c_str());
                LOG4_ERROR(m_pErrBuff);
                oOutHttpMsg.set_type(HTTP_RESPONSE);
                oOutHttpMsg.set_status_code(404);
                oOutHttpMsg.set_http_major(oInHttpMsg.http_major());
                oOutHttpMsg.set_http_minor(oInHttpMsg.http_minor());
            }
            else
            {
                module_iter->second->pModule->AnyMessage(stMsgShell, oInHttpMsg);
            }
        }
        else
        {
            module_iter->second->pModule->AnyMessage(stMsgShell, oInHttpMsg);
        }
    }
    else
    {
        std::unordered_map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
        if (http_step_iter == m_mapHttpAttr.end())
        {
            LOG4_ERROR("no callback for http response from %s!", oInHttpMsg.url().c_str());
        }
        else
        {
            if (http_step_iter->second.begin() != http_step_iter->second.end())
            {
                std::unordered_map<uint32, Step*>::iterator step_iter;
                step_iter = m_mapCallbackStep.find(*http_step_iter->second.begin());
                if (step_iter != m_mapCallbackStep.end() && step_iter->second != NULL)   // 步骤回调
                {
                    E_CMD_STATUS eResult;
                    step_iter->second->SetActiveTime(ev_now(m_loop));
                    eResult = ((HttpStep*)step_iter->second)->Callback(stMsgShell, oInHttpMsg);
                    if (eResult != STATUS_CMD_RUNNING)
                    {
                        DeleteCallback(step_iter->second);
                    }
                }
                else
                {
                    snprintf(m_pErrBuff, gc_iErrBuffLen, "no callback or the callback for seq %lu had been timeout!",
                                    *http_step_iter->second.begin());
                    LOG4_WARN(m_pErrBuff);
                }
                http_step_iter->second.erase(http_step_iter->second.begin());
            }
            else
            {
                LOG4_ERROR("no callback for http response from %s!", oInHttpMsg.url().c_str());
            }
        }
    }
    return(true);
}

bool Worker::Dispose(util::MysqlAsyncConn *c, util::SqlTask *task, MYSQL_RES *pResultSet)
{
	std::unordered_map<uint32, Step*>::iterator step_iter;
	step_iter = m_mapCallbackStep.find(((CustomMysqlHandler*)task->handler)->m_uiMysqlStepSeq);
	if (step_iter != m_mapCallbackStep.end() && step_iter->second != NULL)   // 步骤回调
	{
		E_CMD_STATUS eResult;
		step_iter->second->SetActiveTime(ev_now(m_loop));
		eResult = ((MysqlStep*)step_iter->second)->Callback(c,task,pResultSet);
		if (eResult != STATUS_CMD_RUNNING)
		{
			DeleteCallback(step_iter->second);
		}
	}
	else
	{
		snprintf(m_pErrBuff, gc_iErrBuffLen, "no callback or the callback for MysqlStepSeq %lu had been timeout!",
				((CustomMysqlHandler*)task->handler)->m_uiMysqlStepSeq);
		LOG4_WARN(m_pErrBuff);
	}
	return(true);
}

} /* namespace net */
