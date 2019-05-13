/*******************************************************************************
 * Project:  Net
 * @file     Manager.cpp
 * @brief 
 * @author   cjy
 * @date:    2017年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include "unix/proctitle_helper.h"
#include "unix/process_helper.h"
#include "cmd/sys_cmd/CmdConnectWorker.hpp"
#include "Manager.hpp"

namespace net
{

void Manager::SignalCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Manager* pManager = (Manager*)watcher->data;
        if (SIGCHLD == watcher->signum)
        {
            pManager->OnChildTerminated(watcher);
        }
        else if (SIGUSR1 == watcher->signum)
        {
        	pManager->RefreshServer(true);//kill -SIGUSR1 xxx    killall -SIGUSR1   HelloThunder
        }
        else if (SIGUSR2 == watcher->signum)
        {
            pManager->RestartWorkers();//kill -SIGUSR2 xxx    killall -SIGUSR1   HelloThunder
        }
        else
        {
            pManager->OnManagerTerminated(watcher);
        }
    }
}

void Manager::IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Manager* pManager = (Manager*)watcher->data;
        pManager->CheckWorker();
        pManager->ReportToCenter();
    }
}

void Manager::IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagManagerIoWatcherData* pData = (tagManagerIoWatcherData*)watcher->data;
        Manager* pManager = pData->pManager;
        if (revents & EV_READ)
        {
            pManager->IoRead(pData, watcher);
        }
        if (revents & EV_WRITE)
        {
            pManager->IoWrite(pData, watcher);
        }
        if (revents & EV_ERROR)
        {
            pManager->IoError(pData, watcher);
        }
    }
}

void Manager::IoTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagManagerIoWatcherData* pData = (tagManagerIoWatcherData*)watcher->data;
        Manager* pManager = pData->pManager;
        pManager->IoTimeout(pData, watcher);
    }
}

void Manager::PeriodicTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Manager* pManager = (Manager*)(watcher->data);
#ifndef NODE_TYPE_CENTER
        pManager->ReportToCenter();
#endif
        pManager->CheckWorker();
        pManager->RefreshServer();
    }
    ev_timer_stop (loop, watcher);
    ev_timer_set (watcher, NODE_BEAT + ev_time() - ev_now(loop), 0);
    ev_timer_start (loop, watcher);
}

void Manager::WaitToExitTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagManagerWaitExitWatcherData* pData = (tagManagerWaitExitWatcherData*)(watcher->data);
        Manager* pManager = pData->pManager;
    #ifndef NODE_TYPE_CENTER
        if(CMD_REQ_NODE_STOP == pData->cmd)
        {
            pManager->CheckWorkersLoadEmptyExit(pData,watcher);//等待工作者负载为空时退出节点
        }
        else if (CMD_REQ_NODE_RESTART_WORKERS == pData->cmd)
        {
            pManager->CheckWorkersLoadEmptyRestart(pData,watcher);//等待工作者负载为空时退出节点重启工作者
        }
    #endif
    }
}

void Manager::ClientConnFrequencyTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagClientConnWatcherData* pData = (tagClientConnWatcherData*)watcher->data;
        Manager* pManager = pData->pManager;
        pManager->ClientConnFrequencyTimeout(pData, watcher);
    }
}

Manager::Manager(const std::string& strConfFile)
    : m_ulSequence(0), m_bInitLogger(false), m_dIoTimeout(480), m_strConfFile(strConfFile),
      m_uiNodeId(0), m_iPortForServer(9988), m_iPortForClient(0), m_iGatewayPort(0), m_uiWorkerNum(10),
      m_eCodec(util::CODEC_PROTOBUF), m_dAddrStatInterval(60.0), m_iAddrPermitNum(1000000),
      m_iLogLevel(log4cplus::INFO_LOG_LEVEL), m_iWorkerBeat(11), m_iRefreshInterval(60), m_iLastRefreshCalc(0),
      m_iS2SListenFd(-1), m_iC2SListenFd(-1), m_loop(NULL), m_pPeriodicTaskWatcher(NULL),m_iWaitToExitCounter(0),nPid(0)//, m_pCmdConnect(NULL)
{
    if (strConfFile == "")
    {
        std::cerr << "error: no config file!" << std::endl;
        exit(1);
    }

    if (!LoadConf())
    {
        std::cerr << "LoadConf() error!" << std::endl;
        exit(-1);
    }
    ngx_setproctitle(m_oCurrentConf("server_name").c_str());
    process_daemonize(m_strWorkPath.c_str());
    InstallSignal();
    nPid = getpid();
    Init();
#ifdef WORKER_OVERDUE
    int iWorkerBeat = WORKER_OVERDUE > ((gc_iBeatInterval << 1) + 1) ? WORKER_OVERDUE:((gc_iBeatInterval << 1) + 1);
    if (iWorkerBeat > m_iWorkerBeat) m_iWorkerBeat = iWorkerBeat;
#else
    int iWorkerBeat = (gc_iBeatInterval << 1) + 1;
    if (iWorkerBeat > m_iWorkerBeat) m_iWorkerBeat = iWorkerBeat;
#endif
    LOG4_DEBUG("%s() m_iWorkerBeat:%d gc_iBeatInterval:%d,NODE_BEAT:%f", __FUNCTION__,m_iWorkerBeat,gc_iBeatInterval,NODE_BEAT);
    CreateEvents();
    CreateWorker();
    RegisterToCenter();
}

Manager::~Manager()
{
    Destroy();
}

bool Manager::OnManagerTerminated(struct ev_signal* watcher)
{
    LOG4_WARN("%s terminated by signal %d!pid(%d)", m_oCurrentConf("server_name").c_str(), watcher->signum,getpid());
    ev_break (m_loop, EVBREAK_ALL);
    Destroy();
    exit(-1);
}

bool Manager::OnChildTerminated(struct ev_signal* watcher)
{
    pid_t   iPid = 0;
    int     iStatus = 0;
    int     iReturnCode = 0;
    //WUNTRACED
    while((iPid = waitpid(-1, &iStatus, WNOHANG)) > 0)
    {
        if (WIFEXITED(iStatus))
        {
            iReturnCode = WEXITSTATUS(iStatus);
        }
        else if (WIFSIGNALED(iStatus))
        {
            iReturnCode = WTERMSIG(iStatus);
        }
        else if (WIFSTOPPED(iStatus))
        {
            iReturnCode = WSTOPSIG(iStatus);
        }

        LOG4_FATAL("error %d: duty %d exit and sent signal %d with code %d!",
                        iStatus, iPid, watcher->signum, iReturnCode);
        RestartWorker(iPid);
    }
    return(true);
}

bool Manager::IoRead(tagManagerIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (watcher->fd == m_iS2SListenFd)//服务间通信的连接
    {
#ifdef UNIT_TEST
        return(FdTransfer(watcher->fd));
#endif
        return(AcceptServerConn(watcher->fd));
    }
    else if (m_iC2SListenFd >= 0 && watcher->fd == m_iC2SListenFd)//网关对外的连接
    {
        return(FdTransfer(watcher->fd));
    }
    else
    {
        return(RecvDataAndDispose(pData, watcher));
    }
}

bool Manager::FdTransfer(int iFd)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    char szIpAddr[16] = {0};
    struct sockaddr_in stClientAddr;
    socklen_t clientAddrSize = sizeof(stClientAddr);
    int iAcceptFd = accept(iFd, (struct sockaddr*) &stClientAddr, &clientAddrSize);
    if (iAcceptFd < 0)
    {
        LOG4_ERROR("accept error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        return(false);
    }
    strncpy(szIpAddr, inet_ntoa(stClientAddr.sin_addr), 16);
    {/* tcp连接检测 与发送设置*/
        int iKeepAlive = 1;
        int iKeepIdle = 1000;
        int iKeepInterval = 10;
        int iKeepCount = 3;
        int iTcpNoDelay = 1;
        if (setsockopt(iAcceptFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive)) < 0)
        {
            LOG4_WARN("setsockopt fail to set SO_KEEPALIVE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &iKeepIdle, sizeof(iKeepIdle)) < 0)
        {
            LOG4_WARN("setsockopt fail to set TCP_KEEPIDLE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval)) < 0)
        {
            LOG4_WARN("setsockopt fail to set TCP_KEEPINTVL");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount)) < 0)
        {
            LOG4_WARN("setsockopt fail to set TCP_KEEPCNT");
        }
#ifndef ENABLE_NAGLE
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
        {
            LOG4_WARN("setsockopt fail to set TCP_NODELAY");
        }
#endif
    }
    auto iter = m_mapClientConnFrequency.find(stClientAddr.sin_addr.s_addr);
    if (iter == m_mapClientConnFrequency.end())
    {
        m_mapClientConnFrequency.insert(std::pair<in_addr_t, uint32>(stClientAddr.sin_addr.s_addr, 1));
        AddClientConnFrequencyTimeout(stClientAddr.sin_addr.s_addr, m_dAddrStatInterval);
    }
    else
    {
        iter->second++;
        if (iter->second > (uint32)m_iAddrPermitNum)
        {
            LOG4_WARN("addrPermitNum error:client addr %d had been connected more than %u times in %f seconds, it's not permitted",
                            stClientAddr.sin_addr.s_addr, m_iAddrPermitNum, m_dAddrStatInterval);
            ::close(iAcceptFd);
            return(false);
        }
    }

    std::pair<int, int> worker_pid_fd = GetMinLoadWorkerDataFd();
    if (worker_pid_fd.second > 0)
    {
        LOG4_DEBUG("send new fd %d to worker communication fd %d",iAcceptFd, worker_pid_fd.second);
        int iCodec = m_eCodec;
        int iErrno = send_fd_with_attr(worker_pid_fd.second, iAcceptFd, szIpAddr, 16, iCodec);
        if (iErrno == 0)
        {
            AddWorkerLoad(worker_pid_fd.first);
        }
        else
        {
            LOG4_ERROR("send_fd_with_attr error %d: %s", iErrno, strerror_r(iErrno, m_pErrBuff, 1024));
        }
        close(iAcceptFd);
        return(true);
    }
    else
    {
    	LOG4_ERROR("GetMinLoadWorkerDataFd error");
    }
    return(false);
}

bool Manager::AcceptServerConn(int iFd)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    struct sockaddr_in stClientAddr;
    socklen_t clientAddrSize = sizeof(stClientAddr);
    int iAcceptFd = accept(iFd, (struct sockaddr*) &stClientAddr, &clientAddrSize);
    if (iAcceptFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        return(false);
    }
    else
    {
    	{/* tcp连接检测 */
    		int iKeepAlive = 1;
			int iKeepIdle = 60;
			int iKeepInterval = 5;
			int iKeepCount = 3;
			int iTcpNoDelay = 1;
			if (setsockopt(iAcceptFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive)) < 0)
			{
				LOG4_WARN("fail to set SO_KEEPALIVE");
			}
			if (setsockopt(iAcceptFd, SOL_TCP, TCP_KEEPIDLE, (void*) &iKeepIdle, sizeof(iKeepIdle)) < 0)
			{
				LOG4_WARN("fail to set SO_KEEPIDLE");
			}
			if (setsockopt(iAcceptFd, SOL_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval)) < 0)
			{
				LOG4_WARN("fail to set SO_KEEPINTVL");
			}
			if (setsockopt(iAcceptFd, SOL_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount)) < 0)
			{
				LOG4_WARN("fail to set SO_KEEPALIVE");
			}
#ifndef ENABLE_NAGLE
			if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
			{
				LOG4_WARN("fail to set TCP_NODELAY");
			}
#endif
    	}
        uint32 ulSeq = GetSequence();
        x_sock_set_block(iAcceptFd, 0);
        if (CreateFdAttr(iAcceptFd, ulSeq))
        {
            std::unordered_map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(iAcceptFd);
            if(AddIoTimeout(iAcceptFd, ulSeq))     // 为了防止大量连接攻击，初始化连接只有一秒即超时，在正常发送第一个数据包之后才采用正常配置的网络IO超时检查
            {
                if (!AddIoReadEvent(iter->second))
                {
                    DestroyConnect(iter);
                    return(false);
                }
                return(true);
            }
            else
            {
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

bool Manager::RecvDataAndDispose(tagManagerIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_DEBUG("fd %d, seq %llu", pData->iFd, pData->ulSeq);
    int iErrno = 0;
    int iReadLen = 0;
    auto conn_iter = m_mapFdAttr.find(pData->iFd);
    if (conn_iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd attr for %d!", pData->iFd);
    }
    else
    {
    	tagConnectionAttr* pConn = conn_iter->second;
        if (pData->ulSeq != pConn->ulSeq)
        {
            LOG4_DEBUG("callback seq %llu not match the conn attr seq %llu",pData->ulSeq, conn_iter->second->ulSeq);
            ev_io_stop(m_loop, watcher);
            pData->pManager = NULL;
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return(false);
        }
        iReadLen = pConn->pRecvBuff->ReadFD(pData->iFd, iErrno);
        LOG4_TRACE("recv from fd %d data len %d. and conn_iter->second->pRecvBuff->ReadableBytes() = %d", pData->iFd, iReadLen,conn_iter->second->pRecvBuff->ReadableBytes());
        if (iReadLen > 0)
        {
            while (pConn->pRecvBuff->ReadableBytes() >= gc_uiMsgHeadSize)
            {
                LOG4_TRACE("pConn->pRecvBuff->ReadableBytes() = %d",pConn->pRecvBuff->ReadableBytes());
                MsgHead oInMsgHead;
                bool bResult = oInMsgHead.ParseFromArray(pConn->pRecvBuff->GetRawReadBuffer(), gc_uiMsgHeadSize);
                if (bResult)
                {
                    LOG4_DEBUG("%s() oInMsgHead(%s)",__FUNCTION__,oInMsgHead.DebugString().c_str());
                    MsgBody oInMsgBody;
                    if (pConn->pRecvBuff->ReadableBytes() >= gc_uiMsgHeadSize + oInMsgHead.msgbody_len())
                    {
                        if (0 == oInMsgHead.msgbody_len())  // 无包体的数据包
                        {
                            bResult = true;
                        }
                        else
                        {
                            bResult = oInMsgBody.ParseFromArray(pConn->pRecvBuff->GetRawReadBuffer() + gc_uiMsgHeadSize, oInMsgHead.msgbody_len());
                        }
                        if (bResult)
                        {
                        	pConn->dActiveTime = ev_now(m_loop);
                            bool bContinue = false;     // 是否继续解析下一个数据包
                            tagMsgShell stMsgShell;
                            stMsgShell.iFd = pData->iFd;
                            stMsgShell.ulSeq = pConn->ulSeq;

                            auto worker_fd_iter = m_mapWorkerFdPid.find(watcher->fd);
                            if (worker_fd_iter == m_mapWorkerFdPid.end())   // 其他Server发过来要将连接传送到某个指定Worker进程信息
                            {
//                                LOG4_TRACE("strIdentify: %s, m_mapCenterMsgShell.size()=%d",
//                                                conn_iter->second->strIdentify.c_str(), m_mapCenterMsgShell.size());
                                auto center_iter = m_mapCenterMsgShell.find(pConn->strIdentify);
                                if (center_iter == m_mapCenterMsgShell.end())       // 非与center连接
                                {
//                                    LOG4_TRACE("center_iter == m_mapCenterMsgShell.end()");
                                    bContinue = DisposeDataAndTransferFd(stMsgShell, oInMsgHead, oInMsgBody, pConn->pSendBuff);
                                }
                                else
                                {
//                                    LOG4_TRACE("center_iter == m_mapCenterMsgShell.end()   else");
                                    bContinue = DisposeDataFromCenter(stMsgShell, oInMsgHead, oInMsgBody,pConn);
                                }
                            }
                            else    // Worker进程发过来的消息
                            {
                                bContinue = DisposeDataFromWorker(stMsgShell, oInMsgHead, oInMsgBody, pConn->pSendBuff);
                            }
                            pConn->pRecvBuff->SkipBytes(gc_uiMsgHeadSize + oInMsgBody.ByteSize());
                            pConn->pRecvBuff->Compact(32784);   // 超过32KB则重新分配内存
                            pConn->pSendBuff->Compact(32784);
                            if (!bContinue)
                            {
                                DestroyConnect(conn_iter);
                                return(false);
                            }
                        }
                        else
                        {
                            LOG4_ERROR("oInMsgBody.ParseFromArray() failed, data is broken from fd %d, close it!", pData->iFd);
                            DestroyConnect(conn_iter);
                            break;
                        }
                    }
                    else
                    {
                        break;  // 头部数据已完整，但body部分数据不完整
                    }
                }
                else
                {
                    LOG4_ERROR("oInMsgHead.ParseFromArray() failed, data is broken from fd %d, close it!", pData->iFd);
                    DestroyConnect(conn_iter);
                    break;
                }
            }
            return(true);
        }
        else if (iReadLen == 0)
        {
            LOG4_DEBUG("fd %d closed by peer, error %d %s!",pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
            DestroyConnect(conn_iter);
        }
        else
        {
            if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                LOG4_ERROR("recv from fd %d error %d: %s",pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                DestroyConnect(conn_iter);
            }
        }
    }
    return(true);
}

bool Manager::IoWrite(tagManagerIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_TRACE("%s(%d)", __FUNCTION__, pData->iFd);
    std::unordered_map<int, tagConnectionAttr*>::iterator attr_iter =  m_mapFdAttr.find(pData->iFd);
    if (attr_iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
    	tagConnectionAttr* pConn = attr_iter->second;
        if ((pData->ulSeq != pConn->ulSeq) || (pData->iFd != pConn->iFd))
        {
            LOG4_DEBUG("callback seq %llu or ifd(%d) not match the conn attr seq %llu or ifd(%d)",pData->ulSeq, pData->iFd,pConn->ulSeq,pConn->iFd);
            ev_io_stop(m_loop, watcher);
            pData->pManager = NULL;
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return(false);
        }
        int iErrno = 0;
        int iWriteLen = 0;
        iWriteLen = pConn->pSendBuff->WriteFD(pData->iFd, iErrno);
        LOG4_TRACE("iWriteLen = %d, send to fd %d error %d: %s", iWriteLen,pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
        if (iWriteLen < 0)
        {
            if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                LOG4_ERROR("send to fd %d error %d: %s",pData->iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                DestroyConnect(attr_iter);
            }
            else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
            {
            	pConn->dActiveTime = ev_now(m_loop);
                AddIoWriteEvent(pConn);
            }
        }
        else if (iWriteLen > 0)
        {
        	pConn->dActiveTime = ev_now(m_loop);
            if (iWriteLen == (int)attr_iter->second->pSendBuff->ReadableBytes())  // 已无内容可写，取消监听fd写事件
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
        	//LOG4_TRACE("pData->iFd %d, watcher->fd %d, iter->second->pWaitForSendBuff->ReadableBytes()=%d",pData->iFd, watcher->fd, attr_iter->second->pWaitForSendBuff->ReadableBytes());
            if (pConn->pWaitForSendBuff->ReadableBytes() > 0)    // 存在等待发送的数据，说明本次写事件是connect之后的第一个写事件
            {
                std::unordered_map<uint32, int>::iterator index_iter = m_mapSeq2WorkerIndex.find(pConn->ulSeq);
                if (index_iter != m_mapSeq2WorkerIndex.end())
                {
                    tagMsgShell stMsgShell;
                    stMsgShell.iFd = pData->iFd;
                    stMsgShell.ulSeq = pConn->ulSeq;
                    //AddInnerFd(stMsgShell); 只有Worker需要
                    std::unordered_map<std::string, tagMsgShell>::iterator center_iter = m_mapCenterMsgShell.find(pConn->strIdentify);
                    if (center_iter == m_mapCenterMsgShell.end())
                    {
                        m_mapCenterMsgShell.insert(std::pair<std::string, tagMsgShell>(pConn->strIdentify, stMsgShell));
                    }
                    else
                    {
                        center_iter->second = stMsgShell;
                    }
                    //m_pCmdConnect->Start(stMsgShell, index_iter->second);
                    MsgHead oMsgHead;
                    MsgBody oMsgBody;
                    ConnectWorker oConnWorker;
                    oConnWorker.set_worker_index(index_iter->second);
                    oMsgBody.set_body(oConnWorker.SerializeAsString());
                    oMsgHead.set_cmd(CMD_REQ_CONNECT_TO_WORKER);
                    oMsgHead.set_seq(GetSequence());
                    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
                    m_mapSeq2WorkerIndex.erase(index_iter);
                    LOG4_DEBUG("send after connect,oMsgHead(%s,%u),oMsgBody.ByteSize(%u)",oMsgHead.DebugString().c_str(),
                                    oMsgHead.ByteSize(),oMsgBody.ByteSize());
                    SendTo(stMsgShell, oMsgHead, oMsgBody);
                }
            }
        }
        return(true);
    }
}

bool Manager::IoError(tagManagerIoWatcherData* pData, struct ev_io* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(pData->iFd);
    if (iter == m_mapFdAttr.end())
    {
        return(false);
    }
    else
    {
        if (pData->ulSeq != iter->second->ulSeq || pData->iFd != iter->second->iFd)
        {
            LOG4_DEBUG("callback seq %llu not match the conn attr seq %llu",pData->ulSeq, iter->second->ulSeq);
            ev_io_stop(m_loop, watcher);
            pData->pManager = NULL;
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
            return(false);
        }
        DestroyConnect(iter);
    }
    std::unordered_map<int, int>::iterator worker_fd_iter = m_mapWorkerFdPid.find(pData->iFd);
    if (worker_fd_iter != m_mapWorkerFdPid.end())
    {
        kill(worker_fd_iter->first, SIGINT);
    }
    return(true);
}

bool Manager::IoTimeout(tagManagerIoWatcherData* pData, struct ev_timer* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    bool bRes = false;
    std::unordered_map<int, tagConnectionAttr*>::iterator iter =  m_mapFdAttr.find(pData->iFd);
    if (iter == m_mapFdAttr.end())
    {
        bRes = false;
    }
    else
    {
        if (pData->ulSeq != iter->second->ulSeq || pData->iFd != iter->second->iFd)      // 文件描述符数值相等，但已不是原来的文件描述符
        {
            bRes = false;
        }
        else
        {
            ev_tstamp after = iter->second->dActiveTime - ev_now(m_loop) + m_dIoTimeout;
            if (after > 0)    // IO在定时时间内被重新刷新过，重新设置定时器
            {
                LOG4_DEBUG("%s() ev_timer_set(%lf)", __FUNCTION__,after + ev_time() - ev_now(m_loop));
                ev_timer_stop (m_loop, watcher);
                ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
                ev_timer_start (m_loop, watcher);
                return(true);
            }
            else    // IO已超时，关闭文件描述符并清理相关资源
            {
                LOG4_WARN("%s()", __FUNCTION__);
                DestroyConnect(iter);
            }
            bRes = true;
        }
    }

    ev_timer_stop(m_loop, watcher);
    pData->pManager = NULL;
    delete pData;
    watcher->data = NULL;
    delete watcher;
    watcher = NULL;
    return(bRes);
}

bool Manager::ClientConnFrequencyTimeout(tagClientConnWatcherData* pData, struct ev_timer* watcher)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    bool bRes = false;
    auto iter = m_mapClientConnFrequency.find(pData->iAddr);
    if (iter == m_mapClientConnFrequency.end())
    {
        bRes = false;
    }
    else
    {
        m_mapClientConnFrequency.erase(iter);
        bRes = true;
    }

    ev_timer_stop(m_loop, watcher);
    pData->pManager = NULL;
    delete pData;
    watcher->data = NULL;
    delete watcher;
    watcher = NULL;
    return(bRes);
}

void Manager::Run()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_run (m_loop, 0);
}

bool Manager::InitLogger(const util::CJsonObject& oJsonConf)
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
        std::string strLogname = oJsonConf("log_path") + std::string("/") + getproctitle() + std::string(".log");
        std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
        std::ostringstream ssServerName;
        ssServerName << getproctitle() << " " << m_strHostForServer << ":" << m_iPortForServer;
        oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
        oJsonConf.Get("max_log_file_num", iMaxLogFileNum);
        oJsonConf.Get("log_level", iLogLevel);
        log4cplus::initialize();
        log4cplus::SharedAppenderPtr file_append(new log4cplus::RollingFileAppender(strLogname, iMaxLogFileSize, iMaxLogFileNum));
        file_append->setName(strLogname);
        std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(strParttern));
        file_append->setLayout(layout);
        //log4cplus::Logger::getRoot().addAppender(file_append);
        m_oLogger = log4cplus::Logger::getInstance(strLogname);
        m_oLogger.setLogLevel(iLogLevel);
        m_oLogger.addAppender(file_append);
        if (oJsonConf.Get("socket_logging_host", strLoggingHost) && oJsonConf.Get("socket_logging_port", iLoggingPort))
        {
            log4cplus::SharedAppenderPtr socket_append(new log4cplus::SocketAppender(strLoggingHost, iLoggingPort, ssServerName.str()));
            socket_append->setName(ssServerName.str());
            socket_append->setLayout(layout);
            socket_append->setThreshold(log4cplus::INFO_LOG_LEVEL);
            m_oLogger.addAppender(socket_append);
        }
        LOG4_INFO("%s program begin, and work path %s. pid(%d:%d)", oJsonConf("server_name").c_str(), m_strWorkPath.c_str(),nPid,getpid());
        m_bInitLogger = true;
        return(true);
    }
}

bool Manager::SendTo(const tagMsgShell& stMsgShell)
{
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
            iWriteLen = pConn->pSendBuff->Write(
            		pConn->pWaitForSendBuff, pConn->pWaitForSendBuff->ReadableBytes());
            if (iWriteLen == iNeedWriteLen)
            {
                iNeedWriteLen = (int)pConn->pSendBuff->ReadableBytes();
                iWriteLen = iter->second->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
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
                pConn->pSendBuff->SetWriteIndex(iWriteIdx);
                return(false);
            }
        }
        else
        {
        	LOG4_ERROR("pConn iFd(%d) ulSeq(%llu) stMsgShell iFd(%d) ulSeq(%llu) not match!",
        			pConn->iFd,pConn->ulSeq,stMsgShell.iFd,stMsgShell.ulSeq);
        }
    }
    return(false);
}

bool Manager::SendTo(const tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(cmd %u, seq %u)", __FUNCTION__, oMsgHead.cmd(), oMsgHead.seq());
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

//            MsgHead oSwitchMsgHead;//保证oSwitchMsgHead.ByteSize() == 15需要处理最高位
//            oSwitchMsgHead.set_cmd(oMsgHead.cmd() | 0x80000000);
//            oSwitchMsgHead.set_msgbody_len(oMsgHead.msgbody_len() | 0x80000000);
//            oSwitchMsgHead.set_seq(oMsgHead.seq() | 0x80000000);
            int iNeedWriteLen = oMsgHead.ByteSize();
            int iWriteIdx = pConn->pSendBuff->GetWriteIndex();
            iWriteLen = pConn->pSendBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
            LOG4_TRACE("iWriteLen = %d,oSwitchMsgHead size(%u),oMsgHead(%s)",
                            iWriteLen,oMsgHead.ByteSize(),oMsgHead.DebugString().c_str());
            if (iWriteLen != iNeedWriteLen)
            {
                LOG4_ERROR("write to send buff error, iWriteLen = %d!", iWriteLen);
                pConn->pSendBuff->SetWriteIndex(iWriteIdx);
                return(false);
            }
            iNeedWriteLen = oMsgBody.ByteSize();
            iWriteLen = pConn->pSendBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
            LOG4_TRACE("iWriteLen = %d,oMsgBody size(%u)", iWriteLen,oMsgBody.ByteSize());
            if (iWriteLen == iNeedWriteLen)
            {
                iNeedWriteLen = (int)pConn->pSendBuff->ReadableBytes();
                iWriteLen = pConn->pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
                LOG4_TRACE("iWriteLen = %d, send to fd %d error %d: %s", iWriteLen,
                                stMsgShell.iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
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
                        AddIoWriteEvent(pConn);
                    }
                }
                else if (iWriteLen > 0)
                {
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
                LOG4_ERROR("write to send buff error!");
                pConn->pSendBuff->SetWriteIndex(iWriteIdx);
                return(false);
            }
        }
        else
        {
            LOG4_ERROR("fd %d sequence %llu not match the iFd(%d) sequence %llu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, iter->second->iFd,iter->second->ulSeq);
            return(false);
        }
    }
}

bool Manager::SetConnectIdentify(const tagMsgShell& stMsgShell, const std::string& strIdentify)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(stMsgShell.iFd);
    if (iter == m_mapFdAttr.end())
    {
        LOG4_ERROR("no fd %d found in m_mapFdAttr", stMsgShell.iFd);
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
            LOG4_ERROR("fd %d sequence %llu not match the sequence %llu in m_mapFdAttr",
                            stMsgShell.iFd, stMsgShell.ulSeq, iter->second->ulSeq);
            return(false);
        }
    }
}

bool Manager::AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, strIdentify.c_str());
    int iPosIpPortSeparator = strIdentify.find(':');
    int iPosPortWorkerIndexSeparator = strIdentify.rfind('.');
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, iPosPortWorkerIndexSeparator - (iPosIpPortSeparator + 1));
    std::string strWorkerIndex = strIdentify.substr(iPosPortWorkerIndexSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    int iWorkerIndex = atoi(strWorkerIndex.c_str());
    struct sockaddr_in stAddr;
    int iFd = -1;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    bzero(&(stAddr.sin_zero), 8);
    iFd = socket(AF_INET, SOCK_STREAM, 0);

    std::unordered_map<int, int>::iterator worker_fd_iter = m_mapWorkerFdPid.find(iFd);
    if (worker_fd_iter != m_mapWorkerFdPid.end())
    {
        LOG4_TRACE("iFd = %d found in m_mapWorkerFdPid", iFd);
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
    tagConnectionAttr* pConn = CreateFdAttr(iFd, ulSeq);
    if (pConn)
    {
        if(!AddIoTimeout(iFd, ulSeq, 1.5))
        {
        	LOG4_ERROR("AddIoTimeout error");
			DestroyConnect(m_mapFdAttr.find(iFd));
			return(false);
        }
        if (!AddIoReadEvent(pConn))
		{
			LOG4_ERROR("AddIoReadEvent error");
			DestroyConnect(m_mapFdAttr.find(iFd));
			return(false);
		}
		if (!AddIoWriteEvent(pConn))
		{
			LOG4_ERROR("AddIoWriteEvent error");
			DestroyConnect(m_mapFdAttr.find(iFd));
			return(false);
		}
		pConn->pWaitForSendBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
		LOG4_TRACE("%s(),write oMsgHead size(%u)", __FUNCTION__,oMsgHead.ByteSize());
		pConn->pWaitForSendBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
		LOG4_TRACE("%s(),write oMsgBody size(%u)", __FUNCTION__,oMsgBody.ByteSize());
		pConn->strIdentify = strIdentify;
		LOG4_TRACE("fd %d seq %u identify %s."
						"iter->second->pWaitForSendBuff->ReadableBytes()=%u", iFd, ulSeq, strIdentify.c_str(),
						pConn->pWaitForSendBuff->ReadableBytes());
		m_mapSeq2WorkerIndex.insert(std::pair<uint32, int>(ulSeq, iWorkerIndex));
		std::unordered_map<std::string, tagMsgShell>::iterator center_iter = m_mapCenterMsgShell.find(strIdentify);
		if (center_iter != m_mapCenterMsgShell.end())
		{
			center_iter->second.iFd = iFd;
			center_iter->second.ulSeq = ulSeq;
		}
		connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
		return(true);
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

bool Manager::LoadConf()
{
    char szFilePath[256] = {0};
    //char szFileName[256] = {0};
    if (m_strWorkPath.length() == 0)
    {
        if (getcwd(szFilePath, sizeof(szFilePath)))
        {
            m_strWorkPath = szFilePath;
        }
        else
        {
            return(false);
        }
    }
    m_oLastConf = m_oCurrentConf;

    if (!net::GetConfig(m_oCurrentConf,m_strConfFile))//读取配置文件
	{
		LOG4_ERROR("Open conf (%s) to read error!",m_strConfFile.c_str());
		return false;
	}
	if (m_oLastConf.ToString() != m_oCurrentConf.ToString())
	{
		m_oCurrentConf.Get("io_timeout", m_dIoTimeout);
		m_oCurrentConf.Get("refresh_interval", m_iRefreshInterval);
		m_oCurrentConf.Get("worker_beat", m_iWorkerBeat);
		if (m_oLastConf.ToString().length() == 0)
		{
			m_uiWorkerNum = strtoul(m_oCurrentConf("process_num").c_str(), NULL, 10);
			m_oCurrentConf.Get("node_type", m_strNodeType);
			m_oCurrentConf.Get("inner_host", m_strHostForServer);
			m_oCurrentConf.Get("inner_port", m_iPortForServer);
			m_oCurrentConf.Get("access_host", m_strHostForClient);
			m_oCurrentConf.Get("access_port", m_iPortForClient);
			m_oCurrentConf.Get("gateway", m_strGateway);
			m_oCurrentConf.Get("gateway_port", m_iGatewayPort);
		}
		int32 iCodec;
		if (m_oCurrentConf.Get("access_codec", iCodec))
		{
			m_eCodec = util::E_CODEC_TYPE(iCodec);
		}
		m_oCurrentConf["permission"]["addr_permit"].Get("stat_interval", m_dAddrStatInterval);
		m_oCurrentConf["permission"]["addr_permit"].Get("permit_num", m_iAddrPermitNum);
		if (m_oCurrentConf.Get("log_level", m_iLogLevel))
		{
			switch (m_iLogLevel)
			{
				case log4cplus::DEBUG_LOG_LEVEL:break;
				case log4cplus::INFO_LOG_LEVEL:break;
				case log4cplus::TRACE_LOG_LEVEL:break;
				case log4cplus::WARN_LOG_LEVEL:break;
				case log4cplus::ERROR_LOG_LEVEL:break;
				case log4cplus::FATAL_LOG_LEVEL:break;
				default:m_iLogLevel = log4cplus::INFO_LOG_LEVEL;
			}
		}
		else
		{
			m_iLogLevel = log4cplus::INFO_LOG_LEVEL;
		}
	}
    return(true);
}

bool Manager::Init()
{
    InitLogger(m_oCurrentConf);

    socklen_t addressLen = 0;
    int queueLen = 100;
    if (m_strHostForClient.size() && m_iPortForClient)
    {
    	// 接入节点才需要监听客户端连接
		struct sockaddr_in stAddrOuter;
		struct sockaddr *pAddrOuter;
		stAddrOuter.sin_family = AF_INET;
		stAddrOuter.sin_port = htons(m_iPortForClient);
		stAddrOuter.sin_addr.s_addr = inet_addr(m_strHostForClient.c_str());
		pAddrOuter = (struct sockaddr*)&stAddrOuter;
		addressLen = sizeof(struct sockaddr);
		m_iC2SListenFd = socket(pAddrOuter->sa_family, SOCK_STREAM, 0);
		if (m_iC2SListenFd < 0)
		{
			LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
			int iErrno = errno;
			exit(iErrno);
		}
		{//socket属性
			int reuse = 1;
			int timeout = 1;
			int iTcpNoDelay = 1;
			if (::setsockopt(m_iC2SListenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
			{
				LOG4_WARN("fail to set SO_REUSEADDR");
			}
	//		if (::setsockopt(m_iC2SListenFd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(int)) < 0)
	//		{
	//			LOG4_WARN("fail to set TCP_DEFER_ACCEPT");
	//		}
	#ifndef ENABLE_NAGLE
			if (setsockopt(m_iC2SListenFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
			{
				LOG4_WARN("fail to set TCP_NODELAY");
			}
	#endif
		}
		if (bind(m_iC2SListenFd, pAddrOuter, addressLen) < 0)
		{
			LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
			CloseSocket(m_iC2SListenFd);
			exit(errno);
		}
		if (listen(m_iC2SListenFd, queueLen) < 0)
		{
			LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
			CloseSocket(m_iC2SListenFd);
			exit(errno);
		}
		LOG4_INFO("%s() listen on iPortForClient(%d) strHostForClient(%s)",__FUNCTION__,m_iPortForClient,m_strHostForClient.c_str());
    }

    struct sockaddr_in stAddrInner;
    struct sockaddr *pAddrInner;
    stAddrInner.sin_family = AF_INET;
    stAddrInner.sin_port = htons(m_iPortForServer);
    stAddrInner.sin_addr.s_addr = inet_addr(m_strHostForServer.c_str());
    pAddrInner = (struct sockaddr*)&stAddrInner;
    addressLen = sizeof(struct sockaddr);
    m_iS2SListenFd = socket(pAddrInner->sa_family, SOCK_STREAM, 0);
    if (m_iS2SListenFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        int iErrno = errno;
        exit(iErrno);
    }
    {
    	int reuse = 1;
		int timeout = 1;
		int iTcpNoDelay = 1;
		if (::setsockopt(m_iS2SListenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
		{
			LOG4_WARN("fail to set SO_REUSEADDR");
		}
//		if (::setsockopt(m_iS2SListenFd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(int)) < 0)
//		{
//			LOG4_WARN("fail to set TCP_DEFER_ACCEPT");
//		}
#ifndef ENABLE_NAGLE
		if (setsockopt(m_iS2SListenFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
		{
			LOG4_WARN("fail to set TCP_NODELAY");
		}
#endif
    }
    if (bind(m_iS2SListenFd, pAddrInner, addressLen) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        close(m_iS2SListenFd);
        m_iS2SListenFd = -1;
        int iErrno = errno;
        exit(iErrno);
    }
    if (listen(m_iS2SListenFd, queueLen) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        close(m_iS2SListenFd);
        m_iS2SListenFd = -1;
        int iErrno = errno;
        exit(iErrno);
    }
    LOG4_INFO("%s() pid(%d) listen on iPortForServer(%d) strHostForServer(%s)",__FUNCTION__,getpid(),m_iPortForServer,m_strHostForServer.c_str());
    // 创建到Center的连接信息
    for (int i = 0; i < m_oCurrentConf["center"].GetArraySize(); ++i)
    {
    	// CenterServer只有一个Worker
        std::string strIdentify = m_oCurrentConf["center"][i]("host") + std::string(":") + m_oCurrentConf["center"][i]("port") + std::string(".0");
        tagMsgShell stMsgShell;
        LOG4_TRACE("m_mapCenterMsgShell.insert(%s, fd %d, seq %llu) = %u",strIdentify.c_str(), stMsgShell.iFd, stMsgShell.ulSeq);
        m_mapCenterMsgShell.insert(std::pair<std::string, tagMsgShell>(strIdentify, stMsgShell));
    }
    return(true);
}

void Manager::Destroy()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    for (std::unordered_map<int32, Cmd*>::iterator cmd_iter = m_mapCmd.begin();
                    cmd_iter != m_mapCmd.end(); ++cmd_iter)
    {
        delete cmd_iter->second;
        cmd_iter->second = NULL;
    }
    m_mapCmd.clear();

    m_mapWorker.clear();
    m_mapWorkerFdPid.clear();
    m_mapWorkerRestartNum.clear();
    for (std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.begin();
                    iter != m_mapFdAttr.end(); ++iter)
    {
        DestroyConnect(iter);
    }
    m_mapFdAttr.clear();
    m_mapClientConnFrequency.clear();
    if (m_pPeriodicTaskWatcher != NULL)
    {
        free(m_pPeriodicTaskWatcher);
    }
    if (m_loop != NULL)
    {
        ev_loop_destroy(m_loop);
        m_loop = NULL;
    }
}

void Manager::CreateWorker()
{
    LOG4_TRACE("%s", __FUNCTION__);
    int iPid = 0;

    for (unsigned int i = 0; i < m_uiWorkerNum; ++i)
    {
        int iControlFds[2];
        int iDataFds[2];
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iControlFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        }
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iDataFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        }

        iPid = fork();
        if (iPid == 0)   // 子进程
        {
            ev_loop_destroy(m_loop);
            CloseSocket(m_iS2SListenFd);
            CloseSocket(m_iC2SListenFd);
            close(iControlFds[0]);
            close(iDataFds[0]);
            x_sock_set_block(iControlFds[1], 0);
            x_sock_set_block(iDataFds[1], 0);
            Worker worker(m_strWorkPath, iControlFds[1], iDataFds[1], i, m_oCurrentConf);
            worker.Run();
            LOG4_FATAL("Worker terminated");
            exit(-2);
        }
        else if (iPid > 0)   // 父进程
        {
            close(iControlFds[1]);
            close(iDataFds[1]);
            x_sock_set_block(iControlFds[0], 0);
            x_sock_set_block(iDataFds[0], 0);
            tagWorkerAttr stWorkerAttr;
            stWorkerAttr.iWorkerIndex = i;
            stWorkerAttr.iControlFd = iControlFds[0];
            stWorkerAttr.iDataFd = iDataFds[0];
            m_mapWorker.insert(std::pair<int, tagWorkerAttr>(iPid, stWorkerAttr));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iControlFds[0], iPid));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iDataFds[0], iPid));
            tagConnectionAttr* pTagConnectionAttrControl = CreateFdAttr(iControlFds[0], GetSequence());
            tagConnectionAttr* pTagConnectionAttrdata = CreateFdAttr(iDataFds[0], GetSequence());
            if (pTagConnectionAttrControl)
            {
            	AddIoReadEvent(pTagConnectionAttrControl);
            }
            else
            {
            	LOG4_ERROR("failed to create pTagConnectionAttrControl");
            }
            if (pTagConnectionAttrdata)
            {
            	AddIoReadEvent(pTagConnectionAttrdata);
            }
            else
            {
            	LOG4_ERROR("failed to create pTagConnectionAttrdata");
            }
        }
        else
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, 1024));
        }
    }
}

bool Manager::CreateEvents()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    m_loop = ev_loop_new(EVFLAG_FORKCHECK | EVFLAG_SIGNALFD |EVBACKEND_EPOLL);
    if (m_loop == NULL)
    {
        return(false);
    }
    tagConnectionAttr* ptagConnectionAttrS2S = CreateFdAttr(m_iS2SListenFd, GetSequence());
    if (ptagConnectionAttrS2S)
    {
    	AddIoReadEvent(ptagConnectionAttrS2S);
    }
    else
    {
    	LOG4_ERROR("%s() failed to create ptagConnectionAttrS2S for m_iS2SListenFd:%d",
    			__FUNCTION__,m_iS2SListenFd);
    }
//    AddIoErrorEvent(m_iS2SListenFd);
    if (m_iC2SListenFd >= 0)
    {
    	tagConnectionAttr* pTagConnectionAttrC2S = CreateFdAttr(m_iC2SListenFd, GetSequence());
		if (pTagConnectionAttrC2S)
		{
			AddIoReadEvent(pTagConnectionAttrC2S);
		}
		else
		{
			LOG4_ERROR("%s() failed to create pTagConnectionAttrC2S for m_iC2SListenFd:%d",
						__FUNCTION__,m_iC2SListenFd);
		}
	//    AddIoErrorEvent(m_iC2SListenFd);
    }

    ev_signal* child_signal_watcher = new ev_signal();
    ev_signal_init (child_signal_watcher, SignalCallback, SIGCHLD);
    child_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, child_signal_watcher);

    /*
    ev_signal* int_signal_watcher = new ev_signal();
    ev_signal_init (int_signal_watcher, SignalCallback, SIGINT);
    int_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, int_signal_watcher);
    */

    ev_signal* ill_signal_watcher = new ev_signal();
    ev_signal_init (ill_signal_watcher, SignalCallback, SIGILL);
    ill_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, ill_signal_watcher);

    ev_signal* bus_signal_watcher = new ev_signal();
    ev_signal_init (bus_signal_watcher, SignalCallback, SIGBUS);
    bus_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, bus_signal_watcher);

    ev_signal* fpe_signal_watcher = new ev_signal();
    ev_signal_init (fpe_signal_watcher, SignalCallback, SIGFPE);
    fpe_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, fpe_signal_watcher);

    ev_signal* kill_signal_watcher = new ev_signal();
    ev_signal_init (kill_signal_watcher, SignalCallback, SIGKILL);
    kill_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, kill_signal_watcher);

    ev_signal* term_signal_watcher = new ev_signal();
    ev_signal_init (term_signal_watcher, SignalCallback, SIGTERM);
    term_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, term_signal_watcher);

    //自定义信号1（处理进程初始化）
	ev_signal* user1_signal_watcher = new ev_signal();
	ev_signal_init (user1_signal_watcher, SignalCallback, SIGUSR1);
	user1_signal_watcher->data = (void*)this;
	ev_signal_start (m_loop, user1_signal_watcher);

	ev_signal* user2_signal_watcher = new ev_signal();
    ev_signal_init (user2_signal_watcher, SignalCallback, SIGUSR2);
    user2_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, user2_signal_watcher);

    AddPeriodicTaskEvent();
    // 注册idle事件在Server空闲时会导致CPU占用过高，暂时弃用之，改用定时器实现
//    ev_idle* idle_watcher = new ev_idle();
//    ev_idle_init (idle_watcher, IdleCallback);
//    idle_watcher->data = (void*)this;
//    ev_idle_start (m_loop, idle_watcher);

    return(true);
}

bool Manager::RegisterToCenter()
{
    if (m_mapCenterMsgShell.size() == 0)
    {
        return(true);
    }
    LOG4_DEBUG("%s()", __FUNCTION__);
    int iLoad = 0;
    int iConnect = 0;
    int iRecvNum = 0;
    int iRecvByte = 0;
    int iSendNum = 0;
    int iSendByte = 0;
    int iClientNum = 0;
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    util::CJsonObject oReportData;
    util::CJsonObject oMember;
    oReportData.Add("node_type", m_strNodeType);
    oReportData.Add("node_id", m_uiNodeId);
    oReportData.Add("node_ip", m_strHostForServer);
    oReportData.Add("node_port", m_iPortForServer);
    if (m_strGateway.size() > 0)
    {
        oReportData.Add("access_ip", m_strGateway);
    }
    else
    {
        oReportData.Add("access_ip", m_strHostForClient);
    }
    if (m_iGatewayPort > 0)
    {
        oReportData.Add("access_port", m_iGatewayPort);
    }
    else
    {
        oReportData.Add("access_port", m_iPortForClient);
    }
    oReportData.Add("worker_num", (int)m_mapWorker.size());
    oReportData.Add("active_time", ev_now(m_loop));
    oReportData.Add("node", util::CJsonObject("{}"));
    oReportData.Add("worker", util::CJsonObject("[]"));
    std::unordered_map<int, tagWorkerAttr>::iterator worker_iter = m_mapWorker.begin();
    for (; worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        iLoad += worker_iter->second.iLoad;
        iConnect += worker_iter->second.iConnect;
        iRecvNum += worker_iter->second.iRecvNum;
        iRecvByte += worker_iter->second.iRecvByte;
        iSendNum += worker_iter->second.iSendNum;
        iSendByte += worker_iter->second.iSendByte;
        iClientNum += worker_iter->second.iClientNum;
        oMember.Clear();
        oMember.Add("load", worker_iter->second.iLoad);
        oMember.Add("connect", worker_iter->second.iConnect);
        oMember.Add("recv_num", worker_iter->second.iRecvNum);
        oMember.Add("recv_byte", worker_iter->second.iRecvByte);
        oMember.Add("send_num", worker_iter->second.iSendNum);
        oMember.Add("send_byte", worker_iter->second.iSendByte);
        oMember.Add("client", worker_iter->second.iClientNum);
        oReportData["worker"].Add(oMember);
    }
    oReportData["node"].Add("load", iLoad);
    oReportData["node"].Add("connect", iConnect);
    oReportData["node"].Add("recv_num", iRecvNum);
    oReportData["node"].Add("recv_byte", iRecvByte);
    oReportData["node"].Add("send_num", iSendNum);
    oReportData["node"].Add("send_byte", iSendByte);
    oReportData["node"].Add("client", iClientNum);
    oMsgBody.set_body(oReportData.ToString());
    oMsgHead.set_cmd(CMD_REQ_NODE_REGISTER);
    oMsgHead.set_seq(GetSequence());
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    std::unordered_map<std::string, tagMsgShell>::iterator center_iter = m_mapCenterMsgShell.begin();
    for (; center_iter != m_mapCenterMsgShell.end(); ++center_iter)
    {
        if (center_iter->second.iFd == 0)
        {
            oMsgHead.set_cmd(CMD_REQ_NODE_REGISTER);
            LOG4_TRACE("%s() cmd %d", __FUNCTION__, oMsgHead.cmd());
            AutoSend(center_iter->first, oMsgHead, oMsgBody);
        }
        else
        {
            LOG4_TRACE("%s() cmd %d", __FUNCTION__, oMsgHead.cmd());
            SendTo(center_iter->second, oMsgHead, oMsgBody);
        }
    }
    return(true);
}

bool Manager::RestartWorker(int iDeathPid)
{
    LOG4_DEBUG("%s(%d)", __FUNCTION__, iDeathPid);
    int iNewPid = 0;
    char errMsg[1024] = {0};
    std::unordered_map<int, tagWorkerAttr>::iterator worker_iter;
    std::unordered_map<int, int>::iterator fd_iter;
    std::unordered_map<int, tagConnectionAttr*>::iterator conn_iter;
    std::unordered_map<int, int>::iterator restart_num_iter;
    worker_iter = m_mapWorker.find(iDeathPid);
    if (worker_iter != m_mapWorker.end())
    {
        LOG4_TRACE("restart worker %d, close control fd %d and data fd %d first.",
                        worker_iter->second.iWorkerIndex, worker_iter->second.iControlFd, worker_iter->second.iDataFd);
        int iWorkerIndex = worker_iter->second.iWorkerIndex;
        fd_iter = m_mapWorkerFdPid.find(worker_iter->second.iControlFd);
        if (fd_iter != m_mapWorkerFdPid.end())
        {
            m_mapWorkerFdPid.erase(fd_iter);
        }
        fd_iter = m_mapWorkerFdPid.find(worker_iter->second.iDataFd);
        if (fd_iter != m_mapWorkerFdPid.end())
        {
            m_mapWorkerFdPid.erase(fd_iter);
        }
        DestroyConnect(m_mapFdAttr.find(worker_iter->second.iControlFd));
        DestroyConnect(m_mapFdAttr.find(worker_iter->second.iDataFd));
        m_mapWorker.erase(worker_iter);

        restart_num_iter = m_mapWorkerRestartNum.find(iWorkerIndex);
        if (restart_num_iter != m_mapWorkerRestartNum.end())
        {
            LOG4_INFO("worker %d had been restarted %d times!", iWorkerIndex, restart_num_iter->second);
        }
        //父子进程使用unixsocket通信
        int iControlFds[2];
        int iDataFds[2];
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iControlFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, errMsg, 1024));
        }
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iDataFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, errMsg, 1024));
        }

        iNewPid = fork();
        if (iNewPid == 0)   // 子进程
        {
            ev_loop_destroy(m_loop);
            CloseSocket(m_iS2SListenFd);
            CloseSocket(m_iC2SListenFd);
            close(iControlFds[0]);
            close(iDataFds[0]);
            x_sock_set_block(iControlFds[1], 0);
            x_sock_set_block(iDataFds[1], 0);
            Worker worker(m_strWorkPath, iControlFds[1], iDataFds[1], iWorkerIndex, m_oCurrentConf);
            worker.Run();
            LOG4_FATAL("Worker terminated");
            exit(-2);   // 子进程worker没有正常运行
        }
        else if (iNewPid > 0)   // 父进程
        {
            LOG4_INFO("worker %d restart successfully", iWorkerIndex);
            ev_loop_fork(m_loop);
            close(iControlFds[1]);
            close(iDataFds[1]);
            x_sock_set_block(iControlFds[0], 0);
            x_sock_set_block(iDataFds[0], 0);
            tagWorkerAttr stWorkerAttr;
            stWorkerAttr.iWorkerIndex = iWorkerIndex;
            stWorkerAttr.iControlFd = iControlFds[0];
            stWorkerAttr.iDataFd = iDataFds[0];
            LOG4_TRACE("m_mapWorker insert (iNewPid %d, worker_index %d)", iNewPid, iWorkerIndex);
            m_mapWorker.insert(std::pair<int, tagWorkerAttr>(iNewPid, stWorkerAttr));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iControlFds[0], iNewPid));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iDataFds[0], iNewPid));
            tagConnectionAttr* ptagConnectionAttrControl = CreateFdAttr(iControlFds[0], GetSequence());
            tagConnectionAttr* ptagConnectionAttrData = CreateFdAttr(iDataFds[0], GetSequence());
            if (ptagConnectionAttrControl)
            {
            	AddIoReadEvent(ptagConnectionAttrControl);
            }
            else
            {
            	LOG4_ERROR("failed to create ptagConnectionAttrControl");
            }
            if (ptagConnectionAttrData)
            {
            	AddIoReadEvent(ptagConnectionAttrData);
            }
            else
            {
            	LOG4_ERROR("failed to create ptagConnectionAttrData");
            }
            restart_num_iter = m_mapWorkerRestartNum.find(iWorkerIndex);
            if (restart_num_iter == m_mapWorkerRestartNum.end())
            {
                m_mapWorkerRestartNum.insert(std::pair<int, int>(iWorkerIndex, 1));
            }
            else
            {
                restart_num_iter->second++;
            }
            RegisterToCenter();     // 重启Worker进程后向Center重发注册请求，以获得center下发其他节点的信息
            return(true);
        }
        else
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, errMsg, 1024));
        }
    }
    return(false);
}

bool Manager::AddPeriodicTaskEvent()
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    m_pPeriodicTaskWatcher = (ev_timer*)malloc(sizeof(ev_timer));
    if (m_pPeriodicTaskWatcher == NULL)
    {
        LOG4_ERROR("new timeout_watcher error!");
        return(false);
    }
    ev_timer_init (m_pPeriodicTaskWatcher, PeriodicTaskCallback, NODE_BEAT + ev_time() - ev_now(m_loop), 0.);
    m_pPeriodicTaskWatcher->data = (void*)this;
    ev_timer_start (m_loop, m_pPeriodicTaskWatcher);
    return(true);
}

bool Manager::AddWaitToExitTaskEvent(const tagMsgShell& stMsgShell,uint32 cmd,uint32 seq)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    if(0 != m_iWaitToExitCounter)//正在推出的就不再处理退出消息
    {
        LOG4_WARN("alread wait to exit!");
        return(false);
    }
    ev_timer* timeout_watcher = new ev_timer();
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("new timeout_watcher error!");
        return(false);
    }
    tagManagerWaitExitWatcherData* pData = new tagManagerWaitExitWatcherData();
    if (pData == NULL)
    {
        LOG4_ERROR("new tagManagerWaitExitWatcherData error!");
        delete timeout_watcher;
        return(false);
    }
    pData->stMsgShell = stMsgShell;
    pData->cmd = cmd;
    pData->seq = seq;
    pData->pManager = this;
    ++m_iWaitToExitCounter;
    ev_timer_init (timeout_watcher, WaitToExitTaskCallback, 0.5 + ev_time() - ev_now(m_loop), 0.);
    timeout_watcher->data = (void*)pData;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

bool Manager::AddIoReadEvent(tagConnectionAttr* pConn)
{
    LOG4_TRACE("%s(fd %d)", __FUNCTION__, pConn->iFd);
    ev_io* io_watcher = NULL;
	if (NULL == pConn->pIoWatcher)
	{
		io_watcher = new ev_io();
		if (io_watcher == NULL)
		{
			LOG4_ERROR("new io_watcher error!");
			return(false);
		}
		tagManagerIoWatcherData* pData = new tagManagerIoWatcherData();
		if (pData == NULL)
		{
			LOG4_ERROR("new tagIoWatcherData error!");
			delete io_watcher;
			return(false);
		}
		pData->iFd = pConn->iFd;
		pData->ulSeq = pConn->ulSeq;
		pData->pManager = this;
		ev_io_init (io_watcher, IoCallback, pData->iFd, EV_READ);
		pConn->pIoWatcher = io_watcher;
		io_watcher->data = (void*)pData;
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

bool Manager::AddIoWriteEvent(tagConnectionAttr* pConn)
{
    LOG4_TRACE("%s(fd %d)", __FUNCTION__, pConn->iFd);
    ev_io* io_watcher = NULL;
	if (NULL == pConn->pIoWatcher)
	{
		io_watcher = new ev_io();
		if (io_watcher == NULL)
		{
			LOG4_ERROR("new io_watcher error!");
			return(false);
		}
		tagManagerIoWatcherData* pData = new tagManagerIoWatcherData();
		if (pData == NULL)
		{
			LOG4_ERROR("new tagIoWatcherData error!");
			delete io_watcher;
			return(false);
		}
		pData->iFd = pConn->iFd;
		pData->ulSeq = pConn->ulSeq;
		pData->pManager = this;

		ev_io_init (io_watcher, IoCallback, pData->iFd, EV_WRITE);
		pConn->pIoWatcher = io_watcher;
		io_watcher->data = (void*)pData;
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

bool Manager::RemoveIoWriteEvent(tagConnectionAttr* pConn)
{
    LOG4_TRACE("%s", __FUNCTION__);
    ev_io* io_watcher = NULL;
	if (pConn->pIoWatcher)
	{
		if (pConn->pIoWatcher->events & EV_WRITE)
		{
			io_watcher = pConn->pIoWatcher;
			ev_io_stop(m_loop, io_watcher);
			ev_io_set(io_watcher, io_watcher->fd, io_watcher->events & (~EV_WRITE));
			ev_io_start (m_loop, pConn->pIoWatcher);
		}
	}
    return(true);
}

bool Manager::DelEvents(ev_io** io_watcher_addr)
{
//    LOG4_TRACE("%s(fd %d)", __FUNCTION__, (*io_watcher_addr)->fd);
    if (io_watcher_addr == NULL)
    {
        return(false);
    }
    LOG4_TRACE("%s(fd %d)", __FUNCTION__, (*io_watcher_addr)->fd);
    ev_io_stop (m_loop, *io_watcher_addr);
    tagManagerIoWatcherData* pData = (tagManagerIoWatcherData*)((*io_watcher_addr)->data);
    if (pData != NULL)
    {
        delete pData;
    }
    (*io_watcher_addr)->data = NULL;
    delete (*io_watcher_addr);
    (*io_watcher_addr) = NULL;
    io_watcher_addr = NULL;
    return(true);
}

bool Manager::AddIoTimeout(int iFd, uint32 ulSeq, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_timer* timeout_watcher = new ev_timer();
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("new timeout_watcher error!");
        return(false);
    }
    tagManagerIoWatcherData* pData = new tagManagerIoWatcherData();
    if (pData == NULL)
    {
        LOG4_ERROR("new tagIoWatcherData error!");
        delete timeout_watcher;
        return(false);
    }
    ev_timer_init (timeout_watcher, IoTimeoutCallback, dTimeout + ev_time() - ev_now(m_loop), 0.);
    pData->iFd = iFd;
    pData->ulSeq = ulSeq;
    pData->pManager = this;
    timeout_watcher->data = (void*)pData;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

bool Manager::AddClientConnFrequencyTimeout(in_addr_t iAddr, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_timer* timeout_watcher = new ev_timer();
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("new timeout_watcher error!");
        return(false);
    }
    tagClientConnWatcherData* pData = new tagClientConnWatcherData();
    if (pData == NULL)
    {
        LOG4_ERROR("new tagClientConnWatcherData error!");
        delete timeout_watcher;
        return(false);
    }
    ev_timer_init (timeout_watcher, ClientConnFrequencyTimeoutCallback, dTimeout + ev_time() - ev_now(m_loop), 0.);
    pData->pManager = this;
    pData->iAddr = iAddr;
    timeout_watcher->data = (void*)pData;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

tagConnectionAttr* Manager::CreateFdAttr(int iFd, uint32 ulSeq)
{
    LOG4_DEBUG("%s(iFd %d, seq %llu)", __FUNCTION__, iFd, ulSeq);
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
            LOG4_ERROR("new pConnAttr->pRecvBuff for fd%d error!", iFd);
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
        pConnAttr->dActiveTime = ev_now(m_loop);
        pConnAttr->ulSeq = ulSeq;
        std::unordered_map<int, tagConnectionAttr*>::iterator iter = m_mapFdAttr.find(iFd);
        if (iter == m_mapFdAttr.end())
        {
            m_mapFdAttr.insert(std::pair<int, tagConnectionAttr*>(iFd, pConnAttr));
        }
        else
        {
            delete pConnAttr;
            LOG4_ERROR("%s() m_mapFdAttr fd(%u) is already has!",__FUNCTION__,iter->first);
            return NULL;
        }
        return(pConnAttr);
    }
    else
    {
        LOG4_ERROR("fd %d is exist!", iFd);
        return(NULL);
    }
}

bool Manager::DestroyConnect(std::unordered_map<int, tagConnectionAttr*>::iterator iter)
{
    if (iter == m_mapFdAttr.end())
    {
        return(false);
    }
    LOG4_DEBUG("%s() iter->second->pIoWatcher = 0x%x, fd %d, data 0x%x", __FUNCTION__,
                    iter->second->pIoWatcher, iter->second->pIoWatcher->fd, iter->second->pIoWatcher->data);
    std::unordered_map<std::string, tagMsgShell>::iterator center_iter = m_mapCenterMsgShell.find(iter->second->strIdentify);
    if (center_iter != m_mapCenterMsgShell.end())
    {
        center_iter->second.iFd = 0;
        center_iter->second.ulSeq = 0;
    }
    DelEvents(&(iter->second->pIoWatcher));
    close(iter->first);
    delete iter->second;
    iter->second = NULL;
    m_mapFdAttr.erase(iter);
    return(true);
}

std::pair<int, int> Manager::GetMinLoadWorkerDataFd()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iMinLoadWorkerFd = 0;
    int iMinLoad = -1;
    std::pair<int, int> worker_pid_fd;
    std::unordered_map<int, tagWorkerAttr>::iterator iter;
    for (iter = m_mapWorker.begin(); iter != m_mapWorker.end(); ++iter)
    {
       if (iter == m_mapWorker.begin())
       {
           iMinLoadWorkerFd = iter->second.iDataFd;
           iMinLoad = iter->second.iLoad;
           worker_pid_fd = std::pair<int, int>(iter->first, iMinLoadWorkerFd);//pid=>iFd
       }
       else if (iter->second.iLoad < iMinLoad)
       {
           iMinLoadWorkerFd = iter->second.iDataFd;
           iMinLoad = iter->second.iLoad;
           worker_pid_fd = std::pair<int, int>(iter->first, iMinLoadWorkerFd);
       }
    }
    return(worker_pid_fd);
}

void Manager::SetWorkerLoad(int iPid, util::CJsonObject& oJsonLoad)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<int, tagWorkerAttr>::iterator iter;
    iter = m_mapWorker.find(iPid);
    if (iter != m_mapWorker.end())
    {
        oJsonLoad.Get("load", iter->second.iLoad);
        oJsonLoad.Get("connect", iter->second.iConnect);
        oJsonLoad.Get("recv_num", iter->second.iRecvNum);
        oJsonLoad.Get("recv_byte", iter->second.iRecvByte);
        oJsonLoad.Get("send_num", iter->second.iSendNum);
        oJsonLoad.Get("send_byte", iter->second.iSendByte);
        oJsonLoad.Get("client", iter->second.iClientNum);
        iter->second.dBeatTime = ev_now(m_loop);
    }
}

void Manager::AddWorkerLoad(int iPid, int iLoad)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::unordered_map<int, tagWorkerAttr>::iterator iter;
    iter = m_mapWorker.find(iPid);
    if (iter != m_mapWorker.end())
    {
        iter->second.iLoad += iLoad;
    }
}

const std::unordered_map<int, tagWorkerAttr>& Manager::GetWorkerAttr() const
{
	return(m_mapWorker);
}

bool Manager::CheckWorker()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (m_iRefreshInterval <= 0)
    {
        return(true);
    }
    for (auto worker_iter:m_mapWorker)
    {
        LOG4_TRACE("now %lf, worker's dBeatTime %lf, worker_beat %d",ev_now(m_loop), worker_iter.second.dBeatTime,m_iWorkerBeat);
        if ((ev_now(m_loop) - worker_iter.second.dBeatTime) > m_iWorkerBeat)
        {
            LOG4_INFO( "worker_%d pid %d is unresponsive, terminate it.", worker_iter.second.iWorkerIndex, worker_iter.first);
            kill(worker_iter.first, SIGKILL); //SIGINT);
//            RestartWorker(worker_iter->first);
        }
    }
    return(true);
}

bool Manager::RestartWorkers()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    for (auto worker_iter:m_mapWorker)
    {
        LOG4_TRACE("RestartWorkers:now %lf, worker's dBeatTime %lf, worker_beat %d",ev_now(m_loop), worker_iter.second.dBeatTime, m_iWorkerBeat);
        {
            LOG4_INFO( "terminate worker_%d pid %d",worker_iter.second.iWorkerIndex, worker_iter.first);
            kill(worker_iter.first, SIGKILL); //SIGINT);
        }
    }
    return(true);
}

bool Manager::CheckWorkersLoadEmptyExit(tagManagerWaitExitWatcherData* pData,struct ev_timer* watcher)
{
    LOG4_INFO("CheckWorkerLoadAndExit:m_mapWorker size(%u)",m_mapWorker.size());
    bool boCanExit(true);
    for (auto worker_iter:m_mapWorker)
    {
        LOG4_INFO("iLoad(%d),iConnect(%d)",worker_iter.second.iLoad,worker_iter.second.iConnect);
        if(worker_iter.second.iLoad != worker_iter.second.iConnect)
        {
//            oJsonLoad.Add("load", int32(m_mapFdAttr.size() + m_mapCallbackStep.size()));
//            oJsonLoad.Add("connect", int32(m_mapFdAttr.size()));
            LOG4_INFO("iLoad(%d),iConnect(%d),need iLoad == iConnect,continue to wait",worker_iter.second.iLoad,worker_iter.second.iConnect);
            boCanExit = false;
            break;
        }
    }
    if (boCanExit)//可以关闭
    {
        m_iWaitToExitCounter = 0;
        MsgHead oOutMsgHead;
        MsgBody oOutMsgBody;
        OrdinaryResponse oRes;
        oRes.set_err_no(0);
        oRes.set_err_msg("OK");
        oOutMsgBody.set_body(oRes.SerializeAsString());
        oOutMsgHead.set_cmd(pData->cmd + 1);
        oOutMsgHead.set_seq(pData->seq);
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        SendTo(pData->stMsgShell, oOutMsgHead, oOutMsgBody);
        LOG4_WARN("%s terminated!", m_oCurrentConf("server_name").c_str());
        ev_break (m_loop, EVBREAK_ALL);
        exit(0);
        return true;
    }
    if(m_iWaitToExitCounter < 6)
    {
        LOG4_INFO("WaitToExitTaskCallback: m_iWaitToExitCounter(%d)",m_iWaitToExitCounter);
        ++m_iWaitToExitCounter;
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, 1.0 + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, watcher);
    }
    else
    {
        LOG4_WARN("WaitToExitTaskCallback:ev_timer_stop,m_iWaitToExitCounter(%d)",m_iWaitToExitCounter);
        m_iWaitToExitCounter = 0;
        MsgHead oOutMsgHead;
        MsgBody oOutMsgBody;
        OrdinaryResponse oRes;
        oRes.set_err_no(1);
        oRes.set_err_msg("FAILED");
        oOutMsgBody.set_body(oRes.SerializeAsString());
        oOutMsgHead.set_cmd(pData->cmd + 1);
        oOutMsgHead.set_seq(pData->seq);
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        SendTo(pData->stMsgShell, oOutMsgHead, oOutMsgBody);
        ev_timer_stop (m_loop, watcher);
        delete pData;
        watcher->data = NULL;
        delete watcher;
        watcher = NULL;
    }
    return boCanExit;
}

bool Manager::CheckWorkersLoadEmptyRestart(tagManagerWaitExitWatcherData* pData,struct ev_timer* watcher)
{
    LOG4_INFO("CheckWorkersLoadEmptyRestart:m_mapWorker size(%u)",m_mapWorker.size());
    bool boCanRestart(true);
    for (auto worker_iter: m_mapWorker)
    {
        LOG4_INFO("iLoad(%d),iConnect(%d)",worker_iter.second.iLoad,worker_iter.second.iConnect);
        if(worker_iter.second.iLoad != worker_iter.second.iConnect)
        {
//            oJsonLoad.Add("load", int32(m_mapFdAttr.size() + m_mapCallbackStep.size()));
//            oJsonLoad.Add("connect", int32(m_mapFdAttr.size()));
            LOG4_INFO("iLoad(%d),iConnect(%d),need iLoad == iConnect,continue to wait",worker_iter.second.iLoad,worker_iter.second.iConnect);
            boCanRestart = false;
            break;
        }
    }
    if (boCanRestart)
    {
        LOG4_INFO("%s RestartWorkers!", m_oCurrentConf("server_name").c_str());
        RestartWorkers();
        m_iWaitToExitCounter = 0;
        MsgHead oOutMsgHead;
        MsgBody oOutMsgBody;
        OrdinaryResponse oRes;
        oRes.set_err_no(0);
        oRes.set_err_msg("OK");
        oOutMsgBody.set_body(oRes.SerializeAsString());
        oOutMsgHead.set_cmd(pData->cmd + 1);
        oOutMsgHead.set_seq(pData->seq);
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        SendTo(pData->stMsgShell, oOutMsgHead, oOutMsgBody);
        {//停止定时器
            ev_timer_stop (m_loop, watcher);
            delete pData;
            watcher->data = NULL;
            delete watcher;
            watcher = NULL;
        }
        return true;
    }
    if(m_iWaitToExitCounter < 6)
    {
        LOG4_INFO("CheckWorkersLoadEmptyRestart: m_iWaitToExitCounter(%d)",m_iWaitToExitCounter);
        ++m_iWaitToExitCounter;
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, 1.0 + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, watcher);
    }
    else
    {
        LOG4_WARN("CheckWorkersLoadEmptyRestart:ev_timer_stop,m_iWaitToExitCounter(%d)",m_iWaitToExitCounter);
        m_iWaitToExitCounter = 0;
        MsgHead oOutMsgHead;
        MsgBody oOutMsgBody;
        OrdinaryResponse oRes;
        oRes.set_err_no(1);
        oRes.set_err_msg("FAILED");
        oOutMsgBody.set_body(oRes.SerializeAsString());
        oOutMsgHead.set_cmd(pData->cmd + 1);
        oOutMsgHead.set_seq(pData->seq);
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        SendTo(pData->stMsgShell, oOutMsgHead, oOutMsgBody);
        ev_timer_stop (m_loop, watcher);
        delete pData;
        watcher->data = NULL;
        delete watcher;
        watcher = NULL;
    }
    return boCanRestart;
}

void Manager::RefreshServer(bool boForce)
{
    LOG4_TRACE("%s(m_iRefreshInterval %d, m_iLastRefreshCalc %d)", __FUNCTION__, m_iRefreshInterval, m_iLastRefreshCalc);
    int iErrno = 0;
    if (!boForce)
    {
    	++m_iLastRefreshCalc;
		if (m_iLastRefreshCalc < m_iRefreshInterval)
		{
			return;
		}
    }
    m_iLastRefreshCalc = 0;
    if (LoadConf())
    {
        if (m_oLastConf("log_level") != m_oCurrentConf("log_level")||boForce)
        {
            LOG4_DEBUG("update log_level:(%s) %s",m_oCurrentConf("log_level").c_str(),boForce?"force operation":"normal operation");
            m_oLogger.setLogLevel(m_iLogLevel);
            MsgHead oMsgHead;
            MsgBody oMsgBody;
            LogLevel oLogLevel;
            oLogLevel.set_log_level(m_iLogLevel);
            oMsgBody.set_body(oLogLevel.SerializeAsString());
            oMsgHead.set_cmd(CMD_REQ_SET_LOG_LEVEL);
            SendToWorker(oMsgHead, oMsgBody);
        }
        // 更新动态库配置或重新加载动态库
        if (m_oLastConf["so"].ToString() != m_oCurrentConf["so"].ToString() || boForce)
        {
        	std::string strSo = m_oCurrentConf["so"].ToString();
        	LOG4_INFO("update so:(%s) %s",strSo.c_str(),boForce?"force operation":"normal operation");
            MsgHead oMsgHead;
            MsgBody oMsgBody;
            oMsgBody.set_body(strSo);
            if (boForce)
			{
            	oMsgHead.set_cmd(CMD_REQ_RELOAD_SO| 0x08000000);
			}
            else
            {
            	oMsgHead.set_cmd(CMD_REQ_RELOAD_SO);
            }
            SendToWorker(oMsgHead, oMsgBody);
        }
        if (m_oLastConf["module"].ToString() != m_oCurrentConf["module"].ToString()|| boForce)
        {
        	std::string strModule = m_oCurrentConf["module"].ToString();
        	LOG4_INFO("update Module:(%s) %s",strModule.c_str(),boForce?"force operation":"normal operation");
            MsgHead oMsgHead;
            MsgBody oMsgBody;
            oMsgBody.set_body(strModule);
            if (boForce)
            {
            	oMsgHead.set_cmd(CMD_REQ_RELOAD_MODULE| 0x08000000);
            }
            else
            {
            	oMsgHead.set_cmd(CMD_REQ_RELOAD_MODULE);
            }
            SendToWorker(oMsgHead, oMsgBody);
        }
    }
    else
    {
        LOG4_INFO("LoadConf() error, please check the config file.", "");
    }
}

/**
 * @brief 上报节点状态信息
 * @return 上报是否成功
 * @note 节点状态信息结构如：
 * {
 *     "node_type":"ACCESS",
 *     "node_ip":"192.168.11.12",
 *     "node_port":9988,
 *     "access_ip":"120.234.2.106",
 *     "access_port":10001,
 *     "worker_num":10,
 *     "active_time":16879561651.06,
 *     "node":{
 *         "load":1885792, "connect":495873, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":495870
 *     },
 *     "worker":
 *     [
 *          {"load":655666, "connect":495873, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}},
 *          {"load":655235, "connect":485872, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}},
 *          {"load":585696, "connect":415379, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}}
 *     ]
 * }
 */
bool Manager::ReportToCenter()
{
    int iLoad = 0;
    int iConnect = 0;
    int iRecvNum = 0;
    int iRecvByte = 0;
    int iSendNum = 0;
    int iSendByte = 0;
    int iClientNum = 0;
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    util::CJsonObject oReportData;
    util::CJsonObject oMember;
    oReportData.Add("node_type", m_strNodeType);
    oReportData.Add("node_id", m_uiNodeId);
    oReportData.Add("node_ip", m_strHostForServer);
    oReportData.Add("node_port", m_iPortForServer);
    if (m_strGateway.size() > 0)
    {
        oReportData.Add("access_ip", m_strGateway);
    }
    else
    {
        oReportData.Add("access_ip", m_strHostForClient);
    }
    if (m_iGatewayPort > 0)
    {
        oReportData.Add("access_port", m_iGatewayPort);
    }
    else
    {
        oReportData.Add("access_port", m_iPortForClient);
    }
    oReportData.Add("worker_num", (int)m_mapWorker.size());
    oReportData.Add("active_time", ev_now(m_loop));
    oReportData.Add("node", util::CJsonObject("{}"));
    oReportData.Add("worker", util::CJsonObject("[]"));
    for (auto worker_iter:m_mapWorker)
    {
    	const tagWorkerAttr& oTagWorkerAttr = worker_iter.second;
        iLoad += oTagWorkerAttr.iLoad;
        iConnect += oTagWorkerAttr.iConnect;
        iRecvNum += oTagWorkerAttr.iRecvNum;
        iRecvByte += oTagWorkerAttr.iRecvByte;
        iSendNum += oTagWorkerAttr.iSendNum;
        iSendByte += oTagWorkerAttr.iSendByte;
        iClientNum += oTagWorkerAttr.iClientNum;
        oMember.Clear();
        oMember.Add("load", oTagWorkerAttr.iLoad);
        oMember.Add("connect", oTagWorkerAttr.iConnect);
        oMember.Add("recv_num", oTagWorkerAttr.iRecvNum);
        oMember.Add("recv_byte", oTagWorkerAttr.iRecvByte);
        oMember.Add("send_num", oTagWorkerAttr.iSendNum);
        oMember.Add("send_byte", oTagWorkerAttr.iSendByte);
        oMember.Add("client", oTagWorkerAttr.iClientNum);
        oReportData["worker"].Add(oMember);
    }
    oReportData["node"].Add("load", iLoad);
    oReportData["node"].Add("connect", iConnect);
    oReportData["node"].Add("recv_num", iRecvNum);
    oReportData["node"].Add("recv_byte", iRecvByte);
    oReportData["node"].Add("send_num", iSendNum);
    oReportData["node"].Add("send_byte", iSendByte);
    oReportData["node"].Add("client", iClientNum);
    oMsgBody.set_body(oReportData.ToString());
    oMsgHead.set_cmd(CMD_REQ_NODE_STATUS_REPORT);
    oMsgHead.set_seq(GetSequence());
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    LOG4_TRACE("%s():%s", __FUNCTION__, oReportData.ToString().c_str());

    for (auto center_iter:m_mapCenterMsgShell)
    {
        if (center_iter.second.iFd == 0)
        {
            oMsgHead.set_cmd(CMD_REQ_NODE_REGISTER);
            LOG4_TRACE("%s() cmd %d", __FUNCTION__, oMsgHead.cmd());
            AutoSend(center_iter.first, oMsgHead, oMsgBody);
        }
        else
        {
            oMsgHead.set_cmd(CMD_REQ_NODE_STATUS_REPORT);
            LOG4_TRACE("%s() cmd %d", __FUNCTION__, oMsgHead.cmd());
            SendTo(center_iter.second, oMsgHead, oMsgBody);
        }
    }
    return(true);
}

bool Manager::SendToWorker(const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    int iErrno = 0;
    int iWriteLen = 0;
    int iNeedWriteLen = 0;
    std::unordered_map<int, tagConnectionAttr*>::iterator worker_conn_iter;
    for (auto worker_iter = m_mapWorker.begin(); worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        worker_conn_iter = m_mapFdAttr.find(worker_iter->second.iControlFd);
        if (worker_conn_iter != m_mapFdAttr.end())
        {
        	tagConnectionAttr* pConn = worker_conn_iter->second;
        	pConn->pSendBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
        	pConn->pSendBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
            iNeedWriteLen = pConn->pSendBuff->ReadableBytes();
            LOG4_DEBUG("send cmd %d seq %llu to worker %d", oMsgHead.cmd(), oMsgHead.seq(), worker_iter->second.iWorkerIndex);
            iWriteLen = pConn->pSendBuff->WriteFD(worker_conn_iter->first, iErrno);
            if (iWriteLen < 0)
            {
                if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                {
                    LOG4_ERROR("send to fd %d error %d: %s",worker_conn_iter->first, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                }
                else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                {
                    worker_conn_iter->second->dActiveTime = ev_now(m_loop);
                    AddIoWriteEvent(pConn);
                }
            }
            else if (iWriteLen > 0)
            {
                worker_conn_iter->second->dActiveTime = ev_now(m_loop);
                if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                {
                    LOG4_DEBUG("send to worker %d success, data len %d, cmd %d seq %llu",worker_iter->second.iWorkerIndex, iWriteLen, oMsgHead.cmd(), oMsgHead.seq());
                    RemoveIoWriteEvent(pConn);
                }
                else    // 内容未写完，添加或保持监听fd写事件
                {
                    AddIoWriteEvent(pConn);
                }
            }
            worker_conn_iter->second->pSendBuff->Compact(32784);
        }
    }
    return(true);
}

bool Manager::SendToWorkerWithMod(uint32 uiModFactor,const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    int iErrno = 0;
    int iWriteLen = 0;
    int iNeedWriteLen = 0;
    int target_identify = uiModFactor % m_mapWorker.size();
    auto worker_iter = m_mapWorker.begin();
    for (int i = 0; worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        if (i == target_identify)
        {
            auto worker_conn_iter = m_mapFdAttr.find(worker_iter->second.iControlFd);
            if (worker_conn_iter != m_mapFdAttr.end())
            {
            	tagConnectionAttr* pConn = worker_conn_iter->second;
            	pConn->pSendBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
            	pConn->pSendBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
                iNeedWriteLen = pConn->pSendBuff->ReadableBytes();
                LOG4_DEBUG("send cmd %d seq %llu to worker %d", oMsgHead.cmd(), oMsgHead.seq(), worker_iter->second.iWorkerIndex);
                iWriteLen = pConn->pSendBuff->WriteFD(worker_conn_iter->first, iErrno);
                if (iWriteLen < 0)
                {
                    if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                    {
                        LOG4_ERROR("send to fd %d error %d: %s",worker_conn_iter->first, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                    }
                    else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                    {
                    	pConn->dActiveTime = ev_now(m_loop);
                        AddIoWriteEvent(pConn);
                    }
                }
                else if (iWriteLen > 0)
                {
                    worker_conn_iter->second->dActiveTime = ev_now(m_loop);
                    if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                    {
                        LOG4_DEBUG("send to worker %d success, data len %d, cmd %d seq %llu",worker_iter->second.iWorkerIndex, iWriteLen, oMsgHead.cmd(), oMsgHead.seq());
                        RemoveIoWriteEvent(pConn);
                    }
                    else    // 内容未写完，添加或保持监听fd写事件
                    {
                        AddIoWriteEvent(pConn);
                    }
                }
                pConn->pSendBuff->Compact(32784);
            }
            return(true);
        }
    }
    return(false);
}

bool Manager::DisposeDataFromWorker(const tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, util::CBuffer* pSendBuff)
{
    LOG4_DEBUG("%s(cmd %u, seq %u)", __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq());
    if (CMD_REQ_UPDATE_WORKER_LOAD == oInMsgHead.cmd())    // 新请求
    {
        std::unordered_map<int, int>::iterator iter = m_mapWorkerFdPid.find(stMsgShell.iFd);
        if (iter != m_mapWorkerFdPid.end())
        {
            util::CJsonObject oJsonLoad;
            oJsonLoad.Parse(oInMsgBody.body());
            SetWorkerLoad(iter->second, oJsonLoad);
        }
    }
    else if(CMD_REQ_SERVER_CONFIG == oInMsgHead.cmd())
    {
        MsgHead oOutMsgHead;
        MsgBody oOutMsgBody;
        OrdinaryResponse oRes;
        oRes.set_err_no(0);
        oRes.set_err_msg("OK");
        oOutMsgBody.set_body(oRes.SerializeAsString());
        oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
        oOutMsgHead.set_seq(oInMsgHead.seq());
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
        SendToWorkerWithMod(0,oInMsgHead, oInMsgBody);//只要发给第一个worker,让其修改配置文件
        m_iLastRefreshCalc = m_iRefreshInterval;//定时器到时时需要检查一次配置文件
        return(true);
    }
    else if (CMD_REQ_RELOAD_LOGIC_CONFIG == oInMsgHead.cmd())
    {
        MsgHead oOutMsgHead;
        MsgBody oOutMsgBody;
        OrdinaryResponse oRes;
        oRes.set_err_no(0);
        oRes.set_err_msg("OK");
        oOutMsgBody.set_body(oRes.SerializeAsString());
        oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
        oOutMsgHead.set_seq(oInMsgHead.seq());
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
        SendToWorker(oInMsgHead, oInMsgBody);
        return(true);
    }
    else if (CMD_REQ_NODE_STOP == oInMsgHead.cmd())
    {
        AddWaitToExitTaskEvent(stMsgShell,oInMsgHead.cmd(),oInMsgHead.seq());//退出节点
        return(true);
    }
    else if (CMD_REQ_NODE_RESTART_WORKERS == oInMsgHead.cmd())
    {
        AddWaitToExitTaskEvent(stMsgShell,oInMsgHead.cmd(),oInMsgHead.seq());//重启工作者
        return(true);
    }
    else
    {
        LOG4_WARN("unknow cmd %d from worker!", oInMsgHead.cmd());
    }
    return(true);
}

bool Manager::DisposeDataAndTransferFd(const tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,
		util::CBuffer* pSendBuff)
{
    LOG4_DEBUG("%s(cmd %u, seq %u)", __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq());
    int iErrno = 0;
    ConnectWorker oConnWorker;
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    OrdinaryResponse oRes;
    LOG4_TRACE("oInMsgHead.cmd() = %u, seq() = %u", oInMsgHead.cmd(), oInMsgHead.seq());
    if (oInMsgHead.cmd() == CMD_REQ_CONNECT_TO_WORKER)
    {
        if (oConnWorker.ParseFromString(oInMsgBody.body()))
        {
            std::unordered_map<int, tagWorkerAttr>::iterator worker_iter;
            for (worker_iter = m_mapWorker.begin();
                            worker_iter != m_mapWorker.end(); ++worker_iter)
            {
                if (oConnWorker.worker_index() == worker_iter->second.iWorkerIndex)
                {
                    // 这里假设传送文件描述符都成功，是因为对传送文件描述符成功后Manager顺利回复消息暂时没有比较好的解决办法
                    oRes.set_err_no(ERR_OK);
                    oRes.set_err_msg("OK");
                    oOutMsgBody.set_body(oRes.SerializeAsString());
                    oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
                    oOutMsgHead.set_seq(oInMsgHead.seq());
                    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
                    pSendBuff->Write(oOutMsgHead.SerializeAsString().c_str(), oOutMsgHead.ByteSize());
                    pSendBuff->Write(oOutMsgBody.SerializeAsString().c_str(), oOutMsgBody.ByteSize());
                    int iWriteLen = pSendBuff->WriteFD(stMsgShell.iFd, iErrno);

                    char szIp[16] = {0};
                    strncpy(szIp, "0.0.0.0", 16);   // 内网其他Server的IP不重要
                    int iErrno = send_fd_with_attr(worker_iter->second.iDataFd, stMsgShell.iFd, szIp, 16, util::CODEC_PROTOBUF);
                    //int iErrno = send_fd(worker_iter->second.iDataFd, stMsgShell.iFd);
                    if (iErrno != 0)
                    {
                        LOG4_ERROR("send_fd_with_attr error %d: %s!",
                                        iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                    }
                    return(false);
                }
            }
            if (worker_iter == m_mapWorker.end())
            {
                oRes.set_err_no(ERR_NO_SUCH_WORKER_INDEX);
                oRes.set_err_msg("no such worker index!");
            }
        }
        else
        {
            LOG4_ERROR("oConnWorker.ParseFromString() error!");
            oRes.set_err_no(ERR_PARASE_PROTOBUF);
            oRes.set_err_msg("oConnWorker.ParseFromString() error!");
            return(false);
        }
    }
    else
    {
        oRes.set_err_no(ERR_UNKNOWN_CMD);
        oRes.set_err_msg("unknow cmd!");
        LOG4_DEBUG("unknow cmd %d!", oInMsgHead.cmd());
        oOutMsgBody.set_body(oRes.SerializeAsString());
        oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
        oOutMsgHead.set_seq(oInMsgHead.seq());
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        pSendBuff->Write(oOutMsgHead.SerializeAsString().c_str(), oOutMsgHead.ByteSize());
        pSendBuff->Write(oOutMsgBody.SerializeAsString().c_str(), oOutMsgBody.ByteSize());
        pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
        return(false);
    }
    return(false);
}

bool Manager::DisposeDataFromCenter(const tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,tagConnectionAttr* pConn)
{
    LOG4_DEBUG("%s(cmd %u, seq %u)", __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq());
    util::CBuffer* pSendBuff = pConn->pSendBuff;
	util::CBuffer* pWaitForSendBuff = pConn->pWaitForSendBuff;
    int iErrno = 0;
    if (gc_uiCmdReq & oInMsgHead.cmd())    // 新请求，直接转发给Worker，并回复Center已收到请求
    {
        if (CMD_REQ_BEAT == oInMsgHead.cmd())   // center发过来的心跳包
        {
            MsgHead oOutMsgHead = oInMsgHead;
            MsgBody oOutMsgBody = oInMsgBody;
            oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
            SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
            return(true);
        }
        else if(CMD_REQ_SERVER_CONFIG == oInMsgHead.cmd())
        {
            MsgHead oOutMsgHead;
            MsgBody oOutMsgBody;
            OrdinaryResponse oRes;
            oRes.set_err_no(0);
            oRes.set_err_msg("OK");
            oOutMsgBody.set_body(oRes.SerializeAsString());
            oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
            oOutMsgHead.set_seq(oInMsgHead.seq());
            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
            SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
            SendToWorkerWithMod(0,oInMsgHead, oInMsgBody);//只要发给第一个worker
            m_iLastRefreshCalc = m_iRefreshInterval;//定时器到时时需要检查一次配置文件
            return(true);
        }
        else if (CMD_REQ_RELOAD_LOGIC_CONFIG == oInMsgHead.cmd())
        {
            MsgHead oOutMsgHead;
            MsgBody oOutMsgBody;
            OrdinaryResponse oRes;
            oRes.set_err_no(0);
            oRes.set_err_msg("OK");
            oOutMsgBody.set_body(oRes.SerializeAsString());
            oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
            oOutMsgHead.set_seq(oInMsgHead.seq());
            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
            SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
            SendToWorker(oInMsgHead, oInMsgBody);
            return(true);
        }
        else if (CMD_REQ_NODE_STOP == oInMsgHead.cmd())
        {
            AddWaitToExitTaskEvent(stMsgShell,oInMsgHead.cmd(),oInMsgHead.seq());//退出节点
            return(true);
        }
        else if (CMD_REQ_NODE_RESTART_WORKERS == oInMsgHead.cmd())
        {
            AddWaitToExitTaskEvent(stMsgShell,oInMsgHead.cmd(),oInMsgHead.seq());//重启工作者
            return(true);
        }
        SendToWorker(oInMsgHead, oInMsgBody);
        MsgHead oOutMsgHead;
        MsgBody oOutMsgBody;
        OrdinaryResponse oRes;
        oRes.set_err_no(0);
        oRes.set_err_msg("OK");
        oOutMsgBody.set_body(oRes.SerializeAsString());
        oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
        oOutMsgHead.set_seq(oInMsgHead.seq());
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        SendTo(stMsgShell, oOutMsgHead, oOutMsgBody);
    }
    else    // 回调
    {
        if (CMD_RSP_NODE_REGISTER == oInMsgHead.cmd()) //Manager这层只有向center注册会收到回调，上报状态不收回调或者收到回调不必处理
        {
            util::CJsonObject oNode(oInMsgBody.body());
            int iErrno = 0;
            oNode.Get("errcode", iErrno);
            if (0 == iErrno)
            {
                oNode.Get("node_id", m_uiNodeId);
                MsgHead oMsgHead;
                oMsgHead.set_cmd(CMD_REQ_REFRESH_NODE_ID);
                oMsgHead.set_seq(oInMsgHead.seq());
                oMsgHead.set_msgbody_len(oInMsgHead.msgbody_len());
                SendToWorker(oMsgHead, oInMsgBody);
                RemoveIoWriteEvent(pConn);
            }
            else
            {
                LOG4_WARN("register to center error, errcode %d!", iErrno);
            }
        }
        else if (CMD_RSP_CONNECT_TO_WORKER == oInMsgHead.cmd()) // 连接center时的回调
        {
            MsgHead oOutMsgHead;
            MsgBody oOutMsgBody;
            TargetWorker oTargetWorker;
            oTargetWorker.set_err_no(0);
            char szWorkerIdentify[64] = {0};
            snprintf(szWorkerIdentify, 64, "%s:%d", m_strHostForServer.c_str(), m_iPortForServer);
            oTargetWorker.set_worker_identify(szWorkerIdentify);
            oTargetWorker.set_node_type(GetNodeType());
            oTargetWorker.set_err_msg("OK");
            oOutMsgBody.set_body(oTargetWorker.SerializeAsString());
            oOutMsgHead.set_cmd(CMD_REQ_TELL_WORKER);
            oOutMsgHead.set_seq(GetSequence());
            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
            pSendBuff->Write(oOutMsgHead.SerializeAsString().c_str(), oOutMsgHead.ByteSize());
            pSendBuff->Write(oOutMsgBody.SerializeAsString().c_str(), oOutMsgBody.ByteSize());
            pSendBuff->Write(pWaitForSendBuff, pWaitForSendBuff->ReadableBytes());
            int iWriteLen = 0;
            int iNeedWriteLen = pSendBuff->ReadableBytes();
            iWriteLen = pSendBuff->WriteFD(stMsgShell.iFd, iErrno);
            if (iWriteLen < 0)
            {
                if (EAGAIN != iErrno && EINTR != iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
                {
                    LOG4_ERROR("send to fd %d error %d: %s",
                                    stMsgShell.iFd, iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
                    return(false);
                }
                else if (EAGAIN == iErrno)  // 内容未写完，添加或保持监听fd写事件
                {
                    AddIoWriteEvent(pConn);
                }
            }
            else if (iWriteLen > 0)
            {
                if (iWriteLen == iNeedWriteLen)  // 已无内容可写，取消监听fd写事件
                {
                    RemoveIoWriteEvent(pConn);
                }
                else    // 内容未写完，添加或保持监听fd写事件
                {
                    AddIoWriteEvent(pConn);
                }
            }
        }
    }
    return(true);
}

} /* namespace net */
