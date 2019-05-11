
#ifndef SRC_StepState_HPP_
#define SRC_StepState_HPP_
#include <map>
#include <algorithm>
#include <functional>
#include "step/Step.hpp"
#include "step/HttpStep.hpp"
#include "session/Session.hpp"
#include "storage/RedisOperator.hpp"
#include "storage/DbOperator.hpp"
#include "storage/MemOperator.hpp"

namespace net
{
#define StepStateVecSize (20)

//状态访问步骤，在不同状态下可访问网络接口，在网路接口结果到达时，会进入下一个状态。可根据需求设置状态运行失败钩子函数和状态运行成功钩子函数。
class StepState: public HttpStep
{
public:
	typedef bool (*StateFunc)(StepState*);
	typedef void (*FinalFunc)(StepState*);
	StepState();
	StepState(const tagMsgShell& stInMsgShell, const MsgHead& oInMsgHead);
	StepState(const tagMsgShell& stInMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
	StepState(const tagMsgShell& stInMsgShell, const HttpMsg& oInHttpMsg);
    virtual ~StepState();
    void Init();
    void Init(uint32 uiTimeOutMax,uint8 uiTimeOutRetry){m_uiTimeOutMax = uiTimeOutMax;m_uiTimeOutRetry = uiTimeOutRetry;}
    virtual void SetStepDesc(const std::string &s){m_strStepDesc = s;}
    void AddStateFunc(StateFunc func);
    void SetSuccFunc(FinalFunc func){m_SuccFunc = func;}
    void SetFailFunc(FinalFunc func){m_FailFunc = func;}
    net::E_CMD_STATUS Callback(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data = NULL);
    E_CMD_STATUS Callback(const tagMsgShell& stMsgShell,const HttpMsg& oHttpMsg,void* data = NULL);
    E_CMD_STATUS Timeout();
    E_CMD_STATUS Emit(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    bool SetNextState(int s = -1)//设置本状态的下一个状态过程（依然需要等待本状态的回调）,如果不指定状态默认下一个状态
    {
    	if (s < 0)s = m_uiState+1;
		if ((uint32)s < m_uiStateVecNum && (m_StateVec[(uint32)s])){m_uiNextState = s;return true;}
		return false;
    }
    bool JumpNextState(int s = -1)//直接跳到下一个状态（不需要等待本状态的回调）,如果不指定状态默认下一个状态
    {
    	if (s < 0)s = m_uiState+1;
		if (SetNextState(s))
		{
			m_boJumpState = true;
			return true;
		}
		return false;
    }
    virtual void Finish(){m_uiState = m_uiStateVecNum;}//本状态结束后结束步骤（不需要等待回调）

    uint32 GetCurrentState()const{return m_uiState;}//当前状态
    uint32 GetLastState()const{return m_uiLastState;}//上一个状态
    uint32 GetStageNum()const{return m_uiStateVecNum;}//总状态数
	virtual void OnSucc(){if (m_SuccFunc) m_SuccFunc(this);}
	virtual void OnFail(){if (m_FailFunc) m_FailFunc(this);}
	//错误信息
	int m_iErrno;std::string m_strErrMsg;std::string m_strStepDesc;
	//响应数据
	HttpMsg m_oResHttpMsg;MsgHead m_oResMsgHead;MsgBody m_oResMsgBody;
protected:
	uint32 m_uiTimeOutCounter;//已超时次数
	uint32 m_uiTimeOutMax;//最多超时次数
	uint8 m_uiTimeOutRetry;//是否超时重新尝试过程

	uint32 m_uiStateVecNum;//状态过程计数
    uint32 m_uiState;//状态机
    uint32 m_uiLastState; //上一个状态机
    int m_uiNextState;//指定下一个状态机
    bool m_boJumpState;//不等待本状态回调，直接运行下一个状态
private:
	StateFunc m_StateVec[StepStateVecSize];//状态过程函数
    FinalFunc m_SuccFunc;
    FinalFunc m_FailFunc;
};

}
typedef net::StepState::StateFunc StepStateFunc;
typedef net::StepState::FinalFunc StepFinalFunc;


#endif
