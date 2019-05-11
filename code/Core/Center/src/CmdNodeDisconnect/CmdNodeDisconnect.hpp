/*******************************************************************************
 * Project:  CenterServer
 * @file     CmdNodeDisconnect.hpp
 * @brief    节点连接断开
 * @author   cjy
 * @date:    2015年9月18日
 * @note     各人节点注册到CENTER，后面断开，CENTER处理节点断开
 * Modify history:
 ******************************************************************************/
#ifndef CMD_NODE_DISCONNECT_HPP_
#define CMD_NODE_DISCONNECT_HPP_
#include <iostream>
#include "cmd/Cmd.hpp"
#include "protocol/oss_sys.pb.h"
#include "util/json/CJsonObject.hpp"
#include "../NodeSession.h"

namespace core
{
/**
 * @brief   节点连接断开
 * @author  hsc
 * @date    2015年8月9日
 * @note    各个模块启动时需要向CENTER进行注册，如果节点连接断开，CENTER处理相关逻辑
 */
class CmdNodeDisconnect : public net::Cmd
{
public:
    CmdNodeDisconnect();
    virtual ~CmdNodeDisconnect();
    virtual bool Init();
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
private:
    NodeSession* pSess;
    bool boInit;
};

} /* namespace core */

#endif /* CMDTOLDWORKER_HPP_ */
