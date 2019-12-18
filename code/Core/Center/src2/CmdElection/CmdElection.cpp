/*******************************************************************************
 * Project:  Center
 * @file     CmdElection.cpp
 * @brief 
 * @author   bwar
 * @date:    2019-1-6
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdElection.hpp"
#include "coor.pb.h"

MUDULE_CREATE(coor::CmdElection);

namespace coor
{

bool CmdElection::Init()
{
    return(true);
}

bool CmdElection::AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oMsgHead,const MsgBody& oMsgBody)
{
    Election oElection;
    if (nullptr == m_pSessionOnlineNodes)
    {
        m_pSessionOnlineNodes = (SessionOnlineNodes*)net::GetSession("coor::SessionOnlineNodes");
        if (nullptr == m_pSessionOnlineNodes)
        {
            LOG4_ERROR("no session node found!");
            return(false);
        }
    }
    if (oElection.ParseFromString(oMsgBody.body()))
    {
        m_pSessionOnlineNodes->AddCenterBeat(GetLabor()->GetConnectIdentify(stMsgShell), oElection);
        return(true);
    }
    else
    {
        LOG4_ERROR("failed to parse election info from MsgBody.data()!");
        return(false);
    }
}

} /* namespace coor */
