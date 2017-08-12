/*
 * StepNodeAccess.cpp
 *
 *  Created on: 2017年8月1日
 *      Author: chen
 */
#include "StepNodeAccess.hpp"

#include "ThunderError.hpp"
#include "ThunderDefine.hpp"

namespace thunder
{

StepNodeAccess::StepNodeAccess(const std::string &strMsgSerial):m_strMsgSerial(strMsgSerial)
{
    m_uiTimeOut = 0;
    m_uiTimeOutCounter = 3;
    m_uiRetrySend = 0;
    m_storageCallbackSession = NULL;
    m_storageCallbackStep = NULL;
    m_standardCallbackSession = NULL;
    m_standardCallbackStep = NULL;
    m_pSession = NULL;
    m_pStep = NULL;
    m_uiUpperStepSeq = 0;
    m_uiCmd = 0;
}

thunder::E_CMD_STATUS StepNodeAccess::Emit(int iErrno , const std::string& strErrMsg , const std::string& strErrShow )
{
	if (m_uiCmd > 0 && m_strNodeType.size() > 0)
	{
		MsgHead oOutHead;
		MsgBody oOutBody;
		oOutHead.set_seq(GetSequence());
		oOutHead.set_cmd(m_uiCmd);
		oOutBody.set_body(m_strMsgSerial);
		oOutHead.set_msgbody_len(oOutBody.ByteSize());
		bool bRet = Step::SendToNext(m_strNodeType,oOutHead,oOutBody);
		if (!bRet)
		{
			LOG4CPLUS_ERROR_FMT(GetLogger(), "StepNodeAccess::Emit!  Send cmd[%u] seq[%u] send fail",
							oOutHead.cmd(),oOutHead.seq());
			return thunder::STATUS_CMD_FAULT;
		}
		return thunder::STATUS_CMD_RUNNING;
	}
	else
	{
		LOG4CPLUS_ERROR_FMT(GetLogger(), "error m_uiCmd(%u) or m_strNodeType(%s)",m_uiCmd,m_strNodeType.c_str());
		return thunder::STATUS_CMD_FAULT;
	}
}

thunder::E_CMD_STATUS StepNodeAccess::Timeout()
{
    LOG4CPLUS_TRACE_FMT(GetLogger(),"%s()",__FUNCTION__);
    ++m_uiTimeOut;
    if (m_uiTimeOut < m_uiTimeOutCounter)
    {
    	if (m_uiRetrySend > 0)
    	{
    		return Emit();
    	}
        return thunder::STATUS_CMD_RUNNING;
    }
    LOG4CPLUS_ERROR_FMT(GetLogger(),"%s()",__FUNCTION__);
    return thunder::STATUS_CMD_COMPLETED;
}

thunder::E_CMD_STATUS StepNodeAccess::Callback(
        const thunder::MsgShell& stMsgShell,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody,
        void* data)
{
    LOG4CPLUS_TRACE_FMT(GetLogger(),"%s()",__FUNCTION__);
    if(thunder::CMD_RSP_SYS_ERROR == oInMsgHead.cmd())
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),"system response error");
        return thunder::STATUS_CMD_FAULT;
    }
    if (m_storageCallbackSession)
    {
    	DataMem::MemRsp oRsp;
    	if (!DecodeMemRsp(oRsp,oInMsgBody))
    	{
    		LOG4CPLUS_ERROR_FMT(GetLogger(),"DecodeMemRsp error");
			return thunder::STATUS_CMD_FAULT;
    	}
    	if (GetSession() == NULL)
    	{
    		LOG4CPLUS_ERROR_FMT(GetLogger(), "failed to Get Session(%s,%s)!",
							m_strUpperSessionId.c_str(),m_strUpperSessionClassName.c_str());
			return thunder::STATUS_CMD_FAULT;
    	}
    	m_storageCallbackSession(oRsp,m_pSession);
    }
    else if (m_storageCallbackStep)
	{
    	if (m_pStep)
    	{
    		DataMem::MemRsp oRsp;
			if (!DecodeMemRsp(oRsp,oInMsgBody))
			{
				LOG4CPLUS_ERROR_FMT(GetLogger(),"DecodeMemRsp error");
				return thunder::STATUS_CMD_FAULT;
			}
    		m_storageCallbackStep(oRsp,m_pStep);
    	}
    	else
    	{
    		LOG4CPLUS_ERROR_FMT(GetLogger(), "m_pStep null");
			return thunder::STATUS_CMD_FAULT;
    	}
	}
    else if (m_standardCallbackSession)
    {
    	if (GetSession() == NULL)
		{
			LOG4CPLUS_ERROR_FMT(GetLogger(), "failed to Get Session(%s,%s)!",
							m_strUpperSessionId.c_str(),m_strUpperSessionClassName.c_str());
			return thunder::STATUS_CMD_FAULT;
		}
    	m_standardCallbackSession(oInMsgHead,oInMsgBody,data,m_pSession);
    }
    else if (m_standardCallbackStep)
    {
    	if (m_pStep)
		{
    		m_standardCallbackStep(oInMsgHead,oInMsgBody,data,m_pStep);
		}
		else
		{
			LOG4CPLUS_ERROR_FMT(GetLogger(), "m_pStep null");
			return thunder::STATUS_CMD_FAULT;
		}
    }
	else
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(), "m_callbackSession and m_callbackStep null!");
        return thunder::STATUS_CMD_FAULT;
    }
    return thunder::STATUS_CMD_COMPLETED;
}

bool StepNodeAccess::DecodeMemRsp(DataMem::MemRsp &oRsp,const MsgBody& oInMsgBody)
{
	if(!oRsp.ParseFromString(oInMsgBody.body()))
	{
		LOG4CPLUS_ERROR_FMT(GetLogger(), "parse protobuf data fault");
		return false;
	}
	//读存储出错
	if(0 != oRsp.err_no())
	{
		if(oRsp.err_msg().size() > 0)
		{
			LOG4CPLUS_ERROR_FMT(GetLogger(), "StepLoadDBConfig::Callback error %d: %s!",
							oRsp.err_no(),oRsp.err_msg().c_str());
		}
		else
		{
			LOG4CPLUS_ERROR_FMT(GetLogger(), "StepLoadDBConfig::Callback error %d!",oRsp.err_no());
		}
		return false;
	}
	return true;
}


}
