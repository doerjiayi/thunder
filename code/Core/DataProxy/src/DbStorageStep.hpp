/*******************************************************************************
 * Project:  DataProxyServer
 * @file     DbStorageStep.hpp
 * @brief 
 * @author   cjy
 * @date:    2016年3月21日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDDATAPROXY_DBSTORAGESTEP_HPP_
#define SRC_CMDDATAPROXY_DBSTORAGESTEP_HPP_
#include "ProtoError.h"
#include "step/Step.hpp"
#include "storage/dataproxy.pb.h"

namespace core
{

class DbStorageStep: public net::Step
{
public:
    DbStorageStep(Step* pNextStep = NULL);
    DbStorageStep(const net::tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, Step* pNextStep = NULL);
    virtual ~DbStorageStep();
protected:
    bool Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,int iErrno, const std::string& strErrMsg);
    bool Response(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,const DataMem::MemRsp& oRsp);
};

} /* namespace core */

#endif /* SRC_CMDDATAPROXY_DBSTORAGESTEP_HPP_ */
