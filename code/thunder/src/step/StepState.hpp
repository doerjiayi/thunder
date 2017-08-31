
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
	typedef std::map<uint32,StateFunc> StateMap;
	StepState();
    virtual ~StepState(){};
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
    virtual void StateInit() = 0;
    void StateAdd(uint32 s,StateFunc func)
	{
    	m_StateMap.insert(std::make_pair(s,func));
    	if (s < m_uiState)//初始状态为最小状态
    	{
    		m_uiLastState = m_uiState = s;
    	}
	}
    void StateSet(uint32 s)
    {
    	if (m_StateMap.find(s) != m_StateMap.end())
    	{
    		m_uiState = s;
    	}
    }
    static bool Launch(NodeLabor* pLabor,StepState *step);
    virtual thunder::E_CMD_STATUS Emit(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");
	void SetTimeOut(uint32 uiTimeOutCounter)
	{
		m_uiTimeOutCounter = uiTimeOutCounter;
	}
protected:
	thunder::MsgShell m_ResponseMsgShell;

	HttpMsg m_oResponseHttpMsg;
	MsgHead m_oResponseMsgHead;
	MsgBody m_oResponseMsgBody;
private:
    uint32 m_uiTimeOut;
    uint32 m_uiTimeOutCounter;
    uint32 m_uiRetryTry;
    uint32 m_uiState;
    uint32 m_uiLastState;
	StateMap m_StateMap;
};

}


#endif
