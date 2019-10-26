/*******************************************************************************
 * Project:  DataProxyServer
 * @file     StepWriteToRedis.hpp
 * @brief 
 * @author   cjy
 * @date:    2016年3月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDDATAPROXY_STEPWRITETOREDIS_HPP_
#define SRC_CMDDATAPROXY_STEPWRITETOREDIS_HPP_
#include "ProtoError.h"
#include "RedisStorageStep.hpp"
#include "SessionRedisNode.hpp"
#include "StepSetTtl.hpp"

namespace core
{

class StepWriteToRedis: public RedisStorageStep
{
public:
    StepWriteToRedis(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,
                    const DataMem::MemOperate::RedisOperate& oRedisOperate,
                    SessionRedisNode* pNodeSession, Step* pNextStep = NULL);
    virtual ~StepWriteToRedis();
    virtual net::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    virtual net::E_CMD_STATUS Callback(const redisAsyncContext *c,int status,redisReply* pReply);
protected:
    bool ReadReplyArrayForHashWithoutDataSet(redisReply* pReply);
    bool ReadReplyArrayWithoutDataSet(redisReply* pReply);
    bool ReadSubArray(redisReply* pReply,int &iDataLen,DataMem::MemRsp &oRsp);
private:
    net::tagMsgShell m_stMsgShell;
    MsgHead m_oReqMsgHead;
    DataMem::MemOperate::RedisOperate m_oRedisOperate;
    std::string m_strMasterNode;
    std::string m_strSlaveNode;
public:
    SessionRedisNode* m_pNodeSession;
};

} /* namespace core */

#endif /* SRC_CMDDATAPROXY_STEPWRITETOREDIS_HPP_ */
