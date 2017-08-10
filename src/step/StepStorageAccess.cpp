/*
 * StepStorageAccess.cpp
 *
 *  Created on: 2017年8月1日
 *      Author: chen
 */
#include "ThunderError.hpp"
#include "ThunderDefine.hpp"
#include "StepStorageAccess.hpp"

namespace thunder
{

StepStorageAccess::StepStorageAccess(const std::string &strMsgSerial,const std::string &nodeType):
               m_nodeType(nodeType)
{
    m_strMsgSerial = strMsgSerial;
    m_uiTimeOut = 0;
    m_callbackSession = NULL;
    m_callbackStep = NULL;
    m_pSession = NULL;
    m_pStep = NULL;
    m_uiUpperStepSeq = 0;
}

thunder::E_CMD_STATUS StepStorageAccess::Emit(int iErrno , const std::string& strErrMsg , const std::string& strErrShow )
{
    MsgHead oOutHead;
    MsgBody oOutBody;
    oOutHead.set_seq(GetSequence());
    oOutHead.set_cmd(thunder::CMD_REQ_STORATE);
    oOutBody.set_body(m_strMsgSerial);
    oOutHead.set_msgbody_len(oOutBody.ByteSize());
    bool bRet = Step::SendToNext(m_nodeType,oOutHead,oOutBody);
    if (!bRet)
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(), "StepStorageAccess::Emit!  Send cmd[%u] seq[%u] send fail",
                        oOutHead.cmd(),oOutHead.seq());
        return thunder::STATUS_CMD_FAULT;
    }
    return thunder::STATUS_CMD_RUNNING;
}

thunder::E_CMD_STATUS StepStorageAccess::Timeout()
{
    LOG4CPLUS_TRACE_FMT(GetLogger(),"%s()",__FUNCTION__);
    ++m_uiTimeOut;
    if (m_uiTimeOut < 3)
    {
        return thunder::STATUS_CMD_RUNNING;
    }
    LOG4CPLUS_ERROR_FMT(GetLogger(),"%s()",__FUNCTION__);
    return thunder::STATUS_CMD_COMPLETED;
}

thunder::E_CMD_STATUS StepStorageAccess::Callback(
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
    DataMem::MemRsp oRsp;
    if(!oRsp.ParseFromString(oInMsgBody.body()))
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(), "parse protobuf data fault");
        return thunder::STATUS_CMD_FAULT;
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
        return thunder::STATUS_CMD_FAULT;
    }
    if (m_callbackSession)
    {
        if (m_pSession)
        {
            m_callbackSession(oRsp,m_pSession);
        }
        else if (m_strUpperSessionId.size() > 0 && m_strUpperSessionClassName.size() > 0)
        {
            thunder::Session* pSession = GetLabor()->GetSession(m_strUpperSessionId,m_strUpperSessionClassName);
            if (pSession)
            {
                m_callbackSession(oRsp,pSession);
            }
            else
            {
                LOG4CPLUS_ERROR_FMT(GetLogger(), "failed to Get Session(%s,%s)!",
                                m_strUpperSessionId.c_str(),m_strUpperSessionClassName.c_str());
                return thunder::STATUS_CMD_FAULT;
            }
        }
        else
        {
            LOG4CPLUS_ERROR_FMT(GetLogger(), "failed to Get Session(%s,%s)!",
                            m_strUpperSessionId.c_str(),m_strUpperSessionClassName.c_str());
            return thunder::STATUS_CMD_FAULT;
        }
    }
    else if (m_callbackStep)
	{
    	if (m_pStep)
    	{
    		m_callbackStep(oRsp,m_pStep);
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


}
