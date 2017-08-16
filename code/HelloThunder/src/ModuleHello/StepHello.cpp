/*******************************************************************************
 * Project:  HelloThunder
 * @file     StepHello.cpp
 * @brief 
 * @author   chenjiayi
 * @date:    2017年10月21日
 * @note
 * Modify history:
 ******************************************************************************/
#include "../ModuleHello/StepHello.hpp"

namespace hello
{

StepHello::StepHello(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
	m_stReqMsgShell = stMsgShell;
	m_oInHttpMsg = oInHttpMsg;
}

StepHello::~StepHello()
{
}

thunder::E_CMD_STATUS StepHello::Emit(int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
    LOG4CPLUS_DEBUG_FMT(GetLogger(), "%s", __FUNCTION__);
    MsgHead oOutMsgHead;
	MsgBody oOutMsgBody;
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_msgbody_len(m_oReqMsgBody.ByteSize());
    GetLabor()->SendTo("192.168.2.129:30001.2", m_oReqMsgHead, m_oReqMsgBody);
    SessionHello* pSession = (SessionHello*)GetSession(123456);
    if (pSession == NULL)
    {
        pSession = new SessionHello(123456, 300.0);
        if(!RegisterCallback(pSession))
        {
            delete pSession;
            pSession = NULL;
            return(thunder::STATUS_CMD_FAULT);
        }
    }
    pSession->AddHelloNum(1);
    return(thunder::STATUS_CMD_RUNNING);
}

thunder::E_CMD_STATUS StepHello::Callback(
                    const thunder::MsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody,
                    void* data)
{
    Step::SendTo(m_stReqMsgShell, oInMsgHead, oInMsgBody);
    return(thunder::STATUS_CMD_COMPLETED);
}

thunder::E_CMD_STATUS StepHello::Callback(
					const thunder::MsgShell& stMsgShell,
					const HttpMsg& oHttpMsg,
					void* data)
{
	HttpStep::SendTo(m_stReqMsgShell, oHttpMsg);
    return(thunder::STATUS_CMD_COMPLETED);
}

thunder::E_CMD_STATUS StepHello::Timeout()
{
    return(thunder::STATUS_CMD_FAULT);
}

} /* namespace im */
