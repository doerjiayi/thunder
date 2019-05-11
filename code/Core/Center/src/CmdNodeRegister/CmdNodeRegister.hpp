/*******************************************************************************
 * Project:  CenterServer
 * @file     CmdNodeRegister.hpp
 * @brief    被告知Worker信息
 * @author   cjy
 * @date:    2015年8月9日
 * @note     A连接B成功后，A将己方的Worker信息A_attr（节点类型和Worker唯一标记）告知B，
 * B收到A后将A_attr登记起来，并将己方的Worker信息B_attr回复A
 * Modify history:
 ******************************************************************************/
#ifndef CMD_NODE_REG_HPP_
#define CMD_NODE_REG_HPP_
#include "protocol/oss_sys.pb.h"
#include "cmd/Cmd.hpp"
#include "../Comm.hpp"
#include "../NodeSession.h"

namespace core
{
/**
 * @brief   节点注册
 * @author  chenjiayi
 * @date    2015年8月9日
 * @note    各个模块启动时需要向CENTER进行注册
 */
class CmdNodeRegister: public net::Cmd
{
public:
    CmdNodeRegister();
    virtual ~CmdNodeRegister();
    virtual bool Init();
    virtual bool AnyMessage(const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
private:
    //注册节点应答
    bool Response(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,int iRet,int node_id=0);
    NodeSession* pSess;
    bool boInit;
};
} /* namespace core */

#endif /* CMDTOLDWORKER_HPP_ */
