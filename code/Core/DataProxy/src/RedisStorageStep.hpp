/*******************************************************************************
 * Project:  DataProxyServer
 * @file     StorageStep.hpp
 * @brief 
 * @author   cjy
 * @date:    2016年3月21日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDDATAPROXY_REDISSTORAGESTEP_HPP_
#define SRC_CMDDATAPROXY_REDISSTORAGESTEP_HPP_
#include "ProtoError.h"
#include "step/RedisStep.hpp"
#include "storage/dataproxy.pb.h"

namespace core
{

/**
 * @brief redis数据结构与DB表数据之间关系
 */
enum E_TABLE_RELATIVE
{
    RELATIVE_TABLE      = 0,    //!< RELATIVE_TABLE   redis hash与表字段一一对应
    RELATIVE_DATASET    = 1,    //!< RELATIVE_DATASET redis数据为表的各字段值序列化成record
    RELATIVE_JOIN       = 2,    //!< RELATIVE_JOIN    redis数据为表的某些字段用冒号“:”连接而成
};

enum E_NOSQL_TYPE
{
    NOSQL_T_HASH                = 1,    ///< nosql hash
    NOSQL_T_SET                 = 2,    ///< nosql set
    NOSQL_T_KEYS                = 3,    ///< nosql keys
    NOSQL_T_STRING              = 4,    ///< nosql string
    NOSQL_T_LIST                = 5,    ///< nosql list
    NOSQL_T_SORT_SET            = 6,    ///< nosql sort set
};

class RedisStorageStep: public net::RedisStep
{
public:
    RedisStorageStep(Step* pNextStep = NULL);
    RedisStorageStep(const net::tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, Step* pNextStep = NULL);
    virtual ~RedisStorageStep();
protected:
    bool Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,int iErrno, const std::string& strErrMsg);
    bool Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemRsp& oRsp);
};

} /* namespace core */

#endif /* SRC_CMDDATAPROXY_REDISSTORAGESTEP_HPP_ */
