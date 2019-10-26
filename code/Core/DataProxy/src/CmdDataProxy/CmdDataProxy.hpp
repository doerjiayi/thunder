/*******************************************************************************
 * Project:  DataProxy
 * @file     CmdDataProxy.hpp
 * @brief    数据访问代理
 * @author   cjy
 * @date:    2016年12月8日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDDATAPROXY_CMDDATAPROXY_HPP_
#define SRC_CMDDATAPROXY_CMDDATAPROXY_HPP_

#include <dirent.h>
#include <map>
#include <set>
#include "cmd/Cmd.hpp"
#include "ProtoError.h"
#include "storage/StorageOperator.hpp"
#include "storage/dataproxy.pb.h"
#include "SessionSyncDbData.hpp"
#include "DataProxySession.h"
#include "StepReadFromRedis.hpp"
#include "StepSendToDbAgent.hpp"
#include "StepWriteToRedis.hpp"
#include "StepReadFromRedisForWrite.hpp"

namespace core
{

class CmdDataProxy: public net::Cmd
{
public:
    CmdDataProxy();
    virtual ~CmdDataProxy();
    virtual bool Init();
    virtual bool AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody);
protected:
    //数据检查和填写
    bool CheckRequest(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemOperate& oMemOperate);
    //读写操作
    bool RedisOnly(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemOperate& oMemOperate);
    bool DbOnly(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemOperate& oMemOperate);
    bool ReadEither(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemOperate& oMemOperate);
    bool WriteBoth(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,DataMem::MemOperate& oMemOperate);
    bool UpdateBothWithDataset(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,DataMem::MemOperate& oMemOperate);
private:
    bool Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,int iErrno, const std::string& strErrMsg);
    bool GetNodeSession(const DataMem::MemOperate& oMemOperate);
private:
    char m_pErrBuff[gc_iErrBuffSize];
    DataProxySession* m_pProxySess;
    SessionRedisNode* m_pRedisNodeSession;
public:
    StepSendToDbAgent* pStepSendToDbAgent;
    StepReadFromRedis* pStepReadFromRedis;
    StepWriteToRedis* pStepWriteToRedis;
    StepReadFromRedisForWrite* pStepReadFromRedisForWrite;
};

} /* namespace core */

#endif /* SRC_CMDDATAPROXY_CMDDATAPROXY_HPP_ */
