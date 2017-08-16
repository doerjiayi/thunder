/*******************************************************************************
 * Project:  HelloThunder
 * @file     StepHttpRequest.cpp
 * @brief 
 * @author   chenjiayi
 * @date:    2017年11月3日
 * @note
 * Modify history:
 ******************************************************************************/
#include "../ModuleHello/StepHttpRequest.hpp"

namespace hello
{

StepHttpRequest::StepHttpRequest(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
	m_stReqMsgShell = stMsgShell;
	m_oInHttpMsg = oInHttpMsg;
}

StepHttpRequest::~StepHttpRequest()
{
}
thunder::E_CMD_STATUS StepHttpRequest::Emit(int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
    if (HttpGet("https://www.baidu.com"))
    {
//    	HttpPost
        return(thunder::STATUS_CMD_RUNNING);
    }
    else
    {
        return(thunder::STATUS_CMD_FAULT);
    }
}
thunder::E_CMD_STATUS StepHttpRequest::Callback(
                const thunder::MsgShell& stMsgShell,
                const HttpMsg& oHttpMsg,
                void* data)
{
    SendTo(m_stReqMsgShell, oHttpMsg);
    return(thunder::STATUS_CMD_COMPLETED);
}

thunder::E_CMD_STATUS StepHttpRequest::Timeout()
{
    return(thunder::STATUS_CMD_FAULT);
}



} /* namespace oss */
