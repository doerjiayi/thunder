/*******************************************************************************
 * Project:  Net
 * @file     Step.hpp
 * @brief    异步步骤基类
 * @author   cjy
 * @date:    2019年7月27日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef STEP_HPP_
#define STEP_HPP_

#include <map>
#include <set>
#include "NetError.hpp"
#include "NetDefine.hpp"
#include "labor/Labor.hpp"
#include "cmd/CW.hpp"
#include "protocol/msg.pb.h"
#include "protocol/http.pb.h"
#include "session/Session.hpp"

namespace net
{

class Worker;
class Cmd;
class RedisStep;

//参数基类（自定义参数类则继承参数基类）
struct StepParam
{
	StepParam(){};
	virtual ~StepParam(){};
};

/**
 * @brief 步骤基类
 * @note 全异步Server框架基于状态机设计模式实现，Step类就是框架的状态机基类。Step类还保存了业务层连接标
 * 识到连接层实际连接的对应关系，业务层通过Step类可以很方便地把数据发送到指定连接。
 */
class Step
{
public:
    Step(Step* pNextStep = NULL);
    Step(const tagMsgShell& stReqMsgShell, Step* pNextStep = NULL);
    Step(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, Step* pNextStep = NULL);
    Step(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody,Step* pNextStep = NULL);
    void Init();
    virtual ~Step();
    /**
     * @brief 提交，发出
     * @note 注册了一个回调步骤之后执行Emit()就开始等待回调。 Emit()大部分情况下不需要传递参数，
     * 三个带缺省值的参数是为了让上一个通用Step执行出错时将错误码带入下一步业务逻辑Step，由具体
     * 业务逻辑处理。
     * @param iErrno  错误码
     * @param strErrMsg 错误信息
     * @param strErrShow 展示给用户的错误描述
     * @return 执行状态
     */
    virtual E_CMD_STATUS Emit(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "") = 0;
    /**
     * @brief 步骤回调函数
     * @note 满足某个条件，比如监控某个文件描述符fd的EPOLLIN事件和EPOLLERR事件，当这个fd的
     * 这两类事件中的任意一类到达时则会调用Callback()。具体使用到哪几个参数与业务逻辑有关，前三个
     * 参数的使用概率高。
     * @param stMsgShell 消息外壳，回调可通过消息外壳原路回复消息，若消息不是来源于网络IO，则
     * 消息外壳为空
     * @param oInMsgHead 消息头。
     * @param oInMsgBody 消息体。
     * @param data 数据指针，基本网络IO时为空，有专用数据时使用，比如redis的reply。
     */
    virtual E_CMD_STATUS Callback(const tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data = NULL) = 0;
    /**
     * @brief 步骤超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;
    /**
     * @brief 预设数据
     */
    virtual void SetData(StepParam* data){m_data = data;}
    StepParam * GetData(){return m_data;}
    StepParam *m_data;
public:
    /**
     * @brief 设置步骤最近刷新时间
     */
    void SetActiveTime(ev_tstamp dActiveTime){m_dActiveTime = dActiveTime;}
    /**
     * @brief 获取步骤刷新时间
     */
    ev_tstamp GetActiveTime() const{return(m_dActiveTime);}
    /**
     * @brief 设置步骤超时时间
     */
    void SetTimeout(ev_tstamp dTimeout){m_dTimeout = dTimeout;}
    /**
     * @brief 获取步骤超时时间
     */
    ev_tstamp GetTimeout() const{return(m_dTimeout);}
    /**
     * @brief 延迟超时时间
     */
    void DelayTimeout();
    const std::string& ClassName() const{return(m_strClassName);}
    void SetClassName(const std::string& strClassName){m_strClassName = strClassName;}
    /**
     * @brief 是否注册了回调
     * @return 注册结果
     */
    bool IsRegistered() const{return(m_bRegistered);}
	/**
	 * @brief 获取当前Step的序列号
	 * @note 获取当前Step的序列号，对于同一个Step实例，每次调用GetSequence()获取的序列号是相同的。
	 * @return 序列号
	 */
	uint32 GetSequence();
	/*
	 * 运行计时器
	 * */
	RunClock m_RunClock;
public:
    /**
     * @brief 登记回调步骤
     * @note  登记回调步骤。如果StepA.Callback()调用之后仍有后续步骤，则需在StepA.Callback()
     * 中new一个 新的具体步骤net::Step子类实例StepB，调用net::Step基类的RegisterCallback()
     * 方法将该新实例登记并执行该新实例的StepB.Start()方法，若StepB.Start()执行成功则后续
     * StepB.Callback()会被调用，若StepB.Start()执行失败，则调用net::Step基类的
     * DeleteCallback()将刚登记的StepB删除掉并执行对应的错误处理。
     * 若RegisterCallback()登记失败（失败可能性微乎其微）则应将StepB销毁,重新new StepB实例并登记，
     * 若多次（可自定义）登记失败则应放弃登记，并将StepB销毁；若RegisterCallback()登记成功则一定不可以
     * 销毁StepB（销毁操作会自动在回调StepB.Callback()之后执行）
     * @param pStep 回调步骤实例
     * @param dTimeout 超时时间
     * @return 是否登记成功
     */
    bool RegisterCallback(Step* pStep, ev_tstamp dTimeout = 0.0);
    /**
     * @brief 删除回调步骤
     * @note 在RegisterCallback()成功，但执行pStep->Start()失败时被调用
     * @param pStep 回调步骤实例
     */
    void DeleteCallback(Step* pStep)
    {
        LOG4_TRACE("Step[%u]::%s()", GetSequence(), __FUNCTION__);
        g_pLabor->DeleteCallback(GetSequence(), pStep);
    }
    /*
     * 发送客户端封装(只有构造函数传入shell使用)
     * */
    bool SendToClient(HttpMsg &oInHttpMsg,const std::string &strBody,int iCode)
    {
    	return net::SendToClient(m_stReqMsgShell,oInHttpMsg,strBody,iCode);
    }
    bool SendToClient(const std::string &strBody,int nCode = 200)
	{
		if (m_oReqMsgHead.cmd() == 0) return net::SendToClient(m_stReqMsgShell,m_oInHttpMsg,strBody,nCode);
		else return net::SendToClient(m_stReqMsgShell,m_oReqMsgHead,strBody);
	}
public:
	/*
	 * 步骤管理
	 * */
    //延迟下一个步骤的超时时间
    void DelayNextStep();
    // 执行下一步
    bool NextStep(Step* pNextStep, int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrClientShow = "");
    bool NextStep(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrClientShow = "");
    net::Step* GetNextStep(){return(m_pNextStep);}
    void SetNextStepNull();
    /**
	 * @brief 添加上一步骤
	 * @note 用于设置步骤间的依赖关系，框架在当前步骤终止并销毁前检查是否仍有以当前步骤为下一步的步骤尚未
	 * 结束，若有则延长当前步骤的生命期。
	 * @param pStep 上一步骤
	 */
	void AddPreStepSeq(Step* pStep);
	void RemovePreStepSeq(Step* pStep);
protected:
    /**
     * @brief 设置为已注册状态
     */
    void SetRegistered(){m_bRegistered = true;}
    /**
     * @brief 设置为未注册状态
     * @note 当且仅当UnRegisterCallback(Step*)时由框架调用
     */
    void UnsetRegistered(){m_bRegistered = false;}
    void AddNextStepSeq(Step* pStep);
public:  // 请求端的上下文信息，通过Step构造函数初始化，若调用的是不带参数的构造函数Step()，则这几个成员不会被初始化
    tagMsgShell m_stReqMsgShell;
    MsgHead m_oReqMsgHead;
    MsgBody m_oReqMsgBody;
    HttpMsg m_oInHttpMsg;//HttpStep使用
    Step* DelayDel(){m_boDelayDel = true;return this;}//在回调函数中需要再次发送回调函数，则延迟删除
    bool m_boDelayDel;//DataStep使用
private:
    bool m_bRegistered;
    uint32 m_ulSequence;
    ev_tstamp m_dActiveTime;
    ev_tstamp m_dTimeout;
    ev_timer* m_pTimeoutWatcher;
    std::string m_strClassName;
    Step* m_pNextStep;
    std::set<uint32> m_setNextStepSeq;
    std::set<uint32> m_setPreStepSeq;

    friend class Worker;
    friend class DataStep;
	uint32 m_uiUserId;
	uint32 m_uiCmd;
};

} /* namespace net */

#endif /* STEP_HPP_ */
