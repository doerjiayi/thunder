/*******************************************************************************
 * Project:  Net
 * @file     CmdUpdateNodeId.hpp
 * @brief    更新Worker的节点ID
 * @author   cjy
 * @date:    2015年9月18日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMD_SYS_CMD_CMDUPDATENODEID_HPP_
#define SRC_CMD_SYS_CMD_CMDUPDATENODEID_HPP_

#include "cmd/Cmd.hpp"

namespace net
{

class CmdUpdateNodeId: public Cmd
{
public:
    CmdUpdateNodeId();
    virtual ~CmdUpdateNodeId();
    virtual bool AnyMessage(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace net */

#endif /* SRC_CMD_SYS_CMD_CMDUPDATENODEID_HPP_ */
