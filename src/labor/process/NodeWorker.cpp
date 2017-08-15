
/*******************************************************************************
 * Project:  AsyncServer
 * @file     ThunderWorker.cpp
 * @brief 
 * @author   cjy
 * @date:    2015年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
#include "hiredis/async.h"
#include "hiredis/adapters/libev.h"
#include "unix_util/process_helper.h"
#include "unix_util/proctitle_helper.h"
#ifdef __cplusplus
}
#endif
#include "codec/ProtoCodec.hpp"
#include "codec/CustomMsgCodec.hpp"
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
#include "NodeWorker.hpp"

namespace thunder
{

tagSo::tagSo() : pSoHandle(NULL), pCmd(NULL), iVersion(0)
{}

tagSo::~tagSo()
{
    if (pCmd != NULL)
    {
        delete pCmd;
        pCmd = NULL;
    }
    if (pSoHandle != NULL)
    {
        dlclose(pSoHandle);
        pSoHandle = NULL;
    }
}

tagModule::tagModule() : pSoHandle(NULL), pModule(NULL), iVersion(0)
{
}

tagModule::~tagModule()
{
    if (pModule != NULL)
    {
        delete pModule;
        pModule = NULL;
    }
    if (pSoHandle != NULL)
    {
        dlclose(pSoHandle);
        pSoHandle = NULL;
    }
}


void ThunderWorker::TerminatedCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        ThunderWorker* pWorker = (ThunderWorker*)watcher->data;
        pWorker->Terminated(watcher);  // timeout，worker进程无响应或与Manager通信通道异常，被manager进程终止时返回
    }
}

void ThunderWorker::IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        ThunderWorker* pWorker = (ThunderWorker*)watcher->data;
        pWorker->CheckParent();
    }
}

void ThunderWorker::IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagIoWatcherData* pData = (tagIoWatcherData*)watcher->data;
        ThunderWorker* pWorker = (ThunderWorker*)pData->pWorker;
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

void ThunderWorker::IoTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagIoWatcherData* pData = (tagIoWatcherData*)watcher->data;
        ThunderWorker* pWorker = pData->pWorker;
        pWorker->IoTimeout(watcher);
    }
}

void ThunderWorker::PeriodicTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        ThunderWorker* pWorker = (ThunderWorker*)(watcher->data);

        pWorker->CheckParent();

    }
    ev_timer_stop (loop, watcher);
    ev_timer_set (watcher, NODE_BEAT + ev_time() - ev_now(loop), 0);
    ev_timer_start (loop, watcher);
}

void ThunderWorker::StepTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Step* pStep = (Step*)watcher->data;
        ((ThunderWorker*)(pStep->GetLabor()))->StepTimeout(pStep, watcher);
    }
}

void ThunderWorker::SessionTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Session* pSession = (Session*)watcher->data;
        ((ThunderWorker*)pSession->GetLabor())->SessionTimeout(pSession, watcher);
    }
}

void ThunderWorker::RedisConnectCallback(const redisAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        ThunderWorker* pWorker = (ThunderWorker*)c->data;
        pWorker->RedisConnect(c, status);
    }
}

void ThunderWorker::RedisDisconnectCallback(const redisAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        ThunderWorker* pWorker = (ThunderWorker*)c->data;
        pWorker->RedisDisconnect(c, status);
    }
}

void ThunderWorker::RedisCmdCallback(redisAsyncContext *c, void *reply, void *privdata)
{
    if (c->data != NULL)
    {
        ThunderWorker* pWorker = (ThunderWorker*)c->data;
        pWorker->RedisCmdResult(c, reply, privdata);
    }
}

ThunderWorker::ThunderWorker(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, thunder::CJsonObject& oJsonConf)
    : m_pErrBuff(NULL), m_ulSequence(0), m_bInitLogger(false), m_dIoTimeout(480.0), m_strWorkPath(strWorkPath), m_uiNodeId(0),
      m_iManagerControlFd(iControlFd), m_iManagerDataFd(iDataFd), m_iWorkerIndex(iWorkerIndex), m_iWorkerPid(0),
      m_dMsgStatInterval(60.0), m_iMsgPermitNum(60),
      m_iRecvNum(0), m_iRecvByte(0), m_iSendNum(0), m_iSendByte(0),
      m_loop(NULL), m_pCmdConnect(NULL)
{
	m_iWorkerPid = getpid();
    m_pErrBuff = new char[gc_iErrBuffLen];
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
    m_pSchedule = coroutine_open();
}

ThunderWorker::~ThunderWorker()
{
	if (m_pSchedule)
	{
		coroutine_close(m_pSchedule);
	}
    Destroy();
}

void ThunderWorker::Run()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_run (m_loop, 0);
}

void ThunderWorker::Terminated(struct ev_signal* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iSignum = watcher->signum;
    delete watcher;
    //Destroy();
    LOG4_FATAL("terminated by signal %d!", iSignum);
    exit(iSignum);
}

bool ThunderWorker::CheckParent()
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    pid_t iParentPid = getppid();
    if (iParentPid == 1)    // manager进程已不存在
    {
        LOG4_INFO("no manager process exist, worker %d exit.", m_iWorkerIndex);
        //Destroy();
        exit(0);
    }
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    thunder::CJsonObject oJsonLoad;
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
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(m_iManagerControlFd);
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

bool ThunderWorker::SendToParent(const MsgHead& oMsgHead,const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(m_iManagerControlFd);
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

bool ThunderWorker::IoRead(tagIoWatcherData* pData, struct ev_io* watcher)
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

bool ThunderWorker::RecvDataAndDispose(tagIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iErrno = 0;
    int iReadLen = 0;
    std::map<int, tagConnectionAttr*>::iterator conn_iter;
    conn_iter = m_mapFdAttr.find(pData->iFd);
    if (conn_iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd attr for %d!", pData->iFd);
    }
    else
    {
        conn_iter->second->dActiveTime = ev_now(m_loop);
        if (pData->ulSeq != conn_iter->second->ulSeq)
        {
            LOG4_DEBUG("callback seq %lu not match the conn attr seq %lu",
                            pData->ulSeq, conn_iter->second->ulSeq);
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
        iReadLen = conn_iter->second->pRecvBuff->ReadFD(pData->iFd, iErrno);
        if (NULL == conn_iter->second->pRemoteAddr)
        {
            LOG4_TRACE("recv from fd %d identify %s, data len %d codec %d",
                            pData->iFd, conn_iter->second->strIdentify.c_str(),
                            iReadLen, conn_iter->second->eCodecType);
        }
        else
        {
            LOG4_TRACE("recv from fd %d ip %s identify %s, data len %d codec %d",
                            pData->iFd, conn_iter->second->pRemoteAddr,
                            conn_iter->second->strIdentify.c_str(),
                            iReadLen, conn_iter->second->eCodecType);
        }
        if (iReadLen > 0)
        {
            m_iRecvByte += iReadLen;
            conn_iter->second->ulMsgNumUnitTime++;      // TODO 这里要做发送消息频率限制，有待补充
            conn_iter->second->ulMsgNum++;
            MsgHead oInMsgHead, oOutMsgHead;
            MsgBody oInMsgBody, oOutMsgBody;
            ThunderCodec* pCodec = NULL;
            std::map<thunder::E_CODEC_TYPE, ThunderCodec*>::iterator codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
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
            while (conn_iter->second->pRecvBuff->ReadableBytes() >= gc_uitagCustomMsgHeadSize)
            {
                oInMsgHead.Clear();
                oInMsgBody.Clear();
                E_CODEC_STATUS eCodecStatus = codec_iter->second->Decode(conn_iter->second, oInMsgHead, oInMsgBody);
#ifdef NODE_TYPE_GATE
                //网关类型节点的连接初始化时支持协议编解码器的替换（支持的是websocket json 或websocket pb与http,private的替换）
                if (eConnectStatus_init == conn_iter->second->ucConnectStatus)
                {
                    //网关默认配置websocket json(可修改为websocket pb)
                    if (CODEC_STATUS_ERR == eCodecStatus &&
                        (thunder::CODEC_WEBSOCKET_EX_JS == conn_iter->second->eCodecType || thunder::CODEC_WEBSOCKET_EX_PB == conn_iter->second->eCodecType))
                    {
                        //切换为http协议
                        LOG4_DEBUG("failed to decode for codec %d,switch to CODEC_HTTP",conn_iter->second->eCodecType);
                        conn_iter->second->eCodecType = thunder::CODEC_HTTP;
                        std::map<thunder::E_CODEC_TYPE, ThunderCodec*>::iterator codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
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
                        eCodecStatus = codec_iter->second->Decode(conn_iter->second, oInMsgHead, oInMsgBody);
                    }
                    if (CODEC_STATUS_ERR == eCodecStatus && thunder::CODEC_HTTP == conn_iter->second->eCodecType)
                    {
                        //切换为私有协议编解码（与客户端通信协议） private pb
                        LOG4_DEBUG("failed to decode for codec %d,switch to CODEC_PRIVATE",conn_iter->second->eCodecType);
                        conn_iter->second->eCodecType = thunder::CODEC_PRIVATE;
                        std::map<thunder::E_CODEC_TYPE, ThunderCodec*>::iterator codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
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
                        eCodecStatus = codec_iter->second->Decode(conn_iter->second, oInMsgHead, oInMsgBody);
                    }
                }
#endif
                if (CODEC_STATUS_OK == eCodecStatus)
                {
                    ++m_iRecvNum;
                    conn_iter->second->dActiveTime = ev_now(m_loop);
                    bool bDisposeResult = false;
                    MsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = conn_iter->second->ulSeq;
                    // 基于TCP的自定义协议请求或带cmd、seq自定义头域的请求
                    //http短连接不需要带cmd
                    if (oInMsgHead.cmd() > 0)
                    {
#ifdef NODE_TYPE_GATE
                        if (conn_iter->second->pClientData != NULL && conn_iter->second->pClientData->ReadableBytes() > 0)
                        {//长连接并且验证过的，不需要再验证。只填充附带数据。
                            oInMsgBody.set_additional(conn_iter->second->pClientData->GetRawReadBuffer(),
                                            conn_iter->second->pClientData->ReadableBytes());
                            oInMsgHead.set_msgbody_len(oInMsgBody.ByteSize());
                        }
                        else
                        {
                            // 如果是websocket连接，则需要验证连接
                            if (thunder::CODEC_WEBSOCKET_EX_PB == conn_iter->second->eCodecType ||
                                            thunder::CODEC_WEBSOCKET_EX_JS == conn_iter->second->eCodecType)
                            {
                                std::map<int32, std::list<uint32> >::iterator http_iter = m_mapHttpAttr.find(stMsgShell.iFd);
                                if (http_iter == m_mapHttpAttr.end())   // 未经握手的websocket客户端连接发送数据过来，直接断开
                                {
                                    LOG4_DEBUG("invalid request, please handshake first!");
                                    DestroyConnect(conn_iter);
                                    return(false);
                                }
                            }
                            else//其他类型长连接
                            {
                                std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(stMsgShell.iFd);
                                if (inner_iter == m_mapInnerFd.end() && conn_iter->second->ulMsgNum > 1)   // 未经账号验证的客户端连接发送数据过来，直接断开
                                {
                                    LOG4_DEBUG("invalid request, please login first!");
                                    DestroyConnect(conn_iter);
                                    return(false);
                                }
                            }
                        }
#endif
                        if ((gc_uiCmdReq & oInMsgHead.cmd()) && oInMsgHead.seq() > 0)
                        {
                            conn_iter->second->ulForeignSeq = oInMsgHead.seq();
                        }
                        bDisposeResult = Dispose(stMsgShell, oInMsgHead, oInMsgBody, oOutMsgHead, oOutMsgBody); // 处理过程有可能会断开连接，所以下面要做连接是否存在检查
                        std::map<int, tagConnectionAttr*>::iterator dispose_conn_iter = m_mapFdAttr.find(pData->iFd);
                        if (dispose_conn_iter == m_mapFdAttr.end())     // 连接已断开，资源已回收
                        {
                            return(true);
                        }
                        else
                        {
                            if (pData->ulSeq != dispose_conn_iter->second->ulSeq)     // 连接已断开，资源已回收
                            {
                                return(true);
                            }
                        }
                        if (oOutMsgHead.ByteSize() > 0)
                        {
                            eCodecStatus = codec_iter->second->Encode(oOutMsgHead, oOutMsgBody, conn_iter->second->pSendBuff);
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
                    else  // url方式的http请求
                    {
                        HttpMsg oInHttpMsg;
                        HttpMsg oOutHttpMsg;
                        if (oInHttpMsg.ParseFromString(oInMsgBody.body()))
                        {
                            if (thunder::CODEC_WEBSOCKET_EX_PB != conn_iter->second->eCodecType &&
                                    thunder::CODEC_WEBSOCKET_EX_JS != conn_iter->second->eCodecType)
                            {
                                conn_iter->second->dKeepAlive = 10;   // 未带KeepAlive参数的http协议，默认10秒钟关闭
                                LOG4_TRACE("set dKeepAlive(%lf)",conn_iter->second->dKeepAlive);
                            }
                            else
                            {
                                LOG4_TRACE("set dKeepAlive(%lf)",conn_iter->second->dKeepAlive);//websocket保持长连接,dKeepAlive为0
                            }
                            for (int i = 0; i < oInHttpMsg.headers_size(); ++i)
                            {
                                if (std::string("Keep-Alive") == oInHttpMsg.headers(i).header_name())
                                {
                                    conn_iter->second->dKeepAlive = strtoul(oInHttpMsg.headers(i).header_value().c_str(), NULL, 10);
                                    LOG4_TRACE("set dKeepAlive(%lf)",conn_iter->second->dKeepAlive);
                                    AddIoTimeout(conn_iter->first, conn_iter->second->ulSeq, conn_iter->second, conn_iter->second->dKeepAlive);
                                    break;
                                }
                                else if (std::string("Connection") == oInHttpMsg.headers(i).header_name())
                                {
                                    if (std::string("keep-alive") == oInHttpMsg.headers(i).header_value())
                                    {
                                        conn_iter->second->dKeepAlive = 65.0;
                                        LOG4_TRACE("set dKeepAlive(%lf)",conn_iter->second->dKeepAlive);
                                        AddIoTimeout(conn_iter->first, conn_iter->second->ulSeq, conn_iter->second, 65.0);
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
                                        AddIoTimeout(conn_iter->first, conn_iter->second->ulSeq, conn_iter->second, conn_iter->second->dKeepAlive);
                                    }
                                }
                            }
                            bDisposeResult = Dispose(stMsgShell, oInHttpMsg, oOutHttpMsg);  // 处理过程有可能会断开连接，所以下面要做连接是否存在检查
                            std::map<int, tagConnectionAttr*>::iterator dispose_conn_iter = m_mapFdAttr.find(pData->iFd);
                            if (dispose_conn_iter == m_mapFdAttr.end())     // 连接已断开，资源已回收
                            {
                                return(true);
                            }
                            else
                            {
                                if (pData->ulSeq != dispose_conn_iter->second->ulSeq)     // 连接已断开，资源已回收
                                {
                                    return(true);
                                }
                            }
                            if (conn_iter->second->dKeepAlive < 0)
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
        }
        else
        {
            LOG4_TRACE("recv from fd %d errno %d: %s",
                            pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
            if (EAGAIN == iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                ;
            }
            else if (EINTR == iErrno)
            {
                goto read_again;
            }
            else
            {
                LOG4_ERROR("recv from fd %d errno %d: %s",
                                pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
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

bool ThunderWorker::FdTransfer()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    char szIpAddr[16] = {0};
    int iCodec = 0;
    // int iAcceptFd = recv_fd(m_iManagerDataFd);
    int iAcceptFd = recv_fd_with_attr(m_iManagerDataFd, szIpAddr, 16, &iCodec);
    if (iAcceptFd <= 0)
    {
        if (iAcceptFd == 0)
        {
            LOG4_ERROR("recv_fd from m_iManagerDataFd %d len %d", m_iManagerDataFd, iAcceptFd);
            exit(2); // manager与worker通信fd已关闭，worker进程退出
        }
        else if (errno != EAGAIN)
        {
            LOG4_ERROR("recv_fd from m_iManagerDataFd %d error %d", m_iManagerDataFd, errno);
            //Destroy();
            exit(2); // manager与worker通信fd已关闭，worker进程退出
        }
    }
    else
    {
        uint32 ulSeq = GetSequence();
        tagConnectionAttr* pConnAttr = CreateFdAttr(iAcceptFd, ulSeq, thunder::E_CODEC_TYPE(iCodec));
        x_sock_set_block(iAcceptFd, 0);
        if (pConnAttr)
        {
            snprintf(pConnAttr->pRemoteAddr, 32, szIpAddr);
            LOG4_DEBUG("pConnAttr->pClientAddr = %s, iCodec = %d", pConnAttr->pRemoteAddr, iCodec);
            std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(iAcceptFd);
            if(AddIoTimeout(iAcceptFd, ulSeq, iter->second, 1.5))     // 为了防止大量连接攻击，初始化连接只有一秒即超时，在第一次超时检查（或正常发送第一个http数据包）之后才采用正常配置的网络IO超时检查
            {
                if (!AddIoReadEvent(iter))
                {
                    LOG4_DEBUG("if (!AddIoReadEvent(iter))");
                    DestroyConnect(iter);
                    return(false);
                }
//                if (!AddIoErrorEvent(iAcceptFd))
//                {
//                    DestroyConnect(iter);
//                    return(false);
//                }
                return(true);
            }
            else
            {
                LOG4_DEBUG("if(AddIoTimeout(iAcceptFd, ulSeq, iter->second, 1.5)) else");
                DestroyConnect(iter);
                return(false);
            }
        }
        else    // 没有足够资源分配给新连接，直接close掉
        {
            close(iAcceptFd);
        }
    }
    return(false);
}

bool ThunderWorker::IoWrite(tagIoWatcherData* pData, struct ev_io* watcher)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator attr_iter =  m_mapFdAttr.find(pData->iFd);
    if (attr_iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
        if (pData->ulSeq != attr_iter->second->ulSeq)
        {
            LOG4_DEBUG("callback seq %lu not match the conn attr seq %lu",
                            pData->ulSeq, attr_iter->second->ulSeq);
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
        iWriteLen = attr_iter->second->pSendBuff->WriteFD(pData->iFd, iErrno);
        attr_iter->second->pSendBuff->Compact(8192);
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
                AddIoWriteEvent(attr_iter);
            }
        }
        else if (iWriteLen > 0)
        {
            m_iSendByte += iWriteLen;
            attr_iter->second->dActiveTime = ev_now(m_loop);
            if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
            {
                RemoveIoWriteEvent(attr_iter);
            }
            else    // 内容未写完，添加或保持监听fd写事件
            {
                AddIoWriteEvent(attr_iter);
            }
        }
        else    // iWriteLen == 0 写缓冲区为空
        {
//            LOG4_TRACE("pData->iFd %d, watcher->fd %d, iter->second->pWaitForSendBuff->ReadableBytes()=%d",
//                            pData->iFd, watcher->fd, attr_iter->second->pWaitForSendBuff->ReadableBytes());
            if (attr_iter->second->pWaitForSendBuff->ReadableBytes() > 0)    // 存在等待发送的数据，说明本次写事件是connect之后的第一个写事件
            {
                std::map<uint32, int>::iterator index_iter = m_mapSeq2WorkerIndex.find(attr_iter->second->ulSeq);
                if (index_iter != m_mapSeq2WorkerIndex.end())   // 系统内部Server间通信需由NodeManager转发
                {
                    MsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = attr_iter->second->ulSeq;
                    AddInnerFd(stMsgShell);
                    if (thunder::CODEC_PROTOBUF == attr_iter->second->eCodecType)  // 系统内部Server间通信
                    {
                        m_pCmdConnect->Start(stMsgShell, index_iter->second);
                    }
                    else        // 与系统外部Server通信，连接成功后直接将数据发送
                    {
                        SendTo(stMsgShell);
                    }
                    m_mapSeq2WorkerIndex.erase(index_iter);
                    LOG4_TRACE("RemoveIoWriteEvent(%d)", pData->iFd);
                    RemoveIoWriteEvent(attr_iter);    // 在m_pCmdConnect的两个回调之后再把等待发送的数据发送出去
                }
                else // 与系统外部Server通信，连接成功后直接将数据发送
                {
                    MsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = attr_iter->second->ulSeq;
                    SendTo(stMsgShell);
                }
            }
        }
        return(true);
    }
}

bool ThunderWorker::IoError(tagIoWatcherData* pData, struct ev_io* watcher)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(pData->iFd);
    if (iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
        LOG4_TRACE("if (iter == m_mapFdAttr.end()) else");
        if (pData->ulSeq != iter->second->ulSeq)
        {
            LOG4_DEBUG("callback seq %lu not match the conn attr seq %lu",
                            pData->ulSeq, iter->second->ulSeq);
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

bool ThunderWorker::IoTimeout(struct ev_timer* watcher, bool bCheckBeat)
{
    LOG4_TRACE("%s()",__FUNCTION__);
    bool bRes = false;
    tagIoWatcherData* pData = (tagIoWatcherData*)watcher->data;
    if (pData == NULL)
    {
        LOG4_ERROR("pData is null in %s()", __FUNCTION__);
        ev_timer_stop(m_loop, watcher);
        pData->pWorker = NULL;
        delete pData;
        watcher->data = NULL;
        delete watcher;
        watcher = NULL;
        return(false);
    }
    std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(pData->iFd);
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
            LOG4_TRACE("%s() IoTimeout time(%lf) IoTimeout(%lf) bCheckBeat(%d) dKeepAlive(%lf) RemoteAddr(%s)",
                            __FUNCTION__,ev_now(m_loop),m_dIoTimeout,bCheckBeat ? 1 : 0,iter->second->dKeepAlive,
                                            iter->second->pRemoteAddr ? iter->second->pRemoteAddr:"");
            if (bCheckBeat && iter->second->dKeepAlive == 0)     // 需要发送心跳检查 或 完成心跳检查并未超时
            {
                ev_tstamp after = iter->second->dActiveTime - ev_now(m_loop) + m_dIoTimeout;
                if (after > 0)    // IO在定时时间内被重新刷新过，重新设置定时器
                {
                    ev_timer_stop (m_loop, watcher);
                    ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
                    ev_timer_start (m_loop, watcher);
                    LOG4_TRACE("ev_timer_set:after(%lf) ev_time(%lf) ev_now(%lf) IoTimeout(%lf)",
                                    after,ev_time(),ev_now(m_loop),m_dIoTimeout);
                    return(true);
                }
                else    // IO已超时，发送心跳检查
                {
                    MsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = iter->second->ulSeq;
                    StepIoTimeout* pStepIoTimeout = new StepIoTimeout(stMsgShell, watcher);
                    if (pStepIoTimeout != NULL)
                    {
                        if (RegisterCallback(pStepIoTimeout,2.0))
                        {
                            LOG4_TRACE("RegisterCallback(pStepIoTimeout) time(%lf)",ev_now(m_loop));
                            thunder::E_CMD_STATUS eStatus = pStepIoTimeout->Emit(ERR_OK);
                            if (STATUS_CMD_RUNNING != eStatus)
                            {
                                // pStepIoTimeout->Start()会发送心跳包，若干返回非running状态，则表明发包时已出错，
                                // 销毁连接过程在SentTo里已经完成，这里不需要再销毁连接
                                DeleteCallback(pStepIoTimeout);
                            }
                            else
                            {
                                return(true);
                            }
                        }
                        else
                        {
                            LOG4_TRACE("if (RegisterCallback(pStepIoTimeout)) else");
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
                        LOG4_TRACE("ev_timer_set(watcher) time(%lf) dKeepAlive(%lf)",ev_now(m_loop),
                                        iter->second->dKeepAlive);
                        return(true);
                    }
                }
                MsgShell stMsgShell;
                stMsgShell.iFd = pData->iFd;
                stMsgShell.ulSeq = iter->second->ulSeq;
                LOG4_TRACE("io timeout ClientAddr(%s)",GetClientAddr(stMsgShell).c_str());
                std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(pData->iFd);
                if (inner_iter == m_mapInnerFd.end())   // 非内部服务器间的连接才会在超时中关闭
                {
                    LOG4_TRACE("io timeout and send beat, but there is no beat response received!");
                    DestroyConnect(iter);
                }
            }
            bRes = true;
        }
    }

    ev_timer_stop(m_loop, watcher);
    pData->pWorker = NULL;
    delete pData;
    watcher->data = NULL;
    delete watcher;
    watcher = NULL;
    return(bRes);
}

bool ThunderWorker::StepTimeout(Step* pStep, struct ev_timer* watcher)
{
    ev_tstamp after = pStep->GetActiveTime() - ev_now(m_loop) + pStep->GetTimeout();
    if (after > 0)    // 在定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, watcher);
        return(true);
    }
    else    // 会话已超时
    {
        LOG4_TRACE("%s(seq %lu): active_time %lf, now_time %lf, lifetime %lf",
                        __FUNCTION__, pStep->GetSequence(), pStep->GetActiveTime(), ev_now(m_loop), pStep->GetTimeout());
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

bool ThunderWorker::SessionTimeout(Session* pSession, struct ev_timer* watcher)
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
        LOG4_TRACE("%s(session_name: %s,  session_id: %s)",
                        __FUNCTION__, pSession->GetSessionClass().c_str(), pSession->GetSessionId().c_str());
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

bool ThunderWorker::RedisConnect(const redisAsyncContext *c, int status)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<redisAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisAsyncContext*)c);
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

bool ThunderWorker::RedisDisconnect(const redisAsyncContext *c, int status)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::map<redisAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        for (std::list<RedisStep *>::iterator step_iter = attr_iter->second->listData.begin();
                        step_iter != attr_iter->second->listData.end(); ++step_iter)
        {
            LOG4_ERROR("RedisDisconnect callback error %d of redis cmd: %s",
                            c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());
            (*step_iter)->Callback(c, c->err, NULL);
            delete (*step_iter);
            (*step_iter) = NULL;
        }
        attr_iter->second->listData.clear();

        for (std::list<RedisStep *>::iterator step_iter = attr_iter->second->listWaitData.begin();
                        step_iter != attr_iter->second->listWaitData.end(); ++step_iter)
        {
            LOG4_ERROR("RedisDisconnect callback error %d of redis cmd: %s",
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

bool ThunderWorker::RedisCmdResult(redisAsyncContext *c, void *reply, void *privdata)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::map<redisAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        std::list<RedisStep*>::iterator step_iter = attr_iter->second->listData.begin();
        if (NULL == reply)
        {
            std::map<const redisAsyncContext*, std::string>::iterator identify_iter = m_mapContextIdentify.find(c);
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

time_t ThunderWorker::GetNowTime() const
{
    return((time_t)ev_now(m_loop));
}

bool ThunderWorker::Pretreat(Cmd* pCmd)
{
    LOG4_TRACE("%s(Cmd*)", __FUNCTION__);
    if (pCmd == NULL)
    {
        return(false);
    }
    pCmd->SetLabor(this);
    pCmd->SetLogger(&m_oLogger);
    return(true);
}

bool ThunderWorker::Pretreat(Step* pStep)
{
    LOG4_TRACE("%s(Step*)", __FUNCTION__);
    if (pStep == NULL)
    {
        return(false);
    }
    pStep->SetLabor(this);
    pStep->SetLogger(&m_oLogger);
    return(true);
}

bool ThunderWorker::Pretreat(Session* pSession)
{
    LOG4_TRACE("%s(Session*)", __FUNCTION__);
    if (pSession == NULL)
    {
        return(false);
    }
    pSession->SetLabor(this);
    pSession->SetLogger(&m_oLogger);
    return(true);
}

int ThunderWorker::NewCoroutine(coroutine_func func,void *ud)
{
	int co1 = coroutine_new(m_pSchedule, func, ud);
	LOG4_TRACE("%s coroutine_new co1:%d", __FUNCTION__,co1);
	if (co1 == -1)
	{
		LOG4_ERROR("%s coroutine invalid co1(%u)", __FUNCTION__, co1);
	}
	return co1;
}

void ThunderWorker::RunCoroutine(int co1)
{
	int running_id = coroutine_running(m_pSchedule);
	if (running_id != -1)//抢占式
	{
		LOG4_TRACE("%s coroutine_yield running_id(%d)", __FUNCTION__, running_id);
		coroutine_yield(m_pSchedule);//放弃执行权
	}
	LOG4_TRACE("%s coroutine_resume co1(%d)", __FUNCTION__, co1);
	coroutine_resume(m_pSchedule,co1);//执行函数
}

void ThunderWorker::YieldCoroutine()
{
	int running_id = coroutine_running(m_pSchedule);
	if (running_id != -1)
	{
		LOG4_TRACE("%s coroutine_yield running_id(%d)", __FUNCTION__, running_id);
		coroutine_yield(m_pSchedule);//放弃执行权
	}
	else
	{
		LOG4_ERROR("%s no running coroutine", __FUNCTION__);
	}
}

bool ThunderWorker::RegisterCallback(Step* pStep, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pStep, dTimeout);
    if (pStep == NULL)
    {
        return(false);
    }
    if (pStep->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功
    {
        LOG4_WARN("Step(seq %u) registered already.",pStep->GetSequence());
        return(true);
    }
    pStep->SetLabor(this);
    pStep->SetLogger(&m_oLogger);
    pStep->SetRegistered();
    pStep->SetActiveTime(ev_now(m_loop));
    std::pair<std::map<uint32, Step*>::iterator, bool> ret
        = m_mapCallbackStep.insert(std::pair<uint32, Step*>(pStep->GetSequence(), pStep));
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

bool ThunderWorker::RegisterCallback(uint32 uiSelfStepSeq, Step* pStep, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pStep, dTimeout);
    if (pStep == NULL)
    {
        return(false);
    }
    std::map<uint32, Step*>::iterator callback_iter;
    std::set<uint32>::iterator next_step_seq_iter;
    if (pStep->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功
    {
        // 登记前置step
        next_step_seq_iter = pStep->m_setNextStepSeq.find(uiSelfStepSeq);
        if (next_step_seq_iter != pStep->m_setNextStepSeq.end())
        {
            callback_iter = m_mapCallbackStep.find(uiSelfStepSeq);
            if (callback_iter != m_mapCallbackStep.end())
            {
                callback_iter->second->m_setPreStepSeq.insert(pStep->GetSequence());
            }
        }
        return(true);
    }
    pStep->SetLabor(this);
    pStep->SetLogger(&m_oLogger);
    pStep->SetRegistered();
    pStep->SetActiveTime(ev_now(m_loop));

    // 登记前置step
    next_step_seq_iter = pStep->m_setNextStepSeq.find(uiSelfStepSeq);
    if (next_step_seq_iter != pStep->m_setNextStepSeq.end())
    {
        callback_iter = m_mapCallbackStep.find(uiSelfStepSeq);
        if (callback_iter != m_mapCallbackStep.end())
        {
            callback_iter->second->m_setPreStepSeq.insert(pStep->GetSequence());
        }
    }

    std::pair<std::map<uint32, Step*>::iterator, bool> ret
        = m_mapCallbackStep.insert(std::pair<uint32, Step*>(pStep->GetSequence(), pStep));
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

void ThunderWorker::DeleteCallback(Step* pStep)
{
    LOG4_TRACE("%s(Step* 0x%X)", __FUNCTION__, pStep);
    if (pStep == NULL)
    {
        return;
    }
    std::map<uint32, Step*>::iterator callback_iter;
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

void ThunderWorker::DeleteCallback(uint32 uiSelfStepSeq, Step* pStep)
{
    LOG4_TRACE("%s(self_seq[%u], Step* 0x%X)", __FUNCTION__, uiSelfStepSeq, pStep);
    if (pStep == NULL)
    {
        return;
    }
    std::map<uint32, Step*>::iterator callback_iter;
    for (std::set<uint32>::iterator step_seq_iter = pStep->m_setPreStepSeq.begin();
                    step_seq_iter != pStep->m_setPreStepSeq.end(); )
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
        LOG4_TRACE("step[%u] try to delete step[%u]", uiSelfStepSeq, pStep->GetSequence());
        delete pStep;
        m_mapCallbackStep.erase(callback_iter);
    }
}

/*
bool ThunderWorker::UnRegisterCallback(Step* pStep)
{
    LOG4_TRACE("%s(Step* 0x%X)", __FUNCTION__, pStep);
    if (pStep == NULL)
    {
        return(false);
    }
    if (pStep->m_pTimeoutWatcher != NULL)
    {
        ev_timer_stop (m_loop, pStep->m_pTimeoutWatcher);
    }
    std::map<uint32, Step*>::iterator iter = m_mapCallbackStep.find(pStep->GetSequence());
    if (iter != m_mapCallbackStep.end())
    {
        LOG4_TRACE("unRigester step(seq %u)", pStep->GetSequence());
        pStep->UnsetRegistered();
        m_mapCallbackStep.erase(iter);
    }
    return(true);
}
*/

bool ThunderWorker::RegisterCallback(Session* pSession)
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
    pSession->SetLabor(this);
    pSession->SetLogger(&m_oLogger);
    pSession->SetRegistered();
    pSession->SetActiveTime(ev_now(m_loop));

    std::pair<std::map<std::string, Session*>::iterator, bool> ret;
    std::map<std::string, std::map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(pSession->GetSessionClass());
    if (name_iter == m_mapCallbackSession.end())
    {
        std::map<std::string, Session*> mapSession;
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
        m_mapCallbackSession.insert(std::pair<std::string, std::map<std::string, Session*> >(pSession->GetSessionClass(), mapSession));
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

void ThunderWorker::DeleteCallback(Session* pSession)
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
    std::map<std::string, std::map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(pSession->GetSessionClass());
    if (name_iter != m_mapCallbackSession.end())
    {
        std::map<std::string, Session*>::iterator id_iter = name_iter->second.find(pSession->GetSessionId());
        if (id_iter != name_iter->second.end())
        {
            LOG4_TRACE("delete session(session_id %s)", pSession->GetSessionId().c_str());
            delete id_iter->second;
            id_iter->second = NULL;
            name_iter->second.erase(id_iter);
        }
    }
}

bool ThunderWorker::RegisterCallback(const redisAsyncContext* pRedisContext, RedisStep* pRedisStep)
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
    pRedisStep->SetLabor(this);
    pRedisStep->SetLogger(&m_oLogger);
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

    std::map<redisAsyncContext*, tagRedisAttr*>::iterator iter = m_mapRedisAttr.find((redisAsyncContext*)pRedisContext);
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

bool ThunderWorker::ResetTimeout(Step* pStep, struct ev_timer* watcher)
{
    ev_tstamp after = ev_now(m_loop) + pStep->GetTimeout();
    ev_timer_stop (m_loop, watcher);
    ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
    ev_timer_start (m_loop, watcher);
    return(true);
}

Session* ThunderWorker::GetSession(uint64 uiSessionId, const std::string& strSessionClass)
{
    std::map<std::string, std::map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(strSessionClass);
    if (name_iter == m_mapCallbackSession.end())
    {
        return(NULL);
    }
    else
    {
        char szSession[32] = {0};
        snprintf(szSession, sizeof(szSession), "%llu", uiSessionId);
        std::map<std::string, Session*>::iterator id_iter = name_iter->second.find(szSession);
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

Session* ThunderWorker::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    std::map<std::string, std::map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(strSessionClass);
    if (name_iter == m_mapCallbackSession.end())
    {
        return(NULL);
    }
    else
    {
        std::map<std::string, Session*>::iterator id_iter = name_iter->second.find(strSessionId);
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

//bool ThunderWorker::RegisterCallback(Session* pSession)
//{
//    if (pSession == NULL)
//    {
//        return(false);
//    }
//    pSession->SetWorker(this);
//    pSession->SetLogger(m_oLogger);
//    pSession->SetRegistered();
//    ev_timer* timeout_watcher = new ev_timer();
//    if (timeout_watcher == NULL)
//    {
//        return(false);
//    }
//    uint32* pUlSeq = new uint32;
//    if (pUlSeq == NULL)
//    {
//        delete timeout_watcher;
//        return(false);
//    }
//    ev_timer_init (timeout_watcher, StepTimeoutCallback, 60 + ev_time() - ev_now(m_loop), 0.);
//    *pUllSeq = pSession->GetSequence();
//    timeout_watcher->data = (void*)pUllSeq;
//    ev_timer_start (m_loop, timeout_watcher);
//
//    std::pair<std::map<uint32, Session*>::iterator, bool> ret
//        = m_mapCallbackSession.insert(std::pair<uint32, Session*>(pSession->GetSequence(), pSession));
//    return(ret.second);
//}
//
//void ThunderWorker::DeleteCallback(Session* pSession)
//{
//    if (pSession == NULL)
//    {
//        return;
//    }
//    std::map<uint32, Session*>::iterator iter = m_mapCallbackSession.find(pSession->GetSequence());
//    if (iter != m_mapCallbackSession.end())
//    {
//        delete iter->second;
//        iter->second = NULL;
//        m_mapCallbackSession.erase(iter);
//    }
//}

bool ThunderWorker::Disconnect(const MsgShell& stMsgShell, bool bMsgShellNotice)
{
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
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

bool ThunderWorker::Disconnect(const std::string& strIdentify, bool bMsgShellNotice)
{
    std::map<std::string, MsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        return(true);
    }
    else
    {
        return(Disconnect(shell_iter->second, bMsgShellNotice));
    }
}

bool ThunderWorker::SetProcessName(const thunder::CJsonObject& oJsonConf)
{
    char szProcessName[64] = {0};
    snprintf(szProcessName, sizeof(szProcessName), "%s_W%d", oJsonConf("server_name").c_str(), m_iWorkerIndex);
    ngx_setproctitle(szProcessName);
    return(true);
}

bool ThunderWorker::Init(thunder::CJsonObject& oJsonConf)
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
#ifdef NODE_TYPE_GATE
    oJsonConf["permission"]["uin_permit"].Get("stat_interval", m_dMsgStatInterval);
    oJsonConf["permission"]["uin_permit"].Get("permit_num", m_iMsgPermitNum);
#endif
    if (!InitLogger(oJsonConf))
    {
        return(false);
    }
    ThunderCodec* pCodec = new ProtoCodec(thunder::CODEC_PROTOBUF);
    pCodec->SetLogger(m_oLogger);
    m_mapCodec.insert(std::pair<thunder::E_CODEC_TYPE, ThunderCodec*>(thunder::CODEC_PROTOBUF, pCodec));
    pCodec = new HttpCodec(thunder::CODEC_HTTP);
    pCodec->SetLogger(m_oLogger);
    m_mapCodec.insert(std::pair<thunder::E_CODEC_TYPE, ThunderCodec*>(thunder::CODEC_HTTP, pCodec));
    pCodec = new CustomMsgCodec(thunder::CODEC_PRIVATE);
    pCodec->SetLogger(m_oLogger);
    m_mapCodec.insert(std::pair<thunder::E_CODEC_TYPE, ThunderCodec*>(thunder::CODEC_PRIVATE, pCodec));
    pCodec = new CodecWebSocketJson(thunder::CODEC_WEBSOCKET_EX_JS);
    pCodec->SetLogger(m_oLogger);
    m_mapCodec.insert(std::pair<thunder::E_CODEC_TYPE, ThunderCodec*>(thunder::CODEC_WEBSOCKET_EX_JS, pCodec));
    pCodec = new CodecWebSocketPb(thunder::CODEC_WEBSOCKET_EX_PB);
    pCodec->SetLogger(m_oLogger);
    m_mapCodec.insert(std::pair<thunder::E_CODEC_TYPE, ThunderCodec*>(thunder::CODEC_WEBSOCKET_EX_PB, pCodec));
    m_pCmdConnect = new CmdConnectWorker();
    if (m_pCmdConnect == NULL)
    {
        return(false);
    }
    m_pCmdConnect->SetLogger(&m_oLogger);
    m_pCmdConnect->SetLabor(this);
    return(true);
}

bool ThunderWorker::InitLogger(const thunder::CJsonObject& oJsonConf)
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
        std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(strParttern));
        log4cplus::SharedAppenderPtr append(new log4cplus::RollingFileAppender(
                        strLogname, iMaxLogFileSize, iMaxLogFileNum));
        append->setName(strLogname);
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

bool ThunderWorker::CreateEvents()
{
    m_loop = ev_loop_new(EVFLAG_AUTO);
    if (m_loop == NULL)
    {
        return(false);
    }

    signal(SIGPIPE, SIG_IGN);
    // 注册信号事件
    ev_signal* signal_watcher = new ev_signal();
    ev_signal_init (signal_watcher, TerminatedCallback, SIGINT);
    signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, signal_watcher);

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
        MsgShell stMsgShell;
        stMsgShell.iFd = m_iManagerControlFd;
        stMsgShell.ulSeq = ulSeq;
        std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(m_iManagerControlFd);
        if (!AddIoReadEvent(iter))
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
        MsgShell stMsgShell;
        stMsgShell.iFd = m_iManagerDataFd;
        stMsgShell.ulSeq = ulSeq;
        std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(m_iManagerDataFd);
        if (!AddIoReadEvent(iter))
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

void ThunderWorker::PreloadCmd()
{
    Cmd* pCmdToldWorker = new CmdToldWorker();
    pCmdToldWorker->SetCmd(CMD_REQ_TELL_WORKER);
    pCmdToldWorker->SetLogger(&m_oLogger);
    pCmdToldWorker->SetLabor(this);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdToldWorker->GetCmd(), pCmdToldWorker));

    Cmd* pCmdUpdateNodeId = new CmdUpdateNodeId();
    pCmdUpdateNodeId->SetCmd(CMD_REQ_REFRESH_NODE_ID);
    pCmdUpdateNodeId->SetLogger(&m_oLogger);
    pCmdUpdateNodeId->SetLabor(this);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdUpdateNodeId->GetCmd(), pCmdUpdateNodeId));

    Cmd* pCmdNodeNotice = new CmdNodeNotice();
    pCmdNodeNotice->SetCmd(CMD_REQ_NODE_REG_NOTICE);
    pCmdNodeNotice->SetLogger(&m_oLogger);
    pCmdNodeNotice->SetLabor(this);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdNodeNotice->GetCmd(), pCmdNodeNotice));

    Cmd* pCmdBeat = new CmdBeat();
    pCmdBeat->SetCmd(CMD_REQ_BEAT);
    pCmdBeat->SetLogger(&m_oLogger);
    pCmdBeat->SetLabor(this);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdBeat->GetCmd(), pCmdBeat));

    Cmd* pCmdUpdateConfig = new CmdUpdateConfig();
    pCmdUpdateConfig->SetCmd(CMD_REQ_SERVER_CONFIG);
    pCmdUpdateConfig->SetLogger(&m_oLogger);
    pCmdUpdateConfig->SetLabor(this);
    pCmdUpdateConfig->SetConfigPath(m_strWorkPath);
    m_mapCmd.insert(std::pair<int32, Cmd*>(pCmdUpdateConfig->GetCmd(), pCmdUpdateConfig));
}

void ThunderWorker::Destroy()
{
    LOG4_TRACE("%s()", __FUNCTION__);

    m_mapHttpAttr.clear();

    for (std::map<int32, Cmd*>::iterator cmd_iter = m_mapCmd.begin();
                    cmd_iter != m_mapCmd.end(); ++cmd_iter)
    {
        delete cmd_iter->second;
        cmd_iter->second = NULL;
    }
    m_mapCmd.clear();

    for (std::map<int, tagSo*>::iterator so_iter = m_mapSo.begin();
                    so_iter != m_mapSo.end(); ++so_iter)
    {
        delete so_iter->second;
        so_iter->second = NULL;
    }
    m_mapSo.clear();

    for (std::map<std::string, tagModule*>::iterator module_iter = m_mapModule.begin();
                    module_iter != m_mapModule.end(); ++module_iter)
    {
        delete module_iter->second;
        module_iter->second = NULL;
    }
    m_mapModule.clear();

    for (std::map<int, tagConnectionAttr*>::iterator attr_iter = m_mapFdAttr.begin();
                    attr_iter != m_mapFdAttr.end(); ++attr_iter)
    {
        LOG4_TRACE("for (std::map<int, tagConnectionAttr*>::iterator attr_iter = m_mapFdAttr.begin();");
        DestroyConnect(attr_iter);
    }

    for (std::map<thunder::E_CODEC_TYPE, ThunderCodec*>::iterator codec_iter = m_mapCodec.begin();
                    codec_iter != m_mapCodec.end(); ++codec_iter)
    {
        delete codec_iter->second;
        codec_iter->second = NULL;
    }
    m_mapCodec.clear();

    if (m_loop != NULL)
    {
        ev_loop_destroy(m_loop);
        m_loop = NULL;
    }

    if (m_pErrBuff != NULL)
    {
        delete[] m_pErrBuff;
        m_pErrBuff = NULL;
    }
}

void ThunderWorker::ResetLogLevel(log4cplus::LogLevel iLogLevel)
{
    m_oLogger.setLogLevel(iLogLevel);
}

bool ThunderWorker::AddMsgShell(const std::string& strIdentify, const MsgShell& stMsgShell)
{
    LOG4_TRACE("%s(%s, fd %d, seq %u)", __FUNCTION__, strIdentify.c_str(), stMsgShell.iFd, stMsgShell.ulSeq);
    std::map<std::string, MsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        m_mapMsgShell.insert(std::pair<std::string, MsgShell>(strIdentify, stMsgShell));
    }
    else
    {
        if ((stMsgShell.iFd != shell_iter->second.iFd || stMsgShell.ulSeq != shell_iter->second.ulSeq))
        {
            LOG4_DEBUG("%s() connect to %s was exist, replace old fd %d with new fd %d",
                            __FUNCTION__,strIdentify.c_str(), shell_iter->second.iFd, stMsgShell.iFd);
            std::map<int, tagConnectionAttr*>::iterator fd_iter = m_mapFdAttr.find(shell_iter->second.iFd);
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

void ThunderWorker::DelMsgShell(const std::string& strIdentify,const MsgShell& stMsgShell)
{
    std::map<std::string, MsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
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
//    std::map<std::string, std::string>::iterator identify_iter = m_mapIdentifyNodeType.find(strIdentify);
//    if (identify_iter != m_mapIdentifyNodeType.end())
//    {
//        std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
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

void ThunderWorker::AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s(%s, %s)", __FUNCTION__, strNodeType.c_str(), strIdentify.c_str());
    std::map<std::string, std::string>::iterator iter = m_mapIdentifyNodeType.find(strIdentify);
    if (iter == m_mapIdentifyNodeType.end())
    {
        m_mapIdentifyNodeType.insert(iter,
                std::pair<std::string, std::string>(strIdentify, strNodeType));

        T_MAP_NODE_TYPE_IDENTIFY::iterator node_type_iter;
        node_type_iter = m_mapNodeIdentify.find(strNodeType);
        if (node_type_iter == m_mapNodeIdentify.end())
        {
            std::set<std::string> setIdentify;
            setIdentify.insert(strIdentify);
            std::pair<T_MAP_NODE_TYPE_IDENTIFY::iterator, bool> insert_node_result;
            insert_node_result = m_mapNodeIdentify.insert(std::pair< std::string,
                            std::pair<std::set<std::string>::iterator, std::set<std::string> > >(
                                            strNodeType, std::make_pair(setIdentify.begin(), setIdentify)));    //这里的setIdentify是临时变量，setIdentify.begin()将会成非法地址
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
}

void ThunderWorker::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s(%s, %s)", __FUNCTION__, strNodeType.c_str(), strIdentify.c_str());
    std::map<std::string, std::string>::iterator identify_iter = m_mapIdentifyNodeType.find(strIdentify);
    if (identify_iter != m_mapIdentifyNodeType.end())
    {
        std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
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
}

void ThunderWorker::GetNodeIdentifys(const std::string& strNodeType, std::vector<std::string>& strIdentifys)
{
    strIdentifys.clear();
    //std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >
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

bool ThunderWorker::RegisterCallback(const std::string& strIdentify, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, strIdentify.c_str());
    int iPosIpPortSeparator = strIdentify.find(':');
    if (iPosIpPortSeparator == std::string::npos)
    {
        return(false);
    }
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    if (iPort == 0)
    {
        return(false);
    }
    std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(strIdentify);
    if (ctx_iter != m_mapRedisContext.end())
    {
        LOG4_DEBUG("redis context %s", strIdentify.c_str());
        return(RegisterCallback(ctx_iter->second, pRedisStep));
    }
    else
    {
        LOG4_DEBUG("GetLabor()->AutoRedisCmd(%s, %d)", strHost.c_str(), iPort);
        return(AutoRedisCmd(strHost, iPort, pRedisStep));
    }
}

bool ThunderWorker::RegisterCallback(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s(%s, %d)", __FUNCTION__, strHost.c_str(), iPort);
    char szIdentify[32] = {0};
    snprintf(szIdentify, sizeof(szIdentify), "%s:%d", strHost.c_str(), iPort);
    std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(szIdentify);
    if (ctx_iter != m_mapRedisContext.end())
    {
        LOG4_TRACE("redis context %s", szIdentify);
        return(RegisterCallback(ctx_iter->second, pRedisStep));
    }
    else
    {
        LOG4_TRACE("GetLabor()->AutoRedisCmd(%s, %d)", strHost.c_str(), iPort);
        return(AutoRedisCmd(strHost, iPort, pRedisStep));
    }
}

bool ThunderWorker::AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx)
{
    LOG4_TRACE("%s(%s, %d, 0x%X)", __FUNCTION__, strHost.c_str(), iPort, ctx);
    char szIdentify[32] = {0};
    snprintf(szIdentify, 32, "%s:%d", strHost.c_str(), iPort);
    std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(szIdentify);
    if (ctx_iter == m_mapRedisContext.end())
    {
        m_mapRedisContext.insert(std::pair<std::string, const redisAsyncContext*>(szIdentify, ctx));
        std::map<const redisAsyncContext*, std::string>::iterator identify_iter = m_mapContextIdentify.find(ctx);
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

void ThunderWorker::DelRedisContextAddr(const redisAsyncContext* ctx)
{
    std::map<const redisAsyncContext*, std::string>::iterator identify_iter = m_mapContextIdentify.find(ctx);
    if (identify_iter != m_mapContextIdentify.end())
    {
        std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(identify_iter->second);
        if (ctx_iter != m_mapRedisContext.end())
        {
            m_mapRedisContext.erase(ctx_iter);
        }
        m_mapContextIdentify.erase(identify_iter);
    }
}

bool ThunderWorker::SendTo(const MsgShell& stMsgShell)
{
    LOG4_TRACE("%s(fd %d, seq %lu) pWaitForSendBuff", __FUNCTION__, stMsgShell.iFd, stMsgShell.ulSeq);
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd %d found in m_mapFdAttr", stMsgShell.iFd);
        return(false);
    }
    else
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
            int iErrno = 0;
            int iWriteLen = 0;
            int iNeedWriteLen = (int)(iter->second->pWaitForSendBuff->ReadableBytes());
            int iWriteIdx = iter->second->pSendBuff->GetWriteIndex();
            iWriteLen = iter->second->pSendBuff->Write(iter->second->pWaitForSendBuff, iter->second->pWaitForSendBuff->ReadableBytes());
            if (iWriteLen == iNeedWriteLen)
            {
                iNeedWriteLen = (int)iter->second->pSendBuff->ReadableBytes();
                iWriteLen = iter->second->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
                iter->second->pSendBuff->Compact(8192);
                if (iWriteLen < 0)
                {
                    if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                    {
                        LOG4_ERROR("send to fd %d error %d: %s",
                                        stMsgShell.iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        DestroyConnect(iter);
                        return(false);
                    }
                    else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                    {
                        iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(iter);
                    }
                }
                else if (iWriteLen > 0)
                {
                    m_iSendByte += iWriteLen;
                    iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        RemoveIoWriteEvent(iter);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        AddIoWriteEvent(iter);
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
    }
    return(false);
}

bool ThunderWorker::ParseFromMsg(const MsgBody& oInMsgBody,google::protobuf::Message &message)
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

bool ThunderWorker::SendToClient(const MsgShell& stMsgShell,MsgHead& oMsgHead,const google::protobuf::Message &message,
                const std::string& additional,uint64 sessionid,const std::string& strSession)
{
    MsgBody oMsgBody;
    {
        std::string strJson;
        google::protobuf::util::JsonPrintOptions oOption;
        google::protobuf::util::Status oStatus = google::protobuf::util::MessageToJsonString(message,&strJson,oOption);
        if(!oStatus.ok())
        {
            LOG4_ERROR("MessageToJsonString failed error(%s) message(%s) message Descriptor(%s)",
                            oStatus.ToString().c_str(),
                            message.DebugString().c_str(),
                            message.GetDescriptor()->DebugString().c_str());
            return false;
        }
        oMsgBody.set_sbody(strJson);
    }
    oMsgBody.set_body(message.SerializeAsString());
    if(additional.size() > 0)
    {
        oMsgBody.set_additional(additional);
    }
    if(sessionid > 0)
    {
        oMsgBody.set_session_id(sessionid);
    }
    if(strSession.size() > 0)
    {
        oMsgBody.set_session(strSession);
    }
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    return SendTo(stMsgShell,oMsgHead,oMsgBody);
}

bool ThunderWorker::SendToClient(const std::string& strIdentify,MsgHead& oMsgHead,const google::protobuf::Message &message,
                const std::string& additional,uint64 sessionid,const std::string& strSession)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    MsgBody oMsgBody;
    {
        std::string strJson;
        google::protobuf::util::JsonPrintOptions oOption;
        google::protobuf::util::Status oStatus = google::protobuf::util::MessageToJsonString(message,&strJson,oOption);
        if(!oStatus.ok())
        {
            LOG4_ERROR("MessageToJsonString failed error(%u,%s)",
                            oStatus.error_code(),oStatus.error_message().ToString().c_str());
            return false;
        }
        oMsgBody.set_sbody(strJson);
    }
    oMsgBody.set_body(message.SerializeAsString());
    if(additional.size() > 0)
    {
        oMsgBody.set_additional(additional);
    }
    if(sessionid > 0)
    {
        oMsgBody.set_session_id(sessionid);
    }
    if(strSession.size() > 0)
    {
        oMsgBody.set_session(strSession);
    }
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());

    std::map<std::string, MsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        LOG4_TRACE("no MsgShell match %s.", strIdentify.c_str());
        return(AutoSend(strIdentify, oMsgHead, oMsgBody));
    }
    else
    {
        return(SendTo(shell_iter->second, oMsgHead, oMsgBody));
    }
}

bool ThunderWorker::BuildClientMsg(MsgHead& oMsgHead,MsgBody &oMsgBody,const google::protobuf::Message &message,
                            const std::string& additional,uint64 sessionid,const std::string& strSession)
{
    oMsgBody.Clear();
    {
        std::string strJson;
        google::protobuf::util::JsonPrintOptions oOption;
        google::protobuf::util::Status oStatus = google::protobuf::util::MessageToJsonString(message,&strJson,oOption);
        if(!oStatus.ok())
        {
            LOG4_ERROR("MessageToJsonString failed error(%u,%s)",
                            oStatus.error_code(),oStatus.error_message().ToString().c_str());
            return false;
        }
        oMsgBody.set_sbody(strJson);
    }
    oMsgBody.set_body(message.SerializeAsString());
    if(additional.size() > 0)
    {
        oMsgBody.set_additional(additional);
    }
    if(sessionid > 0)
    {
        oMsgBody.set_session_id(sessionid);
    }
    if(strSession.size() > 0)
    {
        oMsgBody.set_session(strSession);
    }
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    return true;
}

bool ThunderWorker::EmitStorageAccess(thunder::Session* pSession,const std::string &strMsgSerial,
		StorageCallbackSession callback,bool boPermanentSession,const std::string &nodeType,uint32 uiCmd)
{
	StepNodeAccess* pStep = new StepNodeAccess(strMsgSerial);
    if (pStep == NULL)
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),"new StepNodeAccess() error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (!RegisterCallback(pStep))
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),"RegisterCallback(pStep) error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (thunder::STATUS_CMD_RUNNING != pStep->Emit(ERR_OK))
    {
        DeleteCallback(pStep);
        return(false);
    }
    pStep->SetStorageCallBack(callback,pSession,boPermanentSession,nodeType,uiCmd);
    return true;
}

bool ThunderWorker::EmitStorageAccess(thunder::Step* pUpperStep,const std::string &strMsgSerial,
		StorageCallbackStep callback,const std::string &nodeType,uint32 uiCmd)
{
	StepNodeAccess* pStep = new StepNodeAccess(strMsgSerial);
    if (pStep == NULL)
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),"new StepStorageAccess() error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (!RegisterCallback(pStep))
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),"RegisterCallback(pStep) error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (thunder::STATUS_CMD_RUNNING != pStep->Emit(ERR_OK))
    {
        DeleteCallback(pStep);
        return(false);
    }
    pStep->SetStorageCallBack(callback,pUpperStep,nodeType,uiCmd);
    pUpperStep->AddNextStepSeq(pStep);
    return true;
}

bool ThunderWorker::EmitStandardAccess(thunder::Session* pSession,const std::string &strMsgSerial,
		StandardCallbackSession callback,bool boPermanentSession,const std::string &nodeType,uint32 uiCmd)
{
	StepNodeAccess* pStep = new StepNodeAccess(strMsgSerial);
    if (pStep == NULL)
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),"new StepNodeAccess() error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (!RegisterCallback(pStep))
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),"RegisterCallback(pStep) error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (thunder::STATUS_CMD_RUNNING != pStep->Emit(ERR_OK))
    {
        DeleteCallback(pStep);
        return(false);
    }
    pStep->SetStandardCallBack(callback,pSession,boPermanentSession,nodeType,uiCmd);
    return true;
}

bool ThunderWorker::EmitStandardAccess(thunder::Step* pUpperStep,const std::string &strMsgSerial,
		StandardCallbackStep callback,const std::string &nodeType,uint32 uiCmd)
{
	StepNodeAccess* pStep = new StepNodeAccess(strMsgSerial);
    if (pStep == NULL)
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),"new StepStorageAccess() error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (!RegisterCallback(pStep))
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),"RegisterCallback(pStep) error!");
        delete pStep;
        pStep = NULL;
        return(false);
    }
    if (thunder::STATUS_CMD_RUNNING != pStep->Emit(ERR_OK))
    {
        DeleteCallback(pStep);
        return(false);
    }
    pStep->SetStandardCallBack(callback,pUpperStep,nodeType,uiCmd);
    pUpperStep->AddNextStepSeq(pStep);
    return true;
}

bool ThunderWorker::SendTo(const MsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(fd %d, fd_seq %lu, cmd %u, msg_seq %u)",
                    __FUNCTION__, stMsgShell.iFd, stMsgShell.ulSeq, oMsgHead.cmd(), oMsgHead.seq());
    std::map<int, tagConnectionAttr*>::iterator conn_iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (conn_iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd %d found in m_mapFdAttr", stMsgShell.iFd);
        return(false);
    }
    else
    {
        if (conn_iter->second->ulSeq == stMsgShell.ulSeq)
        {
            std::map<thunder::E_CODEC_TYPE, ThunderCodec*>::iterator codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", conn_iter->second->eCodecType);
                DestroyConnect(conn_iter);
                return(false);
            }
            E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
            if (thunder::CODEC_PROTOBUF == conn_iter->second->eCodecType)//内部协议需要检查连接过程
            {
                LOG4_TRACE("connect status %u", conn_iter->second->ucConnectStatus);
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
                if (NULL == conn_iter->second->pRemoteAddr)
                {
                    LOG4_TRACE("try send cmd[%d] seq[%lu] len %d to fd %d, identify %s",
                                    oMsgHead.cmd(), oMsgHead.seq(), iNeedWriteLen, stMsgShell.iFd,
                                    conn_iter->second->strIdentify.c_str());
                }
                else
                {
                    LOG4_TRACE("try send cmd[%d] seq[%lu] len %d to fd %d ip %s identify %s",
                                oMsgHead.cmd(), oMsgHead.seq(), iNeedWriteLen, stMsgShell.iFd,
                                conn_iter->second->pRemoteAddr, conn_iter->second->strIdentify.c_str());
                }
                int iWriteLen = conn_iter->second->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
                conn_iter->second->pSendBuff->Compact(8192);
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
                        conn_iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(conn_iter);
                    }
                    else
                    {
                        LOG4_TRACE("write len %d, errno %d: %s",
                                        iWriteLen, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        conn_iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(conn_iter);
                    }
                }
                else if (iWriteLen > 0)
                {
                    m_iSendByte += iWriteLen;
                    conn_iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        LOG4_TRACE("cmd[%d] seq[%lu] to fd %d ip %s identify %s need write %d and had written len %d",
                                        oMsgHead.cmd(), oMsgHead.seq(), stMsgShell.iFd,
                                        conn_iter->second->pRemoteAddr, conn_iter->second->strIdentify.c_str(),
                                        iNeedWriteLen, iWriteLen);
                        RemoveIoWriteEvent(conn_iter);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        LOG4_TRACE("cmd[%d] seq[%lu] need write %d and had written len %d",
                                        oMsgHead.cmd(), oMsgHead.seq(), iNeedWriteLen, iWriteLen);
                        AddIoWriteEvent(conn_iter);
                    }
                }
                return(true);
            }
            else
            {
                LOG4_WARN("codec_iter->second->Encode failed,oMsgHead.cmd(%u),connect status %u",
                                                                            oMsgHead.cmd(),conn_iter->second->ucConnectStatus);
                return(false);
            }
        }
        else
        {
            LOG4_ERROR("fd %d sequence %lu not match the sequence %lu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second->ulSeq);
            return(false);
        }
    }
}

bool ThunderWorker::SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::map<std::string, MsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        LOG4_TRACE("no MsgShell match %s.", strIdentify.c_str());
        return(AutoSend(strIdentify, oMsgHead, oMsgBody));
    }
    else
    {
        return(SendTo(shell_iter->second, oMsgHead, oMsgBody));
    }
}

bool ThunderWorker::SendToNext(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(node_type: %s)", __FUNCTION__, strNodeType.c_str());
    std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no MsgShell match %s!", strNodeType.c_str());
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
                LOG4_ERROR("no MsgShell match and no node identify found for %s!", strNodeType.c_str());
                return(false);
            }
        }
    }
}

bool ThunderWorker::SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(nody_type: %s, mod_factor: %u)", __FUNCTION__, strNodeType.c_str(), uiModFactor);
    std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no MsgShell match %s!", strNodeType.c_str());
        return(false);
    }
    else
    {
        if (node_type_iter->second.second.size() == 0)
        {
            LOG4_ERROR("no MsgShell match %s!", strNodeType.c_str());
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
                    return(SendTo(*id_iter, oMsgHead, oMsgBody));
                }
            }
            return(false);
        }
    }
}

bool ThunderWorker::SendToNodeType(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(node_type: %s)", __FUNCTION__, strNodeType.c_str());
    std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no MsgShell match %s!", strNodeType.c_str());
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
            LOG4_ERROR("no MsgShell match and no node identify found for %s!", strNodeType.c_str());
            return(false);
        }
    }
    return(true);
}

bool ThunderWorker::SendTo(const MsgShell& stMsgShell, const HttpMsg& oHttpMsg, HttpStep* pHttpStep)
{
    LOG4_TRACE("%s(fd %d, seq %lu)", __FUNCTION__, stMsgShell.iFd, stMsgShell.ulSeq);
    std::map<int, tagConnectionAttr*>::iterator conn_iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (conn_iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd %d found in m_mapFdAttr", stMsgShell.iFd);
        return(false);
    }
    else
    {
        if (conn_iter->second->ulSeq == stMsgShell.ulSeq)
        {
            std::map<thunder::E_CODEC_TYPE, ThunderCodec*>::iterator codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", conn_iter->second->eCodecType);
                DestroyConnect(conn_iter);
                return(false);
            }
            E_CODEC_STATUS eCodecStatus;
            if(thunder::CODEC_WEBSOCKET_EX_PB == conn_iter->second->eCodecType)
            {
                if (conn_iter->second->pWaitForSendBuff->ReadableBytes() > 0)   // 正在连接
                {
                    eCodecStatus = ((CodecWebSocketPb*)codec_iter->second)->Encode(oHttpMsg, conn_iter->second->pWaitForSendBuff);
                }
                else
                {
                    eCodecStatus = ((CodecWebSocketPb*)codec_iter->second)->Encode(oHttpMsg, conn_iter->second->pSendBuff);
                }
            }
            else if(thunder::CODEC_WEBSOCKET_EX_JS == conn_iter->second->eCodecType)
            {
                if (conn_iter->second->pWaitForSendBuff->ReadableBytes() > 0)   // 正在连接
                {
                    eCodecStatus = ((CodecWebSocketJson*)codec_iter->second)->Encode(oHttpMsg, conn_iter->second->pWaitForSendBuff);
                }
                else
                {
                    eCodecStatus = ((CodecWebSocketJson*)codec_iter->second)->Encode(oHttpMsg, conn_iter->second->pSendBuff);
                }
            }
            else if (thunder::CODEC_HTTP == conn_iter->second->eCodecType)
            {
                if (conn_iter->second->pWaitForSendBuff->ReadableBytes() > 0)   // 正在连接
                {
                    eCodecStatus = ((HttpCodec*)codec_iter->second)->Encode(oHttpMsg, conn_iter->second->pWaitForSendBuff);
                }
                else
                {
                    eCodecStatus = ((HttpCodec*)codec_iter->second)->Encode(oHttpMsg, conn_iter->second->pSendBuff);
                }
            }
            else
            {
                LOG4_ERROR("the codec for fd %d is not http or websocket codec(%d)!",
                                                stMsgShell.iFd,conn_iter->second->eCodecType);
                return(false);
            }

            if (CODEC_STATUS_OK == eCodecStatus && conn_iter->second->pSendBuff->ReadableBytes() > 0)
            {
                ++m_iSendNum;
                if ((conn_iter->second->pIoWatcher != NULL) && (conn_iter->second->pIoWatcher->events & EV_WRITE))
                {   // 正在监听fd的写事件，说明发送缓冲区满，此时直接返回，等待EV_WRITE事件再执行WriteFD
                    return(true);
                }
                LOG4_TRACE("fd[%d], seq[%u], conn_iter->second->pSendBuff 0x%x", stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second->pSendBuff);
                int iErrno = 0;
                int iNeedWriteLen = (int)conn_iter->second->pSendBuff->ReadableBytes();
                int iWriteLen = conn_iter->second->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
                conn_iter->second->pSendBuff->Compact(8192);
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
                        LOG4_TRACE("write len %d, error %d: %s",
                                        iWriteLen, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        conn_iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(conn_iter);
                    }
                    else
                    {
                        LOG4_TRACE("write len %d, error %d: %s",
                                        iWriteLen, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                        conn_iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(conn_iter);
                    }
                }
                else if (iWriteLen > 0)
                {
                    if (pHttpStep != NULL)
                    {
                        std::map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
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
                        std::map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
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
                        RemoveIoWriteEvent(conn_iter);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        AddIoWriteEvent(conn_iter);
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
            LOG4_ERROR("fd %d sequence %lu not match the sequence %lu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second->ulSeq);
            return(false);
        }
    }
}

bool ThunderWorker::SentTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, HttpStep* pHttpStep)
{
    char szIdentify[256] = {0};
    snprintf(szIdentify, sizeof(szIdentify), "%s:%d%s", strHost.c_str(), iPort, strUrlPath.c_str());
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, szIdentify);
    return(AutoSend(strHost, iPort, strUrlPath, oHttpMsg, pHttpStep));
    // 向外部发起http请求不复用连接
//    std::map<std::string, MsgShell>::iterator shell_iter = m_mapMsgShell.find(szIdentify);
//    if (shell_iter == m_mapMsgShell.end())
//    {
//        LOG4_TRACE("no MsgShell match %s.", szIdentify);
//        return(AutoSend(strHost, iPort, strUrlPath, oHttpMsg, pHttpStep));
//    }
//    else
//    {
//        return(SendTo(shell_iter->second, oHttpMsg, pHttpStep));
//    }
}

bool ThunderWorker::SetConnectIdentify(const MsgShell& stMsgShell, const std::string& strIdentify)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (stMsgShell.iFd == 0 || strIdentify.size() == 0)
    {
        LOG4_WARN("%s() stMsgShell.iFd(%u) == 0 || strIdentify.size(%u) == 0",
                        __FUNCTION__,stMsgShell.iFd,strIdentify.size());
        return(false);
    }
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
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

bool ThunderWorker::AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
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
    x_sock_set_block(iFd, 0);
    int nREUSEADDR = 1;
    setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    uint32 ulSeq = GetSequence();
    if (CreateFdAttr(iFd, ulSeq))
    {
        std::map<int, tagConnectionAttr*>::iterator conn_iter =  m_mapFdAttr.find(iFd);
        snprintf(conn_iter->second->pRemoteAddr, 32, strIdentify.c_str());
        if(AddIoTimeout(iFd, ulSeq, conn_iter->second, 1.5))
        {
            conn_iter->second->ucConnectStatus = 0;
            if (!AddIoReadEvent(conn_iter))
            {
                LOG4_TRACE("if (!AddIoReadEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
//            if (!AddIoErrorEvent(iFd))
//            {
//                DestroyConnect(iter);
//                return(false);
//            }
            if (!AddIoWriteEvent(conn_iter))
            {
                LOG4_TRACE("if (!AddIoWriteEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
            std::map<thunder::E_CODEC_TYPE, ThunderCodec*>::iterator codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
            if (codec_iter == m_mapCodec.end())
            {
                LOG4_ERROR("no codec found for %d!", conn_iter->second->eCodecType);
                DestroyConnect(conn_iter);
                return(false);
            }
            E_CODEC_STATUS eCodecStatus = codec_iter->second->Encode(oMsgHead, oMsgBody, conn_iter->second->pWaitForSendBuff);
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
            MsgShell stMsgShell;
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

bool ThunderWorker::AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, HttpStep* pHttpStep)
{
    LOG4_TRACE("%s(%s, %d, %s)", __FUNCTION__, strHost.c_str(), iPort, strUrlPath.c_str());
    struct sockaddr_in stAddr;
    MsgShell stMsgShell;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    if (stAddr.sin_addr.s_addr == 4294967295 || stAddr.sin_addr.s_addr == 0)
    {
        struct hostent *he;
        he = gethostbyname(strHost.c_str());
        if (he != NULL)
        {
            stAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)(he->h_addr)));
        }
        else
        {
            LOG4_ERROR("gethostbyname(%s) error!", strHost.c_str());
            return(false);
        }
    }
    bzero(&(stAddr.sin_zero), 8);
    stMsgShell.iFd = socket(AF_INET, SOCK_STREAM, 0);
    if (stMsgShell.iFd == -1)
    {
        return(false);
    }
    x_sock_set_block(stMsgShell.iFd, 0);
    int nREUSEADDR = 1;
    setsockopt(stMsgShell.iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    stMsgShell.ulSeq = GetSequence();
    tagConnectionAttr* pConnAttr = CreateFdAttr(stMsgShell.iFd, stMsgShell.ulSeq);
    if (pConnAttr)
    {
        pConnAttr->eCodecType = thunder::CODEC_HTTP;
        snprintf(pConnAttr->pRemoteAddr, 32, strHost.c_str());
        std::map<int, tagConnectionAttr*>::iterator conn_iter =  m_mapFdAttr.find(stMsgShell.iFd);
        if(AddIoTimeout(stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second, 2.5))
        {
            conn_iter->second->dKeepAlive = 10;
            LOG4_TRACE("set dKeepAlive(%lf)",conn_iter->second->dKeepAlive);
            if (!AddIoReadEvent(conn_iter))
            {
                LOG4_TRACE("if (!AddIoReadEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
            if (!AddIoWriteEvent(conn_iter))
            {
                LOG4_TRACE("if (!AddIoWriteEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
            std::map<thunder::E_CODEC_TYPE, ThunderCodec*>::iterator codec_iter = m_mapCodec.find(conn_iter->second->eCodecType);
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
                std::map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
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
                std::map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
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

bool ThunderWorker::AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s() redisAsyncConnect(%s, %d)", __FUNCTION__, strHost.c_str(), iPort);
    redisAsyncContext *c = redisAsyncConnect(strHost.c_str(), iPort);
    if (c->err)
    {
        /* Let *c leak for now... */
        LOG4_ERROR("error: %s", c->errstr);
        return(false);
    }
    c->data = this;
    tagRedisAttr* pRedisAttr = new tagRedisAttr();
    pRedisAttr->ulSeq = GetSequence();
    pRedisAttr->listWaitData.push_back(pRedisStep);
    pRedisStep->SetLogger(&m_oLogger);
    pRedisStep->SetLabor(this);
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

bool ThunderWorker::AutoConnect(const std::string& strIdentify)
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
    x_sock_set_block(iFd, 0);
    int nREUSEADDR = 1;
    setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    uint32 ulSeq = GetSequence();
    if (CreateFdAttr(iFd, ulSeq))
    {
        std::map<int, tagConnectionAttr*>::iterator conn_iter =  m_mapFdAttr.find(iFd);
        if(AddIoTimeout(iFd, ulSeq, conn_iter->second, 1.5))
        {
            conn_iter->second->ucConnectStatus = 0;
            if (!AddIoReadEvent(conn_iter))
            {
                LOG4_TRACE("if (!AddIoReadEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
//            if (!AddIoErrorEvent(iFd))
//            {
//                DestroyConnect(iter);
//                return(false);
//            }
            if (!AddIoWriteEvent(conn_iter))
            {
                LOG4_TRACE("if (!AddIoWriteEvent(conn_iter))");
                DestroyConnect(conn_iter);
                return(false);
            }
            m_mapSeq2WorkerIndex.insert(std::pair<uint32, int>(ulSeq, iWorkerIndex));
            MsgShell stMsgShell;
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

void ThunderWorker::AddInnerFd(const MsgShell& stMsgShell)
{
    std::map<int, uint32>::iterator iter = m_mapInnerFd.find(stMsgShell.iFd);
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

bool ThunderWorker::GetMsgShell(const std::string& strIdentify, MsgShell& stMsgShell)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::map<std::string, MsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        LOG4_DEBUG("no MsgShell match %s.", strIdentify.c_str());
        return(false);
    }
    else
    {
        stMsgShell = shell_iter->second;
        return(true);
    }
}

bool ThunderWorker::SetClientData(const MsgShell& stMsgShell, thunder::CBuffer* pBuff)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
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

bool ThunderWorker::HadClientData(const MsgShell& stMsgShell)
{
    std::map<int, tagConnectionAttr*>::iterator conn_iter;
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

bool ThunderWorker::GetClientData(const MsgShell& stMsgShell, thunder::CBuffer* pBuff)
{
	LOG4_TRACE("%s()", __FUNCTION__);
	std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
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


std::string ThunderWorker::GetClientAddr(const MsgShell& stMsgShell)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        return("");
    }
    else
    {
        if (iter->second->ulSeq == stMsgShell.ulSeq)
        {
            if (iter->second->pRemoteAddr == NULL)
            {
                return("");
            }
            else
            {
                return(iter->second->pRemoteAddr);
            }
        }
        else
        {
            return("");
        }
    }
}

std::string ThunderWorker::GetConnectIdentify(const MsgShell& stMsgShell)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
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

bool ThunderWorker::AbandonConnect(const std::string& strIdentify)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::map<std::string, MsgShell>::iterator shell_iter = m_mapMsgShell.find(strIdentify);
    if (shell_iter == m_mapMsgShell.end())
    {
        LOG4_DEBUG("no MsgShell match %s.", strIdentify.c_str());
        return(false);
    }
    else
    {
        std::map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(shell_iter->second.iFd);
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

void ThunderWorker::ExecStep(uint32 uiCallerStepSeq, uint32 uiCalledStepSeq,
                int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
    LOG4_TRACE("%s(caller[%u], called[%u])", __FUNCTION__, uiCallerStepSeq, uiCalledStepSeq);
    std::map<uint32, Step*>::iterator step_iter = m_mapCallbackStep.find(uiCalledStepSeq);
    if (step_iter == m_mapCallbackStep.end())
    {
        LOG4_WARN("step %u is not in the callback list.", uiCalledStepSeq);
    }
    else
    {
        if (thunder::STATUS_CMD_RUNNING != step_iter->second->Emit(iErrno, strErrMsg, strErrShow))
        {
            DeleteCallback(uiCallerStepSeq, step_iter->second);
            // 处理调用者step的NextStep
            step_iter = m_mapCallbackStep.find(uiCallerStepSeq);
            if (step_iter != m_mapCallbackStep.end())
            {
                if (step_iter->second->m_pNextStep != NULL
                                && step_iter->second->m_pNextStep->GetSequence() == uiCalledStepSeq)
                {
                    step_iter->second->m_pNextStep = NULL;
                }
            }
        }
    }
}

void ThunderWorker::LoadSo(thunder::CJsonObject& oSoConf)
{
    LOG4_TRACE("%s():oSoConf(%s)", __FUNCTION__,oSoConf.ToString().c_str());
    int iCmd = 0;
    int iVersion = 0;
    bool bIsload = false;
    std::string strSoPath;
    std::map<int, tagSo*>::iterator cmd_iter;
    tagSo* pSo = NULL;
    for (int i = 0; i < oSoConf.GetArraySize(); ++i)
    {
        oSoConf[i].Get("load", bIsload);
        if (bIsload)
        {
            strSoPath = m_strWorkPath + std::string("/") + oSoConf[i]("so_path");
            if (oSoConf[i].Get("cmd", iCmd) && oSoConf[i].Get("version", iVersion))
            {
                cmd_iter = m_mapSo.find(iCmd);
                if (cmd_iter == m_mapSo.end())
                {
                    if (0 != access(strSoPath.c_str(), F_OK))
                    {
                        LOG4_WARN("%s not exist!", strSoPath.c_str());
                        continue;
                    }
                    pSo = LoadSoAndGetCmd(iCmd, strSoPath, oSoConf[i]("entrance_symbol"), iVersion);
                    if (pSo != NULL)
                    {
                        LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                        m_mapSo.insert(std::pair<int, tagSo*>(iCmd, pSo));
                    }
                    else
                    {
                        LOG4_WARN("failed to load %s", strSoPath.c_str());
                    }
                }
                else
                {
                    if (iVersion != cmd_iter->second->iVersion)
                    {
                        if (0 != access(strSoPath.c_str(), F_OK))
                        {
                            LOG4_WARN("%s not exist!", strSoPath.c_str());
                            continue;
                        }
                        pSo = LoadSoAndGetCmd(iCmd, strSoPath, oSoConf[i]("entrance_symbol"), iVersion);
                        LOG4_TRACE("%s:%d after LoadSoAndGetCmd", __FILE__, __LINE__);
                        if (pSo != NULL)
                        {
                            LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                            delete cmd_iter->second;
                            cmd_iter->second = pSo;
                        }
                        else
                        {
                            LOG4_WARN("failed to load %s", strSoPath.c_str());
                        }
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

void ThunderWorker::ReloadSo(thunder::CJsonObject& oCmds)
{
    LOG4_DEBUG("%s():oCmds(%s)", __FUNCTION__,oCmds.ToString().c_str());
    int iCmd = 0;
    int iVersion = 0;
    std::string strSoPath;
    std::string strSymbol;
    std::map<int, tagSo*>::iterator cmd_iter;
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
            pSo = LoadSoAndGetCmd(iCmd, strSoPath, strSymbol, iVersion);
            LOG4_DEBUG("%s:%d after LoadSoAndGetCmd", __FILE__, __LINE__);
            if (pSo != NULL)
            {
                LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                delete cmd_iter->second;
                cmd_iter->second = pSo;
            }
            else
            {
                LOG4_WARN("failed to load %s", strSoPath.c_str());
            }
        }
        else
        {
            LOG4_WARN("no such cmd %s", cmd.c_str());
        }
    }
}

tagSo* ThunderWorker::LoadSoAndGetCmd(int iCmd, const std::string& strSoPath, const std::string& strSymbol, int iVersion)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    tagSo* pSo = NULL;
    void* pHandle = NULL;
    pHandle = dlopen(strSoPath.c_str(), RTLD_NOW);
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
            pSo->pCmd->SetLogger(&m_oLogger);
            pSo->pCmd->SetLabor(this);
            pSo->pCmd->SetConfigPath(m_strWorkPath);
            pSo->pCmd->SetCmd(iCmd);
            if (!pSo->pCmd->Init())
            {
                LOG4_FATAL("Cmd %d %s init error",
                                iCmd, strSoPath.c_str());
                delete pSo;
                pSo = NULL;
            }
        }
        else
        {
            LOG4_FATAL("new tagSo() error!");
            delete pCmd;
            dlclose(pHandle);
        }
    }
    return(pSo);
}

void ThunderWorker::UnloadSoAndDeleteCmd(int iCmd)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagSo*>::iterator cmd_iter;
    cmd_iter = m_mapSo.find(iCmd);
    if (cmd_iter != m_mapSo.end())
    {
        delete cmd_iter->second;
        cmd_iter->second = NULL;
        m_mapSo.erase(cmd_iter);
    }
}

void ThunderWorker::LoadModule(thunder::CJsonObject& oModuleConf)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::string strModulePath;
    int iVersion = 0;
    bool bIsload = false;
    std::string strSoPath;
    std::map<std::string, tagModule*>::iterator module_iter;
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
                module_iter = m_mapModule.find(strModulePath);
                if (module_iter == m_mapModule.end())
                {
                    strSoPath = m_strWorkPath + std::string("/") + oModuleConf[i]("so_path");
                    if (0 != access(strSoPath.c_str(), F_OK))
                    {
                        LOG4_WARN("%s not exist!", strSoPath.c_str());
                        continue;
                    }
                    pModule = LoadSoAndGetModule(strModulePath, strSoPath, oModuleConf[i]("entrance_symbol"), iVersion);
                    if (pModule != NULL)
                    {
                        LOG4_INFO("succeed in loading %s with module path \"%s\".",
                                        strSoPath.c_str(), strModulePath.c_str());
                        m_mapModule.insert(std::pair<std::string, tagModule*>(strModulePath, pModule));
                    }
                }
                else
                {
                    if (iVersion != module_iter->second->iVersion)
                    {
                        strSoPath = m_strWorkPath + std::string("/") + oModuleConf[i]("so_path");
                        if (0 != access(strSoPath.c_str(), F_OK))
                        {
                            LOG4_WARN("%s not exist!", strSoPath.c_str());
                            continue;
                        }
                        pModule = LoadSoAndGetModule(strModulePath, strSoPath, oModuleConf[i]("entrance_symbol"), iVersion);
                        LOG4_TRACE("%s:%d after LoadSoAndGetCmd", __FILE__, __LINE__);
                        if (pModule != NULL)
                        {
                            LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                            delete module_iter->second;
                            module_iter->second = pModule;
                        }
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

void ThunderWorker::ReloadModule(thunder::CJsonObject& oUrlPaths)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<std::string, tagModule*>::iterator module_iter;
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
            pModule = LoadSoAndGetModule(url_path, strSoPath, strSymbol, iVersion);
            LOG4_TRACE("%s:%d after ReloadModule", __FILE__, __LINE__);
            if (pModule != NULL)
            {
                LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                delete module_iter->second;
                module_iter->second = pModule;
            }
        }
        else
        {
            LOG4_WARN("no such url_path %s", url_path.c_str());
        }
    }
}

tagModule* ThunderWorker::LoadSoAndGetModule(const std::string& strModulePath, const std::string& strSoPath, const std::string& strSymbol, int iVersion)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    tagModule* pSo = NULL;
    void* pHandle = NULL;
    pHandle = dlopen(strSoPath.c_str(), RTLD_NOW);
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
            pSo->pModule->SetLogger(&m_oLogger);
            pSo->pModule->SetLabor(this);
            pSo->pModule->SetConfigPath(m_strWorkPath);
            pSo->pModule->SetModulePath(strModulePath);
            if (!pSo->pModule->Init())
            {
                LOG4_FATAL("Module %s %s init error", strModulePath.c_str(), strSoPath.c_str());
                delete pSo;
                pSo = NULL;
            }
        }
        else
        {
            LOG4_FATAL("new tagSo() error!");
            delete pModule;
            dlclose(pHandle);
        }
    }
    return(pSo);
}

void ThunderWorker::UnloadSoAndDeleteModule(const std::string& strModulePath)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<std::string, tagModule*>::iterator module_iter;
    module_iter = m_mapModule.find(strModulePath);
    if (module_iter != m_mapModule.end())
    {
        delete module_iter->second;
        module_iter->second = NULL;
        m_mapModule.erase(module_iter);
    }
}

bool ThunderWorker::AddPeriodicTaskEvent()
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

bool ThunderWorker::AddIoReadEvent(tagConnectionAttr* pTagConnectionAttr,int iFd)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
	if (NULL == pTagConnectionAttr->pIoWatcher)
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
		pData->iFd = iFd;
		pData->ulSeq = pTagConnectionAttr->ulSeq;
		pData->pWorker = this;
		ev_io_init (pTagConnectionAttr->pIoWatcher, IoCallback, iFd, EV_READ);
		io_watcher->data = (void*)pData;
		pTagConnectionAttr->pIoWatcher = io_watcher;
		ev_io_start (m_loop, io_watcher);
	}
	else
	{
		io_watcher = pTagConnectionAttr->pIoWatcher;
		ev_io_stop(m_loop, io_watcher);
		ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_READ);
		ev_io_start (m_loop, io_watcher);
	}
    return(true);
}

bool ThunderWorker::AddIoWriteEvent(tagConnectionAttr* pTagConnectionAttr,int iFd)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
	if (NULL == pTagConnectionAttr->pIoWatcher)
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
		pData->iFd = iFd;
		pData->ulSeq = pTagConnectionAttr->ulSeq;
		pData->pWorker = this;
		ev_io_init (io_watcher, IoCallback, iFd, EV_WRITE);
		io_watcher->data = (void*)pData;
		pTagConnectionAttr->pIoWatcher = io_watcher;
		ev_io_start (m_loop, io_watcher);
	}
	else
	{
		io_watcher = pTagConnectionAttr->pIoWatcher;
		ev_io_stop(m_loop, io_watcher);
		ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_WRITE);
		ev_io_start (m_loop, io_watcher);
	}
    return(true);
}

//bool ThunderWorker::AddIoErrorEvent(int iFd)
//{
//    LOG4_TRACE("%s()", __FUNCTION__);
//    ev_io* io_watcher = NULL;
//    std::map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(iFd);
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

bool ThunderWorker::RemoveIoWriteEvent(tagConnectionAttr* pTagConnectionAttr)
{
    LOG4_TRACE("%s()", __FUNCTION__);
	if (NULL != pTagConnectionAttr->pIoWatcher)
	{
		if (pTagConnectionAttr->pIoWatcher->events & EV_WRITE)
		{
			ev_io* io_watcher = pTagConnectionAttr->pIoWatcher;
			ev_io_stop(m_loop, io_watcher);
			ev_io_set(io_watcher, io_watcher->fd, io_watcher->events & ~EV_WRITE);
			ev_io_start (m_loop, pTagConnectionAttr->pIoWatcher);
		}
	}
    return(true);
}

bool ThunderWorker::AddIoReadEvent(std::map<int, tagConnectionAttr*>::iterator iter)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
    if (NULL == iter->second->pIoWatcher)
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
        pData->iFd = iter->first;
        pData->ulSeq = iter->second->ulSeq;
        pData->pWorker = this;
        ev_io_init (io_watcher, IoCallback, iter->first, EV_READ);
        io_watcher->data = (void*)pData;
        iter->second->pIoWatcher = io_watcher;
        ev_io_start (m_loop, io_watcher);
    }
    else
    {
        io_watcher = iter->second->pIoWatcher;
        ev_io_stop(m_loop, io_watcher);
        ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_READ);
        ev_io_start (m_loop, io_watcher);
    }
    return(true);
}

bool ThunderWorker::AddIoWriteEvent(std::map<int, tagConnectionAttr*>::iterator iter)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
    if (NULL == iter->second->pIoWatcher)
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
        pData->iFd = iter->first;
        pData->ulSeq = iter->second->ulSeq;
        pData->pWorker = this;
        ev_io_init (io_watcher, IoCallback, iter->first, EV_WRITE);
        io_watcher->data = (void*)pData;
        iter->second->pIoWatcher = io_watcher;
        ev_io_start (m_loop, io_watcher);
    }
    else
    {
        io_watcher = iter->second->pIoWatcher;
        ev_io_stop(m_loop, io_watcher);
        ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_WRITE);
        ev_io_start (m_loop, io_watcher);
    }
    return(true);
}

bool ThunderWorker::RemoveIoWriteEvent(std::map<int, tagConnectionAttr*>::iterator iter)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
    if (NULL != iter->second->pIoWatcher)
    {
        if (iter->second->pIoWatcher->events & EV_WRITE)
        {
            io_watcher = iter->second->pIoWatcher;
            ev_io_stop(m_loop, io_watcher);
            ev_io_set(io_watcher, io_watcher->fd, io_watcher->events & ~EV_WRITE);
            ev_io_start (m_loop, iter->second->pIoWatcher);
        }
    }
    return(true);
}

bool ThunderWorker::DelEvents(ev_io** io_watcher_addr)
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
    ev_io_stop (m_loop, *io_watcher_addr);
    tagIoWatcherData* pData = (tagIoWatcherData*)(*io_watcher_addr)->data;
    delete pData;
    (*io_watcher_addr)->data = NULL;
    delete (*io_watcher_addr);
    (*io_watcher_addr) = NULL;
    io_watcher_addr = NULL;
    return(true);
}

bool ThunderWorker::AddIoTimeout(int iFd, uint32 ulSeq, tagConnectionAttr* pConnAttr, ev_tstamp dTimeout)
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

tagConnectionAttr* ThunderWorker::CreateFdAttr(int iFd, uint32 ulSeq, thunder::E_CODEC_TYPE eCodecType)
{
    LOG4_DEBUG("%s(fd[%d], seq[%u], codec[%d])", __FUNCTION__, iFd, ulSeq, eCodecType);
    std::map<int, tagConnectionAttr*>::iterator fd_attr_iter;
    fd_attr_iter = m_mapFdAttr.find(iFd);
    if (fd_attr_iter == m_mapFdAttr.end())
    {
        tagConnectionAttr* pConnAttr = new tagConnectionAttr();
        if (pConnAttr == NULL)
        {
            LOG4_ERROR("new pConnAttr for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->pRecvBuff = new thunder::CBuffer();
        if (pConnAttr->pRecvBuff == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pRecvBuff for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->pSendBuff = new thunder::CBuffer();
        if (pConnAttr->pSendBuff == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pSendBuff for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->pWaitForSendBuff = new thunder::CBuffer();
        if (pConnAttr->pWaitForSendBuff == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pWaitForSendBuff for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->pRemoteAddr = (char*)malloc(32);
        if (pConnAttr->pRemoteAddr == NULL)
        {
            delete pConnAttr;
            LOG4_ERROR("new pConnAttr->pRemoteAddr for fd %d error!", iFd);
            return(NULL);
        }
        pConnAttr->pClientData = new thunder::CBuffer();
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

bool ThunderWorker::DestroyConnect(std::map<int, tagConnectionAttr*>::iterator iter, bool bMsgShellNotice)
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
    MsgShell stMsgShell;
    stMsgShell.iFd = iter->first;
    stMsgShell.ulSeq = iter->second->ulSeq;
    std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(iter->first);
    if (inner_iter != m_mapInnerFd.end())
    {
        LOG4_TRACE("%s() m_mapInnerFd.size() = %u", __FUNCTION__, m_mapInnerFd.size());
        m_mapInnerFd.erase(inner_iter);
    }
    std::map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
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
    delete iter->second;
    iter->second = NULL;
    m_mapFdAttr.erase(iter);
    return(true);
}

void ThunderWorker::MsgShellNotice(const MsgShell& stMsgShell, const std::string& strIdentify, thunder::CBuffer* pClientData)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int32, tagSo*>::iterator cmd_iter;
    cmd_iter = m_mapSo.find(CMD_REQ_DISCONNECT);
    if (cmd_iter != m_mapSo.end() && cmd_iter->second != NULL)
    {
        MsgHead oMsgHead;
        MsgBody oMsgBody;
        oMsgBody.set_body(strIdentify);
        if (pClientData != NULL)
        {
            if (pClientData->ReadableBytes() > 0)
            {
                oMsgBody.set_additional(pClientData->GetRawReadBuffer(), pClientData->ReadableBytes());
            }
        }
        oMsgHead.set_cmd(CMD_REQ_DISCONNECT);
        oMsgHead.set_seq(GetSequence());
        oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
        cmd_iter->second->pCmd->AnyMessage(stMsgShell, oMsgHead, oMsgBody);
    }
}

bool ThunderWorker::Dispose(const MsgShell& stMsgShell,
                const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,
                MsgHead& oOutMsgHead, MsgBody& oOutMsgBody)
{
    LOG4_DEBUG("%s(cmd %u, seq %lu)",
                    __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq());
    oOutMsgHead.Clear();
    oOutMsgBody.Clear();
    if (gc_uiCmdReq & oInMsgHead.cmd())    // 新请求
    {
        std::map<int32, Cmd*>::iterator cmd_iter;
        cmd_iter = m_mapCmd.find(gc_uiCmdBit & oInMsgHead.cmd());
        if (cmd_iter != m_mapCmd.end() && cmd_iter->second != NULL)
        {
            cmd_iter->second->AnyMessage(stMsgShell, oInMsgHead, oInMsgBody);
        }
        else
        {
            std::map<int, tagSo*>::iterator cmd_so_iter;
            cmd_so_iter = m_mapSo.find(gc_uiCmdBit & oInMsgHead.cmd());
            if (cmd_so_iter != m_mapSo.end() && cmd_so_iter->second != NULL)
            {
                cmd_so_iter->second->pCmd->AnyMessage(stMsgShell, oInMsgHead, oInMsgBody);
            }
            else        // 没有对应的cmd，是需由接入层转发的请求
            {
                if (CMD_REQ_SET_LOG_LEVEL == oInMsgHead.cmd())
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
                else if (CMD_REQ_RELOAD_SO == oInMsgHead.cmd())
                {
                    thunder::CJsonObject oSoConfJson;
                    if(!oSoConfJson.Parse(oInMsgBody.body()))
                    {
                        LOG4_WARN("failed to parse oSoConfJson:(%s)",oInMsgBody.body().c_str());
                    }
                    else
                    {
                        LOG4_INFO("CMD_REQ_RELOAD_SO:update so conf to oSoConfJson(%s)", oSoConfJson.ToString().c_str());
                        LoadSo(oSoConfJson);
                    }
                }
                else if (CMD_REQ_RELOAD_MODULE == oInMsgHead.cmd())
                {
                    thunder::CJsonObject oModuleConfJson;
                    if(!oModuleConfJson.Parse(oInMsgBody.body()))
                    {
                        LOG4_WARN("failed to parse oModuleConfJson:(%s)",oInMsgBody.body().c_str());
                    }
                    else
                    {
                        LOG4_INFO("CMD_REQ_RELOAD_MODULE:update module conf to oModuleConfJson(%s)", oModuleConfJson.ToString().c_str());
                        LoadModule(oModuleConfJson);
                    }
                }
                else if (CMD_REQ_RELOAD_LOGIC_CONFIG == oInMsgHead.cmd())
                {
                    thunder::CJsonObject oConfJson;
                    if(!oConfJson.Parse(oInMsgBody.body()))
                    {
                        LOG4_WARN("failed to parse oConfJson:(%s)",oInMsgBody.body().c_str());
                    }
                    else
                    {
                        thunder::CJsonObject oCmds;
                        if(oConfJson.Get("cmd",oCmds))
                        {
                            LOG4_INFO("reload so conf to oCmds(%s)", oCmds.ToString().c_str());
                            ReloadSo(oCmds);
                        }
                        thunder::CJsonObject oUrlPaths;
                        if(oConfJson.Get("url_path",oUrlPaths))
                        {
                            LOG4_INFO("reload module conf to oUrlPaths(%s)", oUrlPaths.ToString().c_str());
                            ReloadModule(oUrlPaths);
                        }
                    }
                }
                else
                {
#ifdef NODE_TYPE_GATE
                    std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(stMsgShell.iFd);
                    if (inner_iter != m_mapInnerFd.end())   // 内部服务往客户端发送  if (std::string("0.0.0.0") == strFromIp)
                    {
                        cmd_so_iter = m_mapSo.find(CMD_REQ_TO_CLIENT);
                        if (cmd_so_iter != m_mapSo.end())
                        {
                            cmd_so_iter->second->pCmd->AnyMessage(stMsgShell, oInMsgHead, oInMsgBody);
                        }
                        else
                        {
                            snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oInMsgHead.cmd());
                            LOG4_ERROR(m_pErrBuff);
                            OrdinaryResponse oRes;
                            oRes.set_err_no(ERR_UNKNOWN_CMD);
                            oRes.set_err_msg(m_pErrBuff);
                            oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                            oOutMsgHead.set_seq(oInMsgHead.seq());
                            if(!BuildClientMsg(oOutMsgHead,oOutMsgBody,oRes))
                            {
                                LOG4_ERROR("failed to BuildClientMsg as CMD_RSP_SYS_ERROR response,seq(%u)",oInMsgHead.seq());
                            }
                        }
                    }
                    else
                    {
                        cmd_so_iter = m_mapSo.find(CMD_REQ_FROM_CLIENT);
                        if (cmd_so_iter != m_mapSo.end() && cmd_so_iter->second != NULL)
                        {
                            cmd_so_iter->second->pCmd->AnyMessage(stMsgShell, oInMsgHead, oInMsgBody);
                        }
                        else
                        {
                            snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oInMsgHead.cmd());
                            LOG4_ERROR(m_pErrBuff);
                            OrdinaryResponse oRes;
                            oRes.set_err_no(ERR_UNKNOWN_CMD);
                            oRes.set_err_msg(m_pErrBuff);
                            oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                            oOutMsgHead.set_seq(oInMsgHead.seq());
                            if(!BuildClientMsg(oOutMsgHead,oOutMsgBody,oRes))
                            {
                                LOG4_ERROR("failed to BuildClientMsg as CMD_RSP_SYS_ERROR response,seq(%u)",oInMsgHead.seq());
                            }
                        }
                    }
#else
                    snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oInMsgHead.cmd());
                    LOG4_ERROR(m_pErrBuff);
                    OrdinaryResponse oRes;
                    oRes.set_err_no(ERR_UNKNOWN_CMD);
                    oRes.set_err_msg(m_pErrBuff);
                    oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                    oOutMsgHead.set_seq(oInMsgHead.seq());
                    if(!BuildClientMsg(oOutMsgHead,oOutMsgBody,oRes))
                    {
                        LOG4_ERROR("failed to BuildClientMsg as CMD_RSP_SYS_ERROR response,seq(%u)",oInMsgHead.seq());
                    }
#endif
                }
            }
        }
    }
    else    // 回调
    {
        std::map<uint32, Step*>::iterator step_iter;
        step_iter = m_mapCallbackStep.find(oInMsgHead.seq());
        if (step_iter != m_mapCallbackStep.end())   // 步骤回调
        {
            LOG4_TRACE("receive message, cmd = %d",
                            oInMsgHead.cmd());
            if (step_iter->second != NULL)
            {
//                if (thunder::CMD_RSP_SYS_ERROR == oInMsgHead.cmd())   框架层不应截止系统错误，业务层会有逻辑，对于无逻辑的也要求加上对系统错误处理
//                {
//                    OrdinaryResponse oError;
//                    if (oError.ParseFromString(oInMsgBody.body()))
//                    {
//                        LOG4_ERROR("cmd[%u] seq[%u] callback error %d: %s!",
//                                        oInMsgHead.cmd(), oInMsgHead.seq(),
//                                        oError.err_no(), oError.err_msg().c_str());
//                        DeleteCallback(step_iter->second);
//                        return(false);
//                    }
//                }
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
//            oRes.set_err_no(ERR_NO_CALLBACK);
//            oRes.set_err_msg(m_pErrBuff);
//            oOutMsgBody.set_body(oRes.SerializeAsString());
//            oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
//            oOutMsgHead.set_seq(oInMsgHead.seq());
//            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        }
    }
    return(true);
}

bool ThunderWorker::Dispose(const MsgShell& stMsgShell,
                const HttpMsg& oInHttpMsg, HttpMsg& oOutHttpMsg)
{
    LOG4_DEBUG("%s() oInHttpMsg.type() = %d, oInHttpMsg.path() = %s",
                    __FUNCTION__, oInHttpMsg.type(), oInHttpMsg.path().c_str());
    oOutHttpMsg.Clear();
    if (HTTP_REQUEST == oInHttpMsg.type())    // 新请求
    {
        std::map<std::string, tagModule*>::iterator module_iter;
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
        std::map<int32, std::list<uint32> >::iterator http_step_iter = m_mapHttpAttr.find(stMsgShell.iFd);
        if (http_step_iter == m_mapHttpAttr.end())
        {
            LOG4_ERROR("no callback for http response from %s!", oInHttpMsg.url().c_str());
        }
        else
        {
            if (http_step_iter->second.begin() != http_step_iter->second.end())
            {
                std::map<uint32, Step*>::iterator step_iter;
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

} /* namespace thunder */
