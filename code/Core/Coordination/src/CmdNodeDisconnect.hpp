/*******************************************************************************
 * Project:  Beacon
 * @file     CmdNodeDisconnect.hpp
 * @brief 
 * @author   bwar
 * @date:    Feb 14, 2017
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDNODEDISCONNECT_CMDNODEDISCONNECT_HPP_
#define SRC_CMDNODEDISCONNECT_CMDNODEDISCONNECT_HPP_

#include "Comm.hpp"

namespace coor
{

class CmdNodeDisconnect: public net::Cmd
{
public:
    CmdNodeDisconnect(int32 iCmd);
    virtual ~CmdNodeDisconnect();

    virtual bool Init();
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);

    virtual std::string ObjectName() const
    {
        return("coor::CmdNodeReport");
    }

private:
    std::shared_ptr<SessionOnlineNodes> m_pSessionOnlineNodes;
};

} /* namespace coor */

#endif /* SRC_CMDNODEDISCONNECT_CMDNODEDISCONNECT_HPP_ */
