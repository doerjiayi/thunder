/*******************************************************************************
 * Project:  DbAgent
 * @file     CmdDbOper.hpp
 * @brief 
 * @author   cjy
 * @date:    2016年3月28日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDDBOPER_CMDDBOPER_HPP_
#define SRC_CMDDBOPER_CMDDBOPER_HPP_
#include "dbi/MysqlDbi.hpp"
#include "util/json/CJsonObject.hpp"
#include "cmd/Cmd.hpp"
#include "storage/dataproxy.pb.h"
#include "DbAgentSession.h"

namespace core
{

class CmdDbOper: public net::Cmd
{
public:
    CmdDbOper();
    virtual ~CmdDbOper();
    virtual bool Init();
    virtual bool AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody);
protected:
    DbAgentSession* pDbAgentSession;
};

} /* namespace core */

#endif /* SRC_CMDDBOPER_CMDDBOPER_HPP_ */
