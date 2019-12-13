/*******************************************************************************
 * Project:  RobotServer
 * @file     CmdGetToken.hpp
 * @brief    token
 * @author   cjy
 * @date:    2017年1月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMD_GEN_TOKEN_HPP_
#define SRC_CMD_GEN_TOKEN_HPP_
#include "cmd/Cmd.hpp"
#include "common.pb.h"
#include "ImError.h"
#include "LogicSession.h"


namespace im
{

class CmdGetToken: public net::Cmd
{
public:
    CmdGetToken() = default;
    virtual ~CmdGetToken() = default;
    bool Init();
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
    void Response(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,int code);
}
;

} /* namespace im */

#endif /* SRC_CMD_GEN_TOKEN_HPP_ */
