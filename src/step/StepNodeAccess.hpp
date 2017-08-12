
#ifndef SRC_StepNodeAccess_HPP_
#define SRC_StepNodeAccess_HPP_
#include "step/Step.hpp"
#include "session/Session.hpp"
#include "ThunderError.hpp"
#include "storage/RedisOperator.hpp"
#include "storage/DbOperator.hpp"
#include "storage/MemOperator.hpp"

namespace thunder
{

class StepNodeAccess: public thunder::Step
{
public:
    StepNodeAccess(const std::string &strMsgSerial);
    virtual ~StepNodeAccess(){};
    virtual thunder::E_CMD_STATUS Callback(
        const thunder::MsgShell& stMsgShell,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody,
        void* data = NULL);
    virtual thunder::E_CMD_STATUS Timeout();
    virtual thunder::E_CMD_STATUS Emit(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    //Session对象异步访问存储回调
    void SetStorageCallBack(StorageCallbackSession callback,thunder::Session* pSession,
    		bool boPermanentSession,const std::string &nodeType = "PROXY",uint32 uiCmd = thunder::CMD_REQ_STORATE)
    {
    	m_storageCallbackSession = callback;
        if (boPermanentSession)
        {
            m_pSession = pSession;
        }
        else
        {
            m_strUpperSessionId = pSession->GetSessionId();
            m_strUpperSessionClassName = pSession->GetSessionClass();
        }
        m_strNodeType = nodeType;
        m_uiCmd = uiCmd;
    }
    //Step对象异步访问存储回调
    void SetStorageCallBack(StorageCallbackStep callback,thunder::Step* pStep,
    		const std::string &nodeType = "PROXY",uint32 uiCmd = thunder::CMD_REQ_STORATE)
	{
    	m_storageCallbackStep = callback;
    	m_pStep = pStep;
    	m_strNodeType = nodeType;
    	m_uiCmd = uiCmd;
	}
    //Session对象异步访问一般节点回调
    void SetStandardCallBack(StandardCallbackSession callback,thunder::Session* pSession,
    		bool boPermanentSession,const std::string &nodeType,uint32 uiCmd)
    {
    	m_standardCallbackSession = callback;
		if (boPermanentSession)
		{
			m_pSession = pSession;
		}
		else
		{
			m_strUpperSessionId = pSession->GetSessionId();
			m_strUpperSessionClassName = pSession->GetSessionClass();
		}
		m_strNodeType = nodeType;
		m_uiCmd = uiCmd;
    }
    //Session对象异步访问一般节点回调
	void SetStandardCallBack(StandardCallbackStep callback,thunder::Step* pStep,
			const std::string &nodeType,uint32 uiCmd)
	{
		m_standardCallbackStep = callback;
		m_pStep = pStep;
		m_strNodeType = nodeType;
		m_uiCmd = uiCmd;
	}
	void SetTimtOut(uint32 uiTimeOutCounter)
	{
		m_uiTimeOutCounter = uiTimeOutCounter;
	}
private:
	bool DecodeMemRsp(DataMem::MemRsp &oRsp,const MsgBody& oInMsgBody);

	thunder::Session* GetSession()
	{
		if (NULL == m_pSession)
		{
			if (m_strUpperSessionId.size() > 0 && m_strUpperSessionClassName.size() > 0)
			{
				m_pSession = GetLabor()->GetSession(m_strUpperSessionId,m_strUpperSessionClassName);
			}
		}
		return m_pSession;
	}
    uint32 m_uiTimeOut;
    uint32 m_uiTimeOutCounter;
    uint32 m_uiRetrySend;

    std::string m_strMsgSerial;
    std::string m_strNodeType;
    uint32 m_uiCmd;
    StorageCallbackSession m_storageCallbackSession;
    StorageCallbackStep m_storageCallbackStep;

    StandardCallbackSession m_standardCallbackSession;
    StandardCallbackStep m_standardCallbackStep;

    std::string m_strUpperSessionId;
    std::string m_strUpperSessionClassName;
    thunder::Session* m_pSession;
    thunder::Step* m_pStep;

    uint32 m_uiUpperStepSeq;
};

}


#endif
