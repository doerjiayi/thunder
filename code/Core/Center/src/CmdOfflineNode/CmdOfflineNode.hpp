/*******************************************************************************
 * Project:  CenterServer
 * @file     CmdOfflineNode.hpp
 * @brief   下线节点
 * @author  chenjiayi
 * @date    2016年8月9日
 * @note    下线节点
 * Modify history:
 ******************************************************************************/
#ifndef CMD_OFFLINE_NODE_HPP_
#define CMD_OFFLINE_NODE_HPP_
#include "protocol/oss_sys.pb.h"
#include "server.pb.h"
#include "user_basic.pb.h"
#include "cmd/Cmd.hpp"
#include "../Comm.hpp"
#include "../NodeSession.h"

// "offline":0//挂起节点路由:0，关闭节点:1
enum offlineFlag
{
    eofflineFlag_suspend_routes = 0,
    eofflineFlag_close_note     = 1,
};

namespace core
{
class CmdOfflineNode: public net::Cmd
{
public:
    CmdOfflineNode();
    virtual ~CmdOfflineNode();
    virtual bool Init();
    virtual bool AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
private:
    bool SendOfflineToTarget(const std::string& sOfflineIdentify);
    bool parseMsg(const MsgBody& oInMsgBody);
    bool Response(int iErrno);
    NodeSession* pSess;
    bool boInit;
    net::tagMsgShell m_stMsgShell;
    MsgHead m_oInMsgHead;
    server::offline_node_req m_oOfflineNodeReq;
    server::offline_node_ack m_oOfflineNodeAck;
};
} /* namespace core */

#endif /* CMDTOLDWORKER_HPP_ */
