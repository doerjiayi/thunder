/*******************************************************************************
* Project:  AsyncServer
* @file     main.cpp
* @brief 
* @author   cjy
* @date:    2017年8月6日
* @note
* Modify history:
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>

#include "CustomMsgHead.hpp"
#include "ThunderDefine.hpp"
#include "ThunderError.hpp"
#include "libev/ev.h"
#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"

#include "json/CJsonObject.hpp"
#include "CBuffer.hpp"
#include "unix_util/process_helper.h"
#include "protocol/msg.pb.h"
#include "codec/CustomMsgCodec.hpp"

using namespace thunder;

struct tagIoWatcherData
{
    int iFd;
    thunder::uint32 ulSeq;

    tagIoWatcherData() : iFd(0), ulSeq(0)
    {
    }
};

unsigned int g_uiSeq = 0;
struct ev_loop *m_loop = EV_DEFAULT;
log4cplus::Logger m_oLogger;
std::map<int, tagConnectionAttr*> g_mapFdAttr;
std::map<std::string, MsgShell> g_mapMsgShell;            // key为Identify
ThunderCodec* g_pCodec;

bool InitLogger(const std::string& strLogFile);
tagConnectionAttr* CreateFdAttr(int iFd, thunder::uint32 ulSeq, llib::E_CODEC_TYPE eCodecType);
bool DestroyConnect(std::map<int, tagConnectionAttr*>::iterator iter);
bool AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
bool AddIoReadEvent(tagConnectionAttr* pTagConnectionAttr,int iFd);
bool AddIoWriteEvent(tagConnectionAttr* pTagConnectionAttr,int iFd);
bool RemoveIoWriteEvent(int iFd);
bool DelEvents(ev_io** io_watcher_attr);
bool AddMsgShell(const std::string& strIdentify, const MsgShell& stMsgShell);
void DelMsgShell(const std::string& strIdentify);
bool SetConnectIdentify(const MsgShell& stMsgShell, const std::string& strIdentify);
static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
void IoRead(struct ev_loop* loop, struct ev_io* watcher, int revents);
void IoWrite(struct ev_loop* loop, struct ev_io* watcher, int revents);
void IoError(struct ev_loop* loop, struct ev_io* watcher, int revents);
bool SendTo(const MsgShell& stMsgShell);
bool SendTo(const MsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
bool SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
bool Dispose(const std::string& strFromIp, const MsgShell& stMsgShell,
                    const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,
                    MsgHead& oOutMsgHead, MsgBody& oOutMsgBody);

void setlog(int argc, char* argv[]);
void sendmsg(int argc, char* argv[]);

int main(int argc, char* argv[])
{
	if (argc != 5)
	{
		printf("Usage: testserver $ip $port $uid_from $uid_to \n");
		exit(-1);
	}
	setlog(argc,argv);
//	sendmsg(argc,argv);
	return(0);
}

void setlog(int argc, char* argv[])
{
	std::string strLogname = std::string("testserver.log");
	std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
	log4cplus::initialize();
	log4cplus::SharedAppenderPtr append(new log4cplus::RollingFileAppender(
					strLogname, 20480000, 5));
	append->setName(strLogname);
	std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(strParttern));
	append->setLayout(layout);
	m_oLogger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(strLogname));
	m_oLogger.addAppender(append);
	m_oLogger.setLogLevel(log4cplus::DEBUG_LOG_LEVEL);
	LOG4CPLUS_INFO_FMT(m_oLogger, "%s begin...", argv[0]);
}

void sendmsg(int argc, char* argv[])
{
	signal(SIGPIPE, SIG_IGN);
	int iUidFrom = atoi(argv[3]);
	int iUidTo = atoi(argv[4]);
	char szUid[16] = {0};
	char szIdentify[32] = {0};
	MsgHead oMsgHead;
	MsgBody oMsgBody;
	g_pCodec = new CustomMsgCodec(llib::CODEC_PRIVATE);
	g_pCodec->SetLogger(m_oLogger);
	snprintf(szIdentify, sizeof(szIdentify), "%s:%s", argv[1], argv[2]);
	for (int i = iUidFrom; i <= iUidTo; ++i)
	{
		LOG4CPLUS_DEBUG_FMT(m_oLogger, "uid %u", i);
		snprintf(szUid, sizeof(szUid), "%u", i);
		oMsgBody.set_body(szUid);
		oMsgHead.set_cmd(507);
		oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
		oMsgHead.set_seq(g_uiSeq++);
		SendTo(szIdentify, oMsgHead, oMsgBody);
	}
	ev_run (m_loop, 0);
}

bool AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    int iPosIpPortSeparator = strIdentify.find(':');
    if ((size_t)iPosIpPortSeparator == std::string::npos)
    {
        return(false);
    }
    int iPosPortWorkerIndexSeparator = strIdentify.rfind('.');
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, iPosPortWorkerIndexSeparator - (iPosIpPortSeparator + 1));
    int iPort = atoi(strPort.c_str());
    if (iPort == 0)
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
    g_uiSeq++;
    if (-1 == connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr)))
	{
    	if (EINPROGRESS == errno)
    	{
    		LOG4CPLUS_INFO_FMT(m_oLogger, "%s connecting errno(%d) strerror:%s",
					__FUNCTION__,errno,strerror(errno));
    	}
    	else
    	{
    		LOG4CPLUS_ERROR_FMT(m_oLogger, "%s failed to connect errno(%d) strerror:%s",
					__FUNCTION__,errno,strerror(errno));
			close(iFd);
			return false;
    	}
	}
    tagConnectionAttr* pTagConnectionAttr = CreateFdAttr(iFd, g_uiSeq, llib::CODEC_PRIVATE);
    if (pTagConnectionAttr)
    {
    	if (!AddIoReadEvent(pTagConnectionAttr,iFd))
		{
			return(false);
		}
		if (!AddIoWriteEvent(pTagConnectionAttr,iFd))
		{
			return(false);
		}
		g_pCodec->Encode(oMsgHead, oMsgBody, pTagConnectionAttr->pWaitForSendBuff);
		return(true);
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

tagConnectionAttr* CreateFdAttr(int iFd, thunder::uint32 ulSeq, llib::E_CODEC_TYPE eCodecType)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s(iFd %d, seq %lu, codec %d)", __FUNCTION__, iFd, ulSeq, eCodecType);
    tagConnectionAttr* pConnAttr = new tagConnectionAttr();
	if (pConnAttr == NULL)
	{
		LOG4CPLUS_ERROR_FMT(m_oLogger, "new pConnAttr for fd %d error!", iFd);
		return(NULL);
	}
    std::pair<std::map<int, tagConnectionAttr*>::iterator,bool> ret = g_mapFdAttr.insert(std::make_pair(iFd,pConnAttr));
    if (ret.second)//插入成功（否则是插入过的）
	{
    	pConnAttr->pRecvBuff = new llib::CBuffer();
		if (pConnAttr->pRecvBuff == NULL)
		{
			delete pConnAttr;
			LOG4CPLUS_ERROR_FMT(m_oLogger, "new pConnAttr->pRecvBuff for fd %d error!", iFd);
			return(NULL);
		}
		pConnAttr->pSendBuff = new llib::CBuffer();
		if (pConnAttr->pSendBuff == NULL)
		{
			delete pConnAttr;
			LOG4CPLUS_ERROR_FMT(m_oLogger, "new pConnAttr->pSendBuff for fd %d error!", iFd);
			return(NULL);
		}
		pConnAttr->pWaitForSendBuff = new llib::CBuffer();
		if (pConnAttr->pWaitForSendBuff == NULL)
		{
			delete pConnAttr;
			LOG4CPLUS_ERROR_FMT(m_oLogger, "new pConnAttr->pWaitForSendBuff for fd %d error!", iFd);
			return(NULL);
		}
		pConnAttr->dActiveTime = ev_now(m_loop);
		pConnAttr->ulSeq = ulSeq;
		pConnAttr->eCodecType = eCodecType;
		return(pConnAttr);
	}
    else
    {
    	delete pConnAttr;
    	pConnAttr = NULL;
    	LOG4CPLUS_ERROR_FMT(m_oLogger, "fd %d is exist!", iFd);
		return(NULL);
    }
}

bool DestroyConnect(std::map<int, tagConnectionAttr*>::iterator iter)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s()", __FUNCTION__);
    if (iter == g_mapFdAttr.end())
    {
        return(false);
    }
    MsgShell stMsgShell;
    stMsgShell.iFd = iter->first;
    stMsgShell.ulSeq = iter->second->ulSeq;
    DelMsgShell(iter->second->strIdentify);
    DelEvents(&(iter->second->pIoWatcher));
    close(iter->first);
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s() close iFd(%d)", __FUNCTION__,iter->first);
    delete iter->second;
    iter->second = NULL;
    g_mapFdAttr.erase(iter);
    return(true);
}

bool InitLogger(const std::string& strLogFile)
{
    thunder::int32 iMaxLogFileSize = 20480000;
    thunder::int32 iMaxLogFileNum = 5;
    thunder::int32 iLogLevel = 0;
    std::string strLogname = strLogFile;
    std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
    iLogLevel = log4cplus::INFO_LOG_LEVEL;
    log4cplus::initialize();
    log4cplus::SharedAppenderPtr append(new log4cplus::RollingFileAppender(
                    strLogname, iMaxLogFileSize, iMaxLogFileNum));
    append->setName(strLogname);
    std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(strParttern));
    append->setLayout(layout);
    m_oLogger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(strLogname));
    m_oLogger.addAppender(append);
    m_oLogger.setLogLevel(iLogLevel);
    LOG4CPLUS_INFO(m_oLogger, "program begin...");
    return(true);
}

bool AddIoReadEvent(tagConnectionAttr* pTagConnectionAttr,int iFd)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
	if (NULL == pTagConnectionAttr->pIoWatcher)
	{
		io_watcher = new ev_io();
		if (io_watcher == NULL)
		{
			LOG4CPLUS_ERROR(m_oLogger, "new io_watcher error!");
			return(false);
		}
		tagIoWatcherData* pData = new tagIoWatcherData();
		if (pData == NULL)
		{
			LOG4CPLUS_ERROR(m_oLogger, "new tagIoWatcherData error!");
			delete io_watcher;
			return(false);
		}
		LOG4CPLUS_DEBUG_FMT(m_oLogger, "new tagIoWatcherData:%p iFd(%d)!",pData,iFd);
		pData->iFd = iFd;
		pData->ulSeq = pTagConnectionAttr->ulSeq;
		ev_io_init (io_watcher, IoCallback, iFd, EV_READ);
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

bool AddIoWriteEvent(tagConnectionAttr* pTagConnectionAttr,int iFd)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
	if (NULL == pTagConnectionAttr->pIoWatcher)
	{
		io_watcher = new ev_io();
		if (io_watcher == NULL)
		{
			LOG4CPLUS_ERROR(m_oLogger, "new io_watcher error!");
			return(false);
		}
		tagIoWatcherData* pData = new tagIoWatcherData();
		if (pData == NULL)
		{
			LOG4CPLUS_ERROR(m_oLogger, "new tagIoWatcherData error!");
			delete io_watcher;
			return(false);
		}
		pData->iFd = iFd;
		pData->ulSeq = pTagConnectionAttr->ulSeq;
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

bool RemoveIoWriteEvent(int iFd)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s()", __FUNCTION__);
    ev_io* io_watcher = NULL;
    std::map<int, tagConnectionAttr*>::iterator iter = g_mapFdAttr.find(iFd);
    if (iter != g_mapFdAttr.end())
    {
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
    }
    return(true);
}

bool DelEvents(ev_io** io_watcher_addr)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s()", __FUNCTION__);
    if (io_watcher_addr == NULL)
    {
    	LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s() io_watcher_addr null", __FUNCTION__);
        return(false);
    }
    if (*io_watcher_addr == NULL)
    {
    	LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s() *io_watcher_addr null", __FUNCTION__);
        return(false);
    }
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s() ev_io_stop fd(%d) *io_watcher_addr:%p",
    		__FUNCTION__,(*io_watcher_addr)->fd,*io_watcher_addr);
    ev_io_stop (m_loop, *io_watcher_addr);
    tagIoWatcherData* pData = (tagIoWatcherData*)(*io_watcher_addr)->data;
    delete pData;
    (*io_watcher_addr)->data = NULL;
    delete (*io_watcher_addr);
    (*io_watcher_addr) = NULL;
    io_watcher_addr = NULL;
    return(true);
}

bool AddMsgShell(const std::string& strIdentify, const MsgShell& stMsgShell)
{
    std::map<std::string, MsgShell>::iterator iter = g_mapMsgShell.find(strIdentify);
    if (iter == g_mapMsgShell.end())
    {
        g_mapMsgShell.insert(std::pair<std::string, MsgShell>(strIdentify, stMsgShell));
    }
    else
    {
        iter->second = stMsgShell;
    }
    SetConnectIdentify(stMsgShell, strIdentify);
    return(true);
}

void DelMsgShell(const std::string& strIdentify)
{
    std::map<std::string, MsgShell>::iterator shell_iter = g_mapMsgShell.find(strIdentify);
    if (shell_iter == g_mapMsgShell.end())
    {
        ;
    }
    else
    {
        g_mapMsgShell.erase(shell_iter);
    }
}

bool SetConnectIdentify(const MsgShell& stMsgShell, const std::string& strIdentify)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s()", __FUNCTION__);
    std::map<int, tagConnectionAttr*>::iterator iter = g_mapFdAttr.find(stMsgShell.iFd);
    if (iter == g_mapFdAttr.end())
    {
        LOG4CPLUS_ERROR_FMT(m_oLogger, "no fd %d found in m_mapFdAttr", stMsgShell.iFd);
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
            LOG4CPLUS_ERROR_FMT(m_oLogger, "fd %d sequence %lu not match the sequence %lu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, iter->second->ulSeq);
            return(false);
        }
    }
}

void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
	/*
	 * ev_io_stop(m_loop, watcher);即便回收连接时调用了ev_io_stop，依然会触发IoWrite。需要检查data
	调用栈如下
	#3  IoWrite (loop=<value optimized out>, watcher=0x216ca70, revents=<value optimized out>) at ../test/main.cpp:622
	#4  0x00000000004154bf in IoCallback (loop=0x7fb3c3659400, watcher=0x216ca70, revents=3) at ../test/main.cpp:476
	#5  0x00007fb3c3450455 in ev_invoke_pending () at ev.c:3155
	#6  0x00007fb3c3453655 in ev_run () at ev.c:3555
	#7  0x00000000004166fe in sendmsg (argc=<value optimized out>, argv=<value optimized out>) at ../test/main.cpp:138
	#8  0x0000000000416910 in main (argc=<value optimized out>, argv=0x7fff916820b8) at ../test/main.cpp:96
	 * */
    if (watcher->data)
    {
        if (revents & EV_READ)
        {
            IoRead(loop, watcher, revents);
        }
        if (revents & EV_WRITE)
        {
            IoWrite(loop, watcher, revents);
        }
        if (revents & EV_ERROR)
        {
            IoError(loop, watcher, revents);
        }
    }
}

void IoRead(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s()", __FUNCTION__);
    tagIoWatcherData* pData = (tagIoWatcherData*)watcher->data;
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s()", __FUNCTION__);
    int iErrno = 0;
    int iReadLen = 0;
    std::map<int, tagConnectionAttr*>::iterator conn_iter;
    conn_iter = g_mapFdAttr.find(pData->iFd);
    if (conn_iter == g_mapFdAttr.end())
    {
        LOG4CPLUS_ERROR_FMT(m_oLogger, "no fd attr for %d!", pData->iFd);
    }
    else
    {
        if (pData->ulSeq != conn_iter->second->ulSeq)
        {
            LOG4CPLUS_DEBUG_FMT(m_oLogger, "callback seq %lu not match the conn attr seq %lu",
                            pData->ulSeq, conn_iter->second->ulSeq);
            ev_io_stop(m_loop, watcher);
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return;
        }
        iReadLen = conn_iter->second->pRecvBuff->ReadFD(pData->iFd, iErrno);
        LOG4CPLUS_DEBUG_FMT(m_oLogger, "recv from fd %d data len %d. "
                        "and conn_iter->second->pRecvBuff->ReadableBytes() = %d, read_idx=%d, codec = %d", pData->iFd, iReadLen,
                        conn_iter->second->pRecvBuff->ReadableBytes(), conn_iter->second->pRecvBuff->GetReadIndex(),
                        conn_iter->second->eCodecType);
        if (iReadLen > 0)
        {
            conn_iter->second->ulMsgNumUnitTime++;      // TODO 这里要做发送消息频率限制，有待补充
            conn_iter->second->ulMsgNum++;
            MsgHead oInMsgHead, oOutMsgHead;
            MsgBody oInMsgBody, oOutMsgBody;
            while (conn_iter->second->pRecvBuff->ReadableBytes() > 0)
            {
                oInMsgHead.Clear();
                oInMsgBody.Clear();
                E_CODEC_STATUS eCodecStatus = g_pCodec->Decode(conn_iter->second->pRecvBuff, oInMsgHead, oInMsgBody);
//                LOG4CPLUS_DEBUG_FMT(GetLogger(), "eCodecStatus=%d, "
//                                "conn_iter->second->pRecvBuff->ReadableBytes() = %d, read_idx=%d", eCodecStatus,
//                                conn_iter->second->pRecvBuff->ReadableBytes(), conn_iter->second->pRecvBuff->GetReadIndex());
                if (CODEC_STATUS_OK == eCodecStatus)
                {
                    conn_iter->second->dActiveTime = ev_now(m_loop);
                    bool bDisposeResult = false;
                    MsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = conn_iter->second->ulSeq;
                    if (oInMsgHead.cmd() > 0 && oInMsgHead.seq() > 0)  // 基于TCP的自定义协议请求或带cmd、seq自定义头域的http请求
                    {
                        std::string strClientIp = "0.0.0.0";
                        if (conn_iter->second->pRemoteAddr != NULL) // 来自客户端的请求
                        {
                            strClientIp = conn_iter->second->pRemoteAddr;
                            if (conn_iter->second->pClientData != NULL)
                            {
                                if (conn_iter->second->pClientData->ReadableBytes() > 0)
                                {
                                    oInMsgBody.set_additional(conn_iter->second->pClientData->GetRawReadBuffer(),
                                                    conn_iter->second->pClientData->ReadableBytes());
                                    oInMsgHead.set_msgbody_len(oInMsgBody.ByteSize());
                                }
                            }
                        }
                        if (gc_uiCmdReq & oInMsgHead.cmd())
                        {
                            conn_iter->second->ulForeignSeq = oInMsgHead.seq();
                        }
                        bDisposeResult = Dispose(strClientIp, stMsgShell, oInMsgHead, oInMsgBody, oOutMsgHead, oOutMsgBody); // 处理过程有可能会断开连接，所以下面要做连接是否存在检查
                        std::map<int, tagConnectionAttr*>::iterator dispose_conn_iter = g_mapFdAttr.find(pData->iFd);
                        if (dispose_conn_iter == g_mapFdAttr.end())     // 连接已断开，资源已回收
                        {
                            return;
                        }
                        else
                        {
                            if (pData->ulSeq != dispose_conn_iter->second->ulSeq)     // 连接已断开，资源已回收
                            {
                                return;
                            }
                        }
                        if (oOutMsgHead.ByteSize() > 0)
                        {
                            eCodecStatus = g_pCodec->Encode(oOutMsgHead, oOutMsgBody, conn_iter->second->pSendBuff);
                            if (CODEC_STATUS_OK == eCodecStatus)
                            {
                                conn_iter->second->pSendBuff->WriteFD(pData->iFd, iErrno);
                                if (iErrno != 0)
                                {
                                    LOG4CPLUS_ERROR_FMT(m_oLogger, "error %d!", iErrno);
                                }
                            }
                        }
                    }
                    if (!bDisposeResult)
                    {
                        break;
                    }
                }
                else if (CODEC_STATUS_ERR == eCodecStatus)
                {
                    DestroyConnect(conn_iter);
                    return;
                }
                else
                {
                    break;  // 数据尚未接收完整
                }
            }
            return;
        }
        else if (iReadLen == 0)
        {
            LOG4CPLUS_DEBUG_FMT(m_oLogger, "fd %d closed by peer, error %d!",
                            pData->iFd, iErrno);
            DestroyConnect(conn_iter);
        }
        else
        {
            if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                LOG4CPLUS_ERROR_FMT(m_oLogger, "recv from fd %d error %d", pData->iFd, iErrno);
                DestroyConnect(conn_iter);
            }
        }
    }
    return;
}

void IoWrite(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s() ev_io:%p iFd(%d)", __FUNCTION__,watcher,watcher->fd);
    tagIoWatcherData* pData = (tagIoWatcherData*)watcher->data;
    std::map<int, tagConnectionAttr*>::iterator attr_iter =  g_mapFdAttr.find(pData->iFd);
    if (attr_iter == g_mapFdAttr.end())
    {
        return;
    }
    else
    {
        if (pData->ulSeq != attr_iter->second->ulSeq)
        {
            LOG4CPLUS_DEBUG_FMT(m_oLogger, "callback seq %lu not match the conn attr seq %lu",
                            pData->ulSeq, attr_iter->second->ulSeq);
            ev_io_stop(m_loop, watcher);
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return;
        }
        int iErrno = 0;
        int iNeedWriteLen = (int)attr_iter->second->pSendBuff->ReadableBytes();
        int iWriteLen = 0;
        iWriteLen = attr_iter->second->pSendBuff->WriteFD(pData->iFd, iErrno);
        if (iWriteLen < 0)
        {
            if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                LOG4CPLUS_ERROR_FMT(m_oLogger, "send to fd %d error %d", pData->iFd, iErrno);
                DestroyConnect(attr_iter);
            }
            else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
            {
                attr_iter->second->dActiveTime = ev_now(m_loop);
                AddIoWriteEvent(attr_iter->second,pData->iFd);
            }
        }
        else if (iWriteLen > 0)
        {
            attr_iter->second->dActiveTime = ev_now(m_loop);
            if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
            {
                RemoveIoWriteEvent(pData->iFd);
            }
            else    // 内容未写完，添加或保持监听fd写事件
            {
                AddIoWriteEvent(attr_iter->second,pData->iFd);
            }
        }
        else    // iWriteLen == 0 写缓冲区为空
        {
//            LOG4CPLUS_DEBUG_FMT(m_oLogger, "pData->iFd %d, watcher->fd %d, iter->second->pWaitForSendBuff->ReadableBytes()=%d",
//                            pData->iFd, watcher->fd, attr_iter->second->pWaitForSendBuff->ReadableBytes());
            if (attr_iter->second->pWaitForSendBuff->ReadableBytes() > 0)    // 存在等待发送的数据，说明本次写事件是connect之后的第一个写事件
            {
                MsgShell stMsgShell;
                stMsgShell.iFd = pData->iFd;
                stMsgShell.ulSeq = attr_iter->second->ulSeq;
                SendTo(stMsgShell);
            }
        }
        return;
    }
}

void IoError(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    if (revents & EV_READ)
    {
        LOG4CPLUS_ERROR_FMT(m_oLogger, "EV_ERROR %d", revents);
    }
}

bool SendTo(const MsgShell& stMsgShell)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s(fd %d, seq %lu) pWaitForSendBuff", __FUNCTION__, stMsgShell.iFd, stMsgShell.ulSeq);
    std::map<int, tagConnectionAttr*>::iterator iter = g_mapFdAttr.find(stMsgShell.iFd);
    if (iter == g_mapFdAttr.end())
    {
        LOG4CPLUS_ERROR_FMT(m_oLogger, "no fd %d found in m_mapFdAttr", stMsgShell.iFd);
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
                if (iWriteLen < 0)
                {
                    if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                    {
                        LOG4CPLUS_ERROR_FMT(m_oLogger, "send to fd %d error %d", stMsgShell.iFd, iErrno);
                        DestroyConnect(iter);
                        return(false);
                    }
                    else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                    {
                        iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(iter->second,stMsgShell.iFd);
                    }
                }
                else if (iWriteLen > 0)
                {
                    iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        RemoveIoWriteEvent(stMsgShell.iFd);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        AddIoWriteEvent(iter->second,stMsgShell.iFd);
                    }
                }
                return(true);
            }
            else
            {
                LOG4CPLUS_ERROR_FMT(m_oLogger, "write to send buff error, iWriteLen = %d!", iWriteLen);
                iter->second->pSendBuff->SetWriteIndex(iWriteIdx);
                return(false);
            }
        }
    }
    return(false);
}

bool SendTo(const MsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s(fd %d, seq %lu)", __FUNCTION__, stMsgShell.iFd, stMsgShell.ulSeq);
    std::map<int, tagConnectionAttr*>::iterator conn_iter = g_mapFdAttr.find(stMsgShell.iFd);
    if (conn_iter == g_mapFdAttr.end())
    {
        LOG4CPLUS_ERROR_FMT(m_oLogger, "no fd %d found in m_mapFdAttr", stMsgShell.iFd);
        return(false);
    }
    else
    {
        if (conn_iter->second->ulSeq == stMsgShell.ulSeq)
        {
            E_CODEC_STATUS eCodecStatus = g_pCodec->Encode(oMsgHead, oMsgBody, conn_iter->second->pSendBuff);
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                int iErrno = 0;
                int iNeedWriteLen = (int)conn_iter->second->pSendBuff->ReadableBytes();
                LOG4CPLUS_DEBUG_FMT(m_oLogger, "try send cmd[%d] seq[%lu] len %d",
                                oMsgHead.cmd(), oMsgHead.seq(), iNeedWriteLen);
                int iWriteLen = conn_iter->second->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
                if (iWriteLen < 0)
                {
                    if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                    {
                        LOG4CPLUS_ERROR_FMT(m_oLogger, "send to fd %d error %d", stMsgShell.iFd, iErrno);
                        DestroyConnect(conn_iter);
                        return(false);
                    }
                    else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                    {
                        LOG4CPLUS_DEBUG_FMT(m_oLogger, "write len %d, error %d", iWriteLen, iErrno);
                        conn_iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(conn_iter->second,stMsgShell.iFd);
                    }
                    else
                    {
                        LOG4CPLUS_DEBUG_FMT(m_oLogger, "write len %d, error %d", iWriteLen, iErrno);
                        conn_iter->second->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(conn_iter->second,stMsgShell.iFd);
                    }
                }
                else if (iWriteLen > 0)
                {
                    conn_iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        LOG4CPLUS_DEBUG_FMT(m_oLogger, "cmd[%d] seq[%lu] need write %d and had written len %d",
                                        oMsgHead.cmd(), oMsgHead.seq(), iNeedWriteLen, iWriteLen);
                        RemoveIoWriteEvent(stMsgShell.iFd);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        LOG4CPLUS_DEBUG_FMT(m_oLogger, "cmd[%d] seq[%lu] need write %d and had written len %d",
                                        oMsgHead.cmd(), oMsgHead.seq(), iNeedWriteLen, iWriteLen);
                        AddIoWriteEvent(conn_iter->second,stMsgShell.iFd);
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
            LOG4CPLUS_ERROR_FMT(m_oLogger, "fd %d sequence %lu not match the sequence %lu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, conn_iter->second->ulSeq);
            return(false);
        }
    }
}

bool SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::map<std::string, MsgShell>::iterator shell_iter = g_mapMsgShell.find(strIdentify);
    if (shell_iter == g_mapMsgShell.end())
    {
        LOG4CPLUS_DEBUG_FMT(m_oLogger, "no MsgShell match %s.", strIdentify.c_str());
        return(AutoSend(strIdentify, oMsgHead, oMsgBody));
    }
    else
    {
        return(SendTo(shell_iter->second, oMsgHead, oMsgBody));
    }
}

bool Dispose(const std::string& strFromIp, const MsgShell& stMsgShell,
                const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,
                MsgHead& oOutMsgHead, MsgBody& oOutMsgBody)
{
    LOG4CPLUS_DEBUG_FMT(m_oLogger, "%s(cmd %u, seq %lu)", __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq());
    OrdinaryResponse oRes;
    oOutMsgHead.Clear();
    oOutMsgBody.Clear();
    if (gc_uiCmdReq & oInMsgHead.cmd())    // 新请求
    {
        if ((gc_uiCmdBit & oInMsgHead.cmd()) == 507)
        {
            MsgHead oOutMsgHead = oInMsgHead;
            MsgBody oOutMsgBody = oInMsgBody;
            oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
            LOG4CPLUS_DEBUG_FMT(m_oLogger, "send cmd %d", oOutMsgHead.cmd());
            SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
        }
        else
        {
            LOG4CPLUS_ERROR_FMT(m_oLogger, "no handler to dispose cmd %u!", oInMsgHead.cmd());
            oRes.set_err_no(100);
            oRes.set_err_msg("no handler to dispose cmd");
            oOutMsgBody.set_body(oRes.SerializeAsString());
            oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
            oOutMsgHead.set_seq(oInMsgHead.seq());
            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        }
    }
    else    // 回调
    {
        LOG4CPLUS_DEBUG_FMT(m_oLogger, "receive callback, cmd = %d", oInMsgHead.cmd());
    }
    return(true);
}
