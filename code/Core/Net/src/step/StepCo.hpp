#ifndef SRC_StepCo_HPP_
#define SRC_StepCo_HPP_

#include "StepState.hpp"
//协程步骤
namespace net
{

//协程访问步骤，在不同状态下可访问网络接口，在网路接口结果到达时，会进入下一个状态。可根据需求设置状态运行失败钩子函数和状态运行成功钩子函数。
class StepCo: public StepState
{
    typedef StepState super;
public:
	typedef bool (*StateFunc)(StepCo*);
	typedef void (*FinalFunc)(StepCo*);
	//开始状态步骤(参考StepState)
	StepCo();
	StepCo(const tagMsgShell& stInMsgShell, const MsgHead& oInMsgHead);
	StepCo(const tagMsgShell& stInMsgShell, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
	StepCo(const tagMsgShell& stInMsgShell, const HttpMsg& oInHttpMsg);
    virtual ~StepCo(){}//在StepState析构函数回收StepState的成员
    void Init();
    void AddCoroutinueFunc(FinalFunc func);
    void SetSuccFunc(FinalFunc func){m_SuccFunc = func;}
    void SetFailFunc(FinalFunc func){m_FailFunc = func;}
    virtual E_CMD_STATUS Timeout();
    virtual E_CMD_STATUS Emit(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");
    bool SetNextState(uint32 s)
    {
        if (s < m_uiStateVecNum && (m_StateCoFuncVec[s])){m_uiNextState = s;return true;}
        return false;
    }
	virtual void OnSucc(){if (m_SuccFunc) m_SuccFunc(this);}
	virtual void OnFail(){if (m_FailFunc) m_FailFunc(this);}
	bool CoroutineYield();
protected:
private:
	FinalFunc m_StateCoFuncVec[StepStateVecSize];//协程状态过程函数
	int m_curCoid;
    FinalFunc m_SuccFunc;
    FinalFunc m_FailFunc;
};

}
typedef net::StepState::StateFunc StepStateFunc;
typedef net::StepState::FinalFunc StepFinalFunc;

#endif
