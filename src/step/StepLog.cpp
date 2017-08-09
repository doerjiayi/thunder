/*******************************************************************************
 * Project:  Thunder
 * @file     StepLog.cpp
 * @brief
 * @author   cjy
 * @date:    2017年5月31日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepLog.hpp"

namespace thunder
{

StepLog::StepLog(const ::google::protobuf::Message &behaviourLog,oss::uint32 logType,oss::Step* pNextStep,const std::string &nodeType)
    : oss::Step(pNextStep),m_nodeType(nodeType)
{
    m_behaviour_log.set_log_info(behaviourLog.SerializeAsString());
    m_behaviour_log.set_log_type(logType);
}

StepLog::~StepLog()
{
}

oss::E_CMD_STATUS StepLog::Emit(int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
	LOG4_DEBUG("%s()",__FUNCTION__);
    if(iErrno != ERR_OK)///被调用前就已经失败了
    {
        NextStep(iErrno,strErrMsg,strErrShow);
        return(oss::STATUS_CMD_FAULT);
    }
    if (m_behaviour_log.ByteSize() == 0)
    {
        LOG4_WARN("%s() m_behaviour_log.ByteSize() == 0",__FUNCTION__);
        return(oss::STATUS_CMD_FAULT);
    }
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    oMsgBody.set_body(m_behaviour_log.SerializeAsString());
    oMsgHead.set_cmd(oss::CMD_REQ_LOG);
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    oMsgHead.set_seq(GetSequence());
    if (!SendToNext(m_nodeType, oMsgHead, oMsgBody))
    {
        LOG4_ERROR("%s() send to %s error!",__FUNCTION__,m_nodeType.c_str());
        NextStep(oss::ERR_SERVERINFO,"send to server error!","server busy");
        return(oss::STATUS_CMD_FAULT);
    }
    return(oss::STATUS_CMD_RUNNING);
}

oss::E_CMD_STATUS StepLog::Callback(
        const oss::tagMsgShell& stMsgShell,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody,
        void* data)
{
    BehaviourLog::behaviour_log_ack oBehaviourLogAck;
    if(!oBehaviourLogAck.ParseFromString(oInMsgBody.body()))
    {
        LOG4_ERROR("%s() oBehaviourLogAck parse protobuf data fault",__FUNCTION__);
        NextStep(oss::ERR_SERVERINFO,"parse protobuf data fault","server busy");
        return oss::STATUS_CMD_FAULT;
    }
    LOG4_TRACE("%s() oBehaviourLogAck(%s)!",__FUNCTION__,oBehaviourLogAck.DebugString().c_str());
    NextStep(ERR_OK,"OK","");
    return oss::STATUS_CMD_COMPLETED;
}

oss::E_CMD_STATUS StepLog::Timeout()
{
    LOG4_TRACE("%s() StepLog timeout!",__FUNCTION__);
    NextStep(oss::ERR_TIMEOUT, "timeout", "timeout");
    return oss::STATUS_CMD_FAULT;
}


} /* namespace robot */
