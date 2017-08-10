
#ifndef SRC_StepStorageAccess_HPP_
#define SRC_StepStorageAccess_HPP_
#include "step/Step.hpp"
#include "session/Session.hpp"
#include "ThunderError.hpp"
#include "storage/RedisOperator.hpp"
#include "storage/DbOperator.hpp"
#include "storage/MemOperator.hpp"

namespace thunder
{

typedef int (*CallbackSession)(const DataMem::MemRsp &oRsp,thunder::Session*pSession);

class StepStorageAccess: public thunder::Step
{
public:
    StepStorageAccess(const std::string &strMsgSerial,const std::string &nodeType = "PROXY");
    virtual ~StepStorageAccess(){};
    virtual thunder::E_CMD_STATUS Callback(
        const thunder::MsgShell& stMsgShell,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody,
        void* data = NULL);
    virtual thunder::E_CMD_STATUS Timeout();
    virtual thunder::E_CMD_STATUS Emit(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    void SetCallBack(CallbackSession callbackSession,thunder::Session* pSession,bool boPermanentSession)
    {
        m_callbackSession = callbackSession;
        if (boPermanentSession)
        {
            m_pSession = pSession;
        }
        else
        {
            m_strUpperSessionId = pSession->GetSessionId();
            m_strUpperSessionClassName = pSession->GetSessionClass();
        }
    }
private:
    uint32 m_uiTimeOut;
    std::string m_strMsgSerial;
    std::string m_nodeType;
    CallbackSession m_callbackSession;

    std::string m_strUpperSessionId;
    std::string m_strUpperSessionClassName;
    thunder::Session* m_pSession;

    uint32 m_uiUpperStepSeq;
};

}


#endif
