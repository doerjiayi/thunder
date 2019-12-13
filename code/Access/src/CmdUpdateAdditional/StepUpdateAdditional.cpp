/*******************************************************************************
 * Project:  AsyncServer
 * @file     StepKickedOffLine.cpp
 * @brief    更新Additional通知
 * @author   ty
 * @date:    2019年9月17日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepUpdateAdditional.hpp"
#include "user.pb.h"
#include "common.pb.h"
#include "user_basic.pb.h"
#include "ImCw.h"
#include "ImError.h"

namespace im
{

StepUpdateAdditional::StepUpdateAdditional(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody)
	:m_iTimeoutNum(0),m_stMsgShell(stMsgShell),m_oInMsgHead(oInMsgHead),m_oInMsgBody(oInMsgBody)
{
}

StepUpdateAdditional::~StepUpdateAdditional()
{
}

net::E_CMD_STATUS StepUpdateAdditional::Emit(int iErrno, const std::string& strErrMsg,const std::string& strErrShow)
{
	LOG4_DEBUG("%s()", __FUNCTION__);
	if (m_oInMsgBody.has_additional() && m_basicinfo.ParseFromString(m_oInMsgBody.additional()))
	{
		LOG4_DEBUG("basicinfo[%s]",m_basicinfo.DebugString().c_str());
		char szIdentify[32] = {0};
		snprintf(szIdentify, sizeof(szIdentify), "%u", m_oInMsgBody.session_id());
		if (m_basicinfo.prohibit() & BAN_LOGIN)
		{
			LOG4_DEBUG("BAN_LOGIN:userid(%u)",m_basicinfo.userid());
			if(m_stMsgShell.iFd > 0)
			{	
				g_pLabor->AbandonConnect(szIdentify);
				g_pLabor->Disconnect(m_stMsgShell,false);
			}
		}
		else
		{
			LOG4_DEBUG("SetClientData:userid(%u)",m_basicinfo.userid());
			g_pLabor->GetMsgShell(szIdentify,m_stMsgShell);
			util::CBuffer oBuff;
			oBuff.Write(m_basicinfo.SerializeAsString().c_str(),m_basicinfo.ByteSize());
			g_pLabor->SetClientData(m_stMsgShell,&oBuff);//设置客户端数据
		}
	}
	else
	{
		LOG4_DEBUG("additional parse errror ");
	}
    return (net::STATUS_CMD_COMPLETED);
}

net::E_CMD_STATUS StepUpdateAdditional::Callback(const net::tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead, const MsgBody& oInMsgBody,
                void* data)
{
    return (net::STATUS_CMD_COMPLETED);
}

net::E_CMD_STATUS StepUpdateAdditional::Timeout()
{
    return (net::STATUS_CMD_COMPLETED);
}

} /* namespace im */
