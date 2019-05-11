#ifndef SRC_StepNode_HPP_
#define SRC_StepNode_HPP_
#include "step/Step.hpp"
#include "step/HttpStep.hpp"
#include "session/Session.hpp"
#include "NetError.hpp"
#include "NetDefine.hpp"
#include "storage/RedisOperator.hpp"
#include "storage/DbOperator.hpp"
#include "storage/MemOperator.hpp"

namespace net
{
//节点间通信异步回调处理步骤(不在逻辑层使用)
class StepNode: public net::Step
{
public:
    StepNode(const DataMem::MemOperate* pMemOper);
    StepNode(const std::string &strBody);
    virtual ~StepNode();
    void Init();
    virtual net::E_CMD_STATUS Callback(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data = NULL);
    virtual net::E_CMD_STATUS Timeout();
    virtual net::E_CMD_STATUS Emit(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = "");

    //回调设置
    void SetCallBack(SessionCallbackMem callback,net::Session* pSession,const std::string &nodeType = PROXY_NODE,uint32 uiCmd = net::CMD_REQ_STORATE,int64 uiModFactor=-1);
    void SetCallBack(StepCallbackMem callback,net::Step* pUpperStep,const std::string &nodeType = PROXY_NODE,uint32 uiCmd = net::CMD_REQ_STORATE,int64 uiModFactor=-1);
    void SetCallBack(SessionCallback callback,net::Session* pSession,const std::string &nodeType,uint32 uiCmd,int64 uiModFactor=-1);
	void SetCallBack(StepCallback callback,net::Step* pUpperStep,const std::string &nodeType,uint32 uiCmd,int64 uiModFactor=-1);

	void SetCallBack(SessionCallback callback,net::Session* pSession,const tagMsgShell &stMsgShell,uint32 uiCmd,int64 uiModFactor=-1);
	void SetCallBack(StepCallback callback,net::Step* pUpperStep,const tagMsgShell &stMsgShell,uint32 uiCmd,int64 uiModFactor=-1);

	void SetTimeOutMax(uint32 uiTimeOut){m_uiTimeOutMax = uiTimeOut;}
	//1:超时重发 0：超时不重发
	void SetRetrySend(uint32 uiRetrySend=1){m_uiRetrySend = uiRetrySend;}
	void SetModFactor(int64 uiModFactor){m_uiModFactor = uiModFactor;}
private:
	bool DecodeMemRsp(DataMem::MemRsp &oRsp,const MsgBody& oInMsgBody);
	net::Session* GetSession();
    uint32 m_uiTimeOut;
    uint32 m_uiTimeOutMax;
    uint32 m_uiRetrySend;
    int64 m_uiModFactor;

    std::string m_strNodeType;//支持 例如 SESSION 或者 IP:端口
    tagMsgShell m_stMsgShell;

    uint32 m_uiCmd;
    std::string m_strMsgSerial;//消息体序列化

    //回调处理
    SessionCallbackMem m_storageCallbackSession;
    StepCallbackMem m_storageCallbackStep;

    SessionCallback m_standardCallbackSession;
    StepCallback m_standardCallbackStep;

    std::string m_strUpperSessionId;
    std::string m_strUpperSessionClassName;
    net::Session* m_pSession;
    uint32 m_uiUpperStepSeq;
};

//存储数据步骤,用来步骤状态
class DataStep: public net::HttpStep
{
public:
    DataStep(StepParam *data=NULL){SetData(data);}
	DataStep(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,StepParam *data=NULL):net::HttpStep(stMsgShell,oInHttpMsg){SetData(data);}
	DataStep(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,StepParam *data=NULL):net::HttpStep(stMsgShell,oInMsgHead){SetData(data);}
    virtual ~DataStep(){}
    virtual E_CMD_STATUS Emit(int iErrno = 0, const std::string& strErrMsg = "", const std::string& strErrShow = ""){return Timeout();}
	virtual E_CMD_STATUS Timeout()
	{
		if (m_boDelayDel)
		{
			m_boDelayDel = false;
			return net::STATUS_CMD_RUNNING;
		}
		return net::STATUS_CMD_COMPLETED;
	}
};

}


#endif
