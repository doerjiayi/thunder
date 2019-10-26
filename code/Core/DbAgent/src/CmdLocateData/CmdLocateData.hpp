/*******************************************************************************
 * Project:  DbAgent
 * @file     CmdLocateData.hpp
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
#include "DbAgentSession.h"

namespace core
{

class CmdLocateData: public net::Cmd
{
public:
    CmdLocateData();
    virtual ~CmdLocateData();
    virtual bool Init();
    virtual bool AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody);
protected:
    bool Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,int iErrno, const std::string& strErrMsg);
private:
    DbAgentSession* pDbAgentSession;
};

} /* namespace core */

#endif /* SRC_CMDLOCATEDATA_CMDLOCATEDATA_HPP_ */
