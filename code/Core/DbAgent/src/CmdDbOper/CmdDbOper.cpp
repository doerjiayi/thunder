/*******************************************************************************
 * Project:  DbAgent
 * @file     CmdDbOper.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年3月28日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdDbOper.hpp"
#include "DbAgentSession.h"

MUDULE_CREATE(core::CmdDbOper);

namespace core
{

CmdDbOper::CmdDbOper():pDbAgentSession(NULL)
{
}

CmdDbOper::~CmdDbOper()
{
}

bool CmdDbOper::Init()
{
    pDbAgentSession = GetDbAgentSession();
    if (!pDbAgentSession)
    {
        LOG4_ERROR("GetDbAgentSession error!");
        return false;
    }
    net::GetCustomConf().Get("sync",pDbAgentSession->m_uiSync);
	if (pDbAgentSession->m_uiSync) LOG4_TRACE("sync db connection");
	else LOG4_TRACE("async db connection");
    return(true);
}

bool CmdDbOper::AnyMessage(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    DataMem::MemOperate oMemOperate;
    if (!oMemOperate.ParseFromString(oInMsgBody.body()))
    {
        LOG4_ERROR("Parse protobuf msg error!");
        pDbAgentSession->Response(stMsgShell,oInMsgHead,ERR_PARASE_PROTOBUF, "DataMem::MemOperate ParseFromString() failed!");
        return(false);
    }
    util::CMysqlDbi* pMasterDbi = NULL;
	util::CMysqlDbi* pSlaveDbi = NULL;
    bool bConnected = pDbAgentSession->GetDbConnection(stMsgShell,oInMsgHead,oMemOperate, &pMasterDbi, &pSlaveDbi);
    if (bConnected)
    {
        LOG4_TRACE("succeed in getting db connection");
        if (0 == pDbAgentSession->m_uiSync)
        {//异步
        	int iResult = 0;
			if (DataMem::MemOperate::DbOperate::SELECT == oMemOperate.db_operate().query_type())
			{
				iResult = pDbAgentSession->AsyncQuery(stMsgShell,oInMsgHead,oMemOperate, pSlaveDbi);
				if (0 == iResult)
				{//异步返回结果
					return(true);
				}
				else
				{
					iResult = pDbAgentSession->AsyncQuery(stMsgShell,oInMsgHead,oMemOperate, pMasterDbi);
					if (0 == iResult)
					{//异步返回结果
						return(true);
					}
					else
					{
						pDbAgentSession->Response(stMsgShell,oInMsgHead,pMasterDbi->GetErrno(), pMasterDbi->GetError());
						return(false);
					}
				}
			}
			else
			{
				if (NULL == pMasterDbi)
				{//异步返回结果
					pDbAgentSession->Response(stMsgShell,oInMsgHead,pSlaveDbi->GetErrno(), pSlaveDbi->GetError());
					return(false);
				}
				iResult = pDbAgentSession->AsyncQuery(stMsgShell,oInMsgHead,oMemOperate, pMasterDbi);
				if (0 == iResult)
				{//异步返回结果
					return(true);
				}
				else
				{
					pDbAgentSession->Response(stMsgShell,oInMsgHead,pMasterDbi->GetErrno(), pMasterDbi->GetError());
					return(false);
				}
			}
        }
        else
        {
        	int iResult = 0;
			if (DataMem::MemOperate::DbOperate::SELECT == oMemOperate.db_operate().query_type())
			{
				iResult = pDbAgentSession->SyncQuery(stMsgShell,oInMsgHead,oMemOperate, pSlaveDbi);
				if (0 == iResult)
				{//同步返回结果
					return(true);
				}
				else
				{
					iResult = pDbAgentSession->SyncQuery(stMsgShell,oInMsgHead,oMemOperate, pMasterDbi);
					if (0 == iResult)
					{//同步返回结果
						return(true);
					}
					else
					{
						pDbAgentSession->Response(stMsgShell,oInMsgHead,pMasterDbi->GetErrno(), pMasterDbi->GetError());
						return(false);
					}
				}
			}
			else
			{
				if (NULL == pMasterDbi)
				{//同步返回结果
					pDbAgentSession->Response(stMsgShell,oInMsgHead,pSlaveDbi->GetErrno(), pSlaveDbi->GetError());
					return(false);
				}
				iResult = pDbAgentSession->SyncQuery(stMsgShell,oInMsgHead,oMemOperate, pMasterDbi);
				if (0 == iResult)
				{//同步返回结果
					return(true);
				}
				else
				{
					pDbAgentSession->Response(stMsgShell,oInMsgHead,pMasterDbi->GetErrno(), pMasterDbi->GetError());
					return(false);
				}
			}
        }
    }
    return(false);
}




} /* namespace core */
