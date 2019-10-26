#ifndef SRC_CMDDATAPROXY_STEPGENERALREDIS_HPP_
#define SRC_CMDDATAPROXY_STEPGENERALREDIS_HPP_
#include <string>
#include "step/RedisStep.hpp"
#include "storage/dataproxy.pb.h"
#include "ProtoError.h"

namespace core
{

class StepGeneralRedis: public net::RedisStep
{
public:
    StepGeneralRedis(const std::string & strNode,uint32 &uiPingNode,const DataMem::MemOperate::RedisOperate& oRedisOperate,Step* pNextStep = NULL):
        net::RedisStep(pNextStep),m_strNode(strNode),m_uiPingNode(uiPingNode),m_oRedisOperate(oRedisOperate),m_iEmitNum(0){}
    virtual ~StepGeneralRedis(){};
    virtual net::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    virtual net::E_CMD_STATUS Callback(const redisAsyncContext *c,int status,redisReply* pReply);
private:
    const std::string m_strNode;
    uint32 &m_uiPingNode;
    DataMem::MemOperate::RedisOperate m_oRedisOperate;
    int m_iEmitNum;
};

}

#endif
