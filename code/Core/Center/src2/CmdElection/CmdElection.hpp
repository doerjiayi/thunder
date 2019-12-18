/*******************************************************************************
 * Project:  Center
 * @file     CmdElection.hpp
 * @brief 
 * @author   bwar
 * @date:    2019-1-6
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDNODEREPORT_CMDNODEREPORT_HPP_
#define SRC_CMDNODEREPORT_CMDNODEREPORT_HPP_

#include "Comm.hpp"
#include "SessionOnlineNodes.hpp"

namespace coor
{

class CmdElection: public net::Cmd
{
public:
    CmdElection() = default;
    virtual ~CmdElection() = default;

    virtual bool Init();
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);

private:
    SessionOnlineNodes* m_pSessionOnlineNodes = nullptr;
};

} /* namespace coor */

#endif /* SRC_CMDNODEREPORT_CMDNODEREPORT_HPP_ */
