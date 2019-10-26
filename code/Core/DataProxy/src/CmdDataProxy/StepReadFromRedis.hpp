/*******************************************************************************
 * Project:  DataProxyServer
 * @file     StepReadFromRedis.hpp
 * @brief 
 * @author   cjy
 * @date:    2016年3月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDDATAPROXY_STEPREADFROMREDIS_HPP_
#define SRC_CMDDATAPROXY_STEPREADFROMREDIS_HPP_
#include "ProtoError.h"
#include "RedisStorageStep.hpp"
#include "SessionRedisNode.hpp"

namespace core
{

class StepReadFromRedis: public RedisStorageStep
{
public:
    StepReadFromRedis(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,
                    const DataMem::MemOperate::RedisOperate& oRedisOperate,SessionRedisNode* pNodeSession,
                    bool bIsDataSet = false,const util::CJsonObject* pTableFields = NULL,const std::string& strKeyField = "",
                    Step* pNextStep = NULL);
    virtual ~StepReadFromRedis();
    virtual net::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    virtual net::E_CMD_STATUS Callback(const redisAsyncContext *c,int status,redisReply* pReply);
protected:
    bool ReadReplyArray(redisReply* pReply);
    bool ReadReplyArrayWithDataSet(redisReply* pReply);
    bool ReadReplyArrayWithoutDataSet(redisReply* pReply);
    bool ReadSubArray(redisReply* pReply,int &iDataLen,DataMem::MemRsp &oRsp);//读取子数组

    bool ReadReplyHash(redisReply* pReply);
    bool ReadReplyArrayForHashWithDataSet(redisReply* pReply);
    bool ReadReplyArrayForHashWithoutDataSet(redisReply* pReply);
    bool ReadReplyArrayForHgetallWithDataSet(redisReply* pReply);
    bool ReadReplyArrayForHgetallWithoutDataSet(redisReply* pReply);
private:
    net::tagMsgShell m_stMsgShell;
    MsgHead m_oReqMsgHead;
    DataMem::MemOperate::RedisOperate m_oRedisOperate;
    bool m_bIsDataSet;
    util::CJsonObject m_oTableFields;
    std::string m_strKeyField;
    std::string m_strMasterNode;
    std::string m_strSlaveNode;
    int m_iReadNum;
    int m_iTableFieldNum;

public:
    SessionRedisNode* m_pNodeSession;
};

} /* namespace core */

#endif /* SRC_CMDDATAPROXY_STEPREADFROMREDIS_HPP_ */
