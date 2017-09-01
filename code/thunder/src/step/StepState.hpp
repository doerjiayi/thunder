
#ifndef SRC_StepState_HPP_
#define SRC_StepState_HPP_
#include <map>
#include "step/Step.hpp"
#include "step/HttpStep.hpp"
#include "session/Session.hpp"
#include "ThunderError.hpp"
#include "storage/RedisOperator.hpp"
#include "storage/DbOperator.hpp"
#include "storage/MemOperator.hpp"

namespace thunder
{

class StepState: public thunder::HttpStep
{
public:
	typedef thunder::E_CMD_STATUS (*StateFunc)(StepState*);
#define StepStateSize (10)
	StepState();
    virtual ~StepState(){};
    virtual void InitState();
    virtual thunder::E_CMD_STATUS State0(){LOG4_TRACE("%s() uiState(%u)",__FUNCTION__,m_uiState);return thunder::STATUS_CMD_COMPLETED;}
    virtual thunder::E_CMD_STATUS State1(){LOG4_TRACE("%s() uiState(%u)",__FUNCTION__,m_uiState);return thunder::STATUS_CMD_COMPLETED;}
    virtual thunder::E_CMD_STATUS State2(){LOG4_TRACE("%s() uiState(%u)",__FUNCTION__,m_uiState);return thunder::STATUS_CMD_COMPLETED;}
    virtual thunder::E_CMD_STATUS State3(){LOG4_TRACE("%s() uiState(%u)",__FUNCTION__,m_uiState);return thunder::STATUS_CMD_COMPLETED;}
    virtual thunder::E_CMD_STATUS State4(){LOG4_TRACE("%s() uiState(%u)",__FUNCTION__,m_uiState);return thunder::STATUS_CMD_COMPLETED;}
    virtual thunder::E_CMD_STATUS State5(){LOG4_TRACE("%s() uiState(%u)",__FUNCTION__,m_uiState);return thunder::STATUS_CMD_COMPLETED;}
    virtual thunder::E_CMD_STATUS State6(){LOG4_TRACE("%s() uiState(%u)",__FUNCTION__,m_uiState);return thunder::STATUS_CMD_COMPLETED;}
	virtual thunder::E_CMD_STATUS State7(){LOG4_TRACE("%s() uiState(%u)",__FUNCTION__,m_uiState);return thunder::STATUS_CMD_COMPLETED;}
	virtual thunder::E_CMD_STATUS State8(){LOG4_TRACE("%s() uiState(%u)",__FUNCTION__,m_uiState);return thunder::STATUS_CMD_COMPLETED;}
	virtual thunder::E_CMD_STATUS State9(){LOG4_TRACE("%s() uiState(%u)",__FUNCTION__,m_uiState);return thunder::STATUS_CMD_COMPLETED;}

    virtual thunder::E_CMD_STATUS Callback(
        const thunder::MsgShell& stMsgShell,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody,
        void* data = NULL);
    virtual thunder::E_CMD_STATUS Callback(
                        const MsgShell& stMsgShell,
                        const HttpMsg& oHttpMsg,
                        void* data = NULL);
    virtual thunder::E_CMD_STATUS Timeout();
    void SetNextState(uint32 s)
    {
    	if (s < StepStateSize)
    	{
    		m_uiState = s;
    	}
    }
    uint32 GetLastState()const{return m_uiLastState;}
    static bool Launch(NodeLabor* pLabor,StepState *step);
    virtual thunder::E_CMD_STATUS Emit(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");
	void SetTimeOut(uint32 uiTimeOutCounter)
	{
		m_uiTimeOutCounter = uiTimeOutCounter;
	}
protected:
	thunder::MsgShell m_ResponseMsgShell;
	HttpMsg m_oResHttpMsg;
	MsgHead m_oResMsgHead;
	MsgBody m_oResMsgBody;

	StateFunc m_StateVec[StepStateSize];
private:
    uint32 m_uiTimeOut;
    uint32 m_uiTimeOutCounter;
    uint32 m_uiRetryTry;
    uint32 m_uiState;
    uint32 m_uiLastState;
};

}


#endif
