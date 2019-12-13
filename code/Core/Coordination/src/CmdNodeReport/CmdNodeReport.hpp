/*******************************************************************************
 * Project:  Beacon
 * @file     CmdNodeReport.hpp
 * @brief 
 * @author   bwar
 * @date:    Feb 14, 2017
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDNODEREPORT_CMDNODEREPORT_HPP_
#define SRC_CMDNODEREPORT_CMDNODEREPORT_HPP_

#include "Comm.hpp"
#include "SessionOnlineNodes.hpp"

namespace coor
{

class CmdNodeReport: public net::Cmd
{
public:
    CmdNodeReport():m_pSessionOnlineNodes(nullptr){}
    virtual ~CmdNodeReport();

    virtual bool Init();
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);

private:
    SessionOnlineNodes* m_pSessionOnlineNodes;
};

} /* namespace coor */

#endif /* SRC_CMDNODEREPORT_CMDNODEREPORT_HPP_ */
