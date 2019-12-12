/*******************************************************************************
 * Project:  Net
 * @file     StepTellWorker.cpp
 * @brief    告知对端己方Worker进程信息
 * @author   cjy
 * @date:    2019年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepTellWorker.hpp"

namespace net
{

StepTellWorker::StepTellWorker(const tagMsgShell& stMsgShell)
    : m_iTimeoutNum(0), m_stMsgShell(stMsgShell)
{
}

StepTellWorker::~StepTellWorker()
{
}

E_CMD_STATUS StepTellWorker::Emit(int iErrno,const std::string& strErrMsg,const std::string& strErrShow)
{
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    TargetWorker oTargetWorker;
    oTargetWorker.set_err_no(0);
    oTargetWorker.set_worker_identify(net::GetWorkerIdentify());
    oTargetWorker.set_node_type(GetNodeType());
    oTargetWorker.set_err_msg("OK");
    oOutMsgBody.set_body(oTargetWorker.SerializeAsString());
    oOutMsgHead.set_cmd(CMD_REQ_TELL_WORKER);
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
    net::SendTo(m_stMsgShell, oOutMsgHead, oOutMsgBody);
    return(STATUS_CMD_RUNNING);
}

E_CMD_STATUS StepTellWorker::Callback(const tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data)
{
    TargetWorker oInTargetWorker;
    if (oInTargetWorker.ParseFromString(oInMsgBody.body()))
    {
        if (oInTargetWorker.err_no() == ERR_OK)
        {
            LOG4_DEBUG("AddMsgShell(%s, fd %d, seq %llu)!",
                            oInTargetWorker.worker_identify().c_str(), stMsgShell.iFd, stMsgShell.ulSeq);
            AddMsgShell(oInTargetWorker.worker_identify(), stMsgShell);
            AddNodeIdentify(oInTargetWorker.node_type(), oInTargetWorker.worker_identify());
            SendTo(stMsgShell);
            return(STATUS_CMD_COMPLETED);
        }
        else
        {
            LOG4_ERROR("error %d: %s!", oInTargetWorker.err_no(), oInTargetWorker.err_msg().c_str());
            return(STATUS_CMD_FAULT);
        }
    }
    else
    {
        LOG4_ERROR("error %d: WorkerLoad ParseFromString error!", ERR_PARASE_PROTOBUF);
        return(STATUS_CMD_FAULT);
    }
}

E_CMD_STATUS StepTellWorker::Timeout()
{
    ++m_iTimeoutNum;
    if (m_iTimeoutNum <= 3)
    {
        MsgHead oMsgHead;
        MsgBody oMsgBody;
        return(Emit(ERR_OK));
    }
    else
    {
        LOG4_ERROR("timeout %d times!", m_iTimeoutNum);
        return(STATUS_CMD_FAULT);
    }
}

} /* namespace net */
