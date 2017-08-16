/*******************************************************************************
* * Project:  Thunder
 * @file     CmdBeat.hpp
 * @brief    心跳包响应
 * @author   cjy
 * @date:    2017年11月5日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMD_SYS_CMD_CMDBEAT_HPP_
#define SRC_CMD_SYS_CMD_CMDBEAT_HPP_

#include "cmd/Cmd.hpp"

namespace thunder
{

class CmdBeat : public Cmd
{
public:
    CmdBeat();
    virtual ~CmdBeat();
    virtual bool AnyMessage(
                    const MsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace thunder */

#endif /* SRC_CMD_SYS_CMD_CMDBEAT_HPP_ */
