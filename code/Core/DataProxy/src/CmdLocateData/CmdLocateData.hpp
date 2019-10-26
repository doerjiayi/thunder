/*******************************************************************************
 * Project:  DataProxyServer
 * @file     CmdLocateData.hpp
 * @brief    查询数据落在集群的哪个节点
 * @author   cjy
 * @date:    2016年4月18日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDLOCATEDATA_CMDLOCATEDATA_HPP_
#define SRC_CMDLOCATEDATA_CMDLOCATEDATA_HPP_
#include "cmd/Cmd.hpp"
#include "storage/StorageOperator.hpp"
#include "storage/dataproxy.pb.h"
#include "ProtoError.h"
#include "step/StepNode.hpp"
#include "SessionRedisNode.hpp"
#include "DataProxySession.h"

namespace core
{

/**
 * @brief 查询数据落在集群的哪个节点
 * @note
 */
class CmdLocateData: public net::Cmd
{
public:
    CmdLocateData();
    virtual ~CmdLocateData();
    virtual bool Init();
    virtual bool AnyMessage(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody);
protected:
    bool ReadNosqlClusterConf();
    bool ReadNosqlDbRelativeConf();
    bool RedisOnly(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemOperate& oMemOperate);
    bool DbOnly(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const MsgBody& oInMsgBody);
    bool RedisAndDb(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const MsgBody& oInMsgBody);
    void Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,int iErrno, const std::string& strErrMsg);
private:
    char m_pErrMsg[256];
    uint32 m_ScanSyncDataTime;
	uint32 m_ScanSyncDataLastTime;
    DataProxySession* m_pProxySess;
};

} /* namespace core */

#endif /* SRC_CMDLOCATEDATA_CMDLOCATEDATA_HPP_ */
