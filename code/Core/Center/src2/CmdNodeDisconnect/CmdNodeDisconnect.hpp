/*******************************************************************************
 * Project:  Center
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
#include "SessionOnlineNodes.hpp"

namespace coor
{

class CmdNodeDisconnect: public net::Cmd
{
public:
    CmdNodeDisconnect() = default;
    virtual ~CmdNodeDisconnect() = default;
    virtual bool Init();
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);

    virtual std::string ObjectName() const
    {
        return("coor::CmdNodeDisconnect");
    }

private:
    SessionOnlineNodes* m_pSessionOnlineNodes = nullptr;
};

} /* namespace coor */

#endif /* SRC_CMDNODEDISCONNECT_CMDNODEDISCONNECT_HPP_ */
