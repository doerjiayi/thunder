/*******************************************************************************
 * Project:  DbAgent
 * @file     CmdPgOper.hpp
 * @brief 
 * @author   cjy
 * @date:    2016年3月28日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDDBOPER_CMDDBOPER_HPP_
#define SRC_CMDDBOPER_CMDDBOPER_HPP_
#include "util/json/CJsonObject.hpp"
#include "cmd/Cmd.hpp"
#include "storage/dataproxy.pb.h"
#include "PgAgentSession.h"

namespace core
{

class CmdPgOper: public net::Cmd
{
public:
    CmdPgOper();
    virtual ~CmdPgOper();
    virtual bool Init();
    virtual bool AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody);
    PgAgentSession* pDbAgentSession;
};

} /* namespace core */

#endif /* SRC_CMDDBOPER_CMDDBOPER_HPP_ */
