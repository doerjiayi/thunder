/*******************************************************************************
 * Project:  DbAgent
 * @file     CmdLocatePgData.hpp
 * @brief 
 * @author   cjy
 * @date:    2016年4月18日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDLOCATEDATA_CMDLOCATEDATA_HPP_
#define SRC_CMDLOCATEDATA_CMDLOCATEDATA_HPP_
#include "util/json/CJsonObject.hpp"
#include "cmd/Cmd.hpp"
#include "storage/dataproxy.pb.h"
#include "PgAgentSession.h"

namespace core
{

class CmdLocatePgData: public net::Cmd
{
public:
    CmdLocatePgData();
    virtual ~CmdLocatePgData();
    virtual bool Init();
    virtual bool AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody);
    PgAgentSession* pDbAgentSession;
};

} /* namespace core */

#endif /* SRC_CMDLOCATEDATA_CMDLOCATEDATA_HPP_ */
