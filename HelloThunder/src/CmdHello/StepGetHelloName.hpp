/*******************************************************************************
 * Project:  HelloThunder
 * @file     StepGetHelloName.hpp
 * @brief 
 * @author   chenjiayi
 * @date:    2017年10月23日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDHELLO_STEPGETHELLONAME_HPP_
#define SRC_CMDHELLO_STEPGETHELLONAME_HPP_
#include "step/RedisStep.hpp"

namespace hello
{

class StepGetHelloName: public thunder::RedisStep
{
public:
    StepGetHelloName(
                    const thunder::MsgShell& stMsgShell,
                    const MsgHead& oInMsgHead);
    virtual ~StepGetHelloName();
    virtual thunder::E_CMD_STATUS Callback(
                    const redisAsyncContext *c,
                    int status,
                    redisReply* pReply);
private:
    thunder::MsgShell m_stMsgShell;
    MsgHead m_oMsgHead;
};

} /* namespace hello */

#endif /* SRC_CMDHELLO_STEPGETHELLONAME_HPP_ */
