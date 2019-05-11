/*******************************************************************************
 * Project:  Net
 * @file     Interface.hpp
 * @brief    Node工作成员
 * @author   cjy
 * @date:    2017年9月6日
 * Modify history:
 ******************************************************************************/
#include <string>
#include "Interface.hpp"
#include "Labor.hpp"
#include "step/MysqlStep.hpp"
#include "step/RedisStep.hpp"
#include "step/StepNode.hpp"

namespace net
{

uint32 GetNodeId(){return(g_pLabor->GetNodeId());}
uint32 GetWorkerIndex(){return(g_pLabor->GetWorkerIndex());}
const std::string& GetNodeType() {return(g_pLabor->GetNodeType());}
const util::CJsonObject& GetCustomConf() {return(g_pLabor->GetCustomConf());}
const std::string& GetWorkerIdentify() {return(g_pLabor->GetWorkerIdentify());}
const std::string& GetWorkPath(){return(g_pLabor->GetWorkPath());}
std::string GetConfigPath(){return(g_pLabor->GetWorkPath() + std::string("/conf/"));}
time_t GetNowTime(){return(g_pLabor->GetNowTime());}
void GetNodeIdentifys(const std::string& strNodeType, std::vector<std::string>& strIdentifys){return(g_pLabor->GetNodeIdentifys(strNodeType,strIdentifys));}

bool SendToClient(const net::tagMsgShell& oInMsgShell,const MsgHead &oInMsgHead,const std::string &strBody)
{
	return g_pLabor->SendToClient(oInMsgShell,oInMsgHead,strBody);
}
bool SendToClient(const net::tagMsgShell& oInMsgShell,const HttpMsg& oInHttpMsg,const std::string &strBody,int iCode,const std::unordered_map<std::string,std::string> &heads)
{
	return g_pLabor->SendToClient(oInMsgShell,oInHttpMsg,strBody,iCode,heads);
}
bool SendToClient(const tagMsgShell& oInMsgShell,const MsgHead& oInMsgHead,const google::protobuf::Message &message,const std::string& additional,uint64 sessionid,const std::string& stressionid,bool boJsonBody)
{
	return g_pLabor->SendToClient(oInMsgShell,oInMsgHead,message,additional,sessionid,stressionid,boJsonBody);
}
bool SendToClient(const std::string& strIdentify,const MsgHead& oInMsgHead,const google::protobuf::Message &message,const std::string& additional,uint64 sessionid,const std::string& stressionid,bool boJsonBody)
{
	return g_pLabor->SendToClient(strIdentify,oInMsgHead,message,additional,sessionid,stressionid,boJsonBody);
}

//发送远程过程回调
bool SendToCallback(Session* pSession,const DataMem::MemOperate* pMemOper,SessionCallbackMem callback,const std::string &nodeType,uint32 uiCmd)
{
    return g_pLabor->SendToCallback(pSession,pMemOper,callback,nodeType,uiCmd,-1);
}

bool SendToModCallback(Session* pSession,const DataMem::MemOperate* pMemOper,SessionCallbackMem callback,int64 uiModFactor,const std::string &nodeType,uint32 uiCmd)
{
    return g_pLabor->SendToCallback(pSession,pMemOper,callback,nodeType,uiCmd,uiModFactor);
}

bool SendToCallback(Session* pSession,uint32 uiCmd,const std::string &strBody,SessionCallback callback,const std::string &nodeType)
{
    return g_pLabor->SendToCallback(pSession,uiCmd,strBody,callback,nodeType,-1);
}

bool SendToModCallback(Session* pSession,uint32 uiCmd,const std::string &strBody,SessionCallback callback,int64 uiModFactor,const std::string &nodeType)
{
    return g_pLabor->SendToCallback(pSession,uiCmd,strBody,callback,nodeType,uiModFactor);
}

bool SendToCallback(Session* pSession,uint32 uiCmd,const std::string &strBody,net::SessionCallback callback,const net::tagMsgShell& stMsgShell)
{
	return g_pLabor->SendToCallback(pSession,uiCmd,strBody,callback,stMsgShell,-1);
}
bool SendToModCallback(Session* pSession,uint32 uiCmd,const std::string &strBody,net::SessionCallback callback,int64 uiModFactor,const net::tagMsgShell& stMsgShell)
{
	return g_pLabor->SendToCallback(pSession,uiCmd,strBody,callback,stMsgShell,uiModFactor);
}

bool SendToCallback(net::Step* pUpperStep,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,const std::string &nodeType,uint32 uiCmd)
{
    return g_pLabor->SendToCallback(pUpperStep,pMemOper,callback,nodeType,uiCmd,-1);
}

bool SendToCallback(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,StepParam *data,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,const std::string &nodeType,uint32 uiCmd)
{
    return g_pLabor->SendToCallback(new DataStep(stMsgShell,oInHttpMsg,data),pMemOper,callback,nodeType,uiCmd,-1);
}

bool SendToCallback(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,const std::string &nodeType,uint32 uiCmd)
{
    return g_pLabor->SendToCallback(new DataStep(stMsgShell,oInHttpMsg),pMemOper,callback,nodeType,uiCmd,-1);
}

bool SendToModCallback(net::Step* pUpperStep,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,int64 uiModFactor,const std::string &nodeType,uint32 uiCmd)
{
    return g_pLabor->SendToCallback(pUpperStep,pMemOper,callback,nodeType,uiCmd,uiModFactor);
}

bool SendToCallback(net::Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,const std::string &nodeType)
{
    return g_pLabor->SendToCallback(pUpperStep,uiCmd,strBody,callback,nodeType,-1);
}
bool SendToModCallback(net::Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,int64 uiModFactor,const std::string &nodeType)
{
    return g_pLabor->SendToCallback(pUpperStep,uiCmd,strBody,callback,nodeType,uiModFactor);
}

bool SendToCallback(net::Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,const tagMsgShell& stMsgShell)
{
	return g_pLabor->SendToCallback(pUpperStep,uiCmd,strBody,callback,stMsgShell,-1);
}
bool SendToModCallback(net::Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,int64 uiModFactor,const tagMsgShell& stMsgShell)
{
	return g_pLabor->SendToCallback(pUpperStep,uiCmd,strBody,callback,stMsgShell,uiModFactor);
}

bool AddMsgShell(const std::string& strIdentify, const net::tagMsgShell& stMsgShell){return(g_pLabor->AddMsgShell(strIdentify, stMsgShell));}
void DelMsgShell(const std::string& strIdentify, const net::tagMsgShell& stMsgShell){g_pLabor->DelMsgShell(strIdentify,stMsgShell);}

bool ParseMsgBody(const MsgBody& oInMsgBody,google::protobuf::Message &message){return g_pLabor->ParseMsgBody(oInMsgBody,message);}

bool RegisterCallback(net::Step* pStep){return(g_pLabor->RegisterCallback(pStep));}
void DeleteCallback(net::Step* pStep){g_pLabor->DeleteCallback(pStep);}
bool RegisterCallback(net::MysqlStep* pMysqlStep){return g_pLabor->RegisterCallback(pMysqlStep);}

bool RegisterCallback(net::Session* pSession){return(g_pLabor->RegisterCallback(pSession));}

void DeleteCallback(net::Session* pSession){g_pLabor->DeleteCallback(pSession);}

Session* GetSession(net::uint64 uiSessionId, const std::string& strSessionClass){return(g_pLabor->GetSession(uiSessionId, strSessionClass));}
Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass){return(g_pLabor->GetSession(strSessionId, strSessionClass));}

bool SendTo(const net::tagMsgShell& stMsgShell){return(g_pLabor->SendTo(stMsgShell));}
bool SendTo(const net::tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody){return(g_pLabor->SendTo(stMsgShell, oMsgHead, oMsgBody));}
bool SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody){return(g_pLabor->SendTo(strIdentify, oMsgHead, oMsgBody));}
bool SendToNext(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody){return(g_pLabor->SendToNext(strNodeType, oMsgHead, oMsgBody));}
bool SendToWithMod(const std::string& strNodeType, uint32 uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody){return(g_pLabor->SendToWithMod(strNodeType, uiModFactor, oMsgHead, oMsgBody));}
bool RegisterCallback(const std::string& strIdentify, net::RedisStep* pRedisStep){return(g_pLabor->RegisterCallback(strIdentify, pRedisStep));}
bool RegisterCallback(const std::string& strHost, int iPort, net::RedisStep* pRedisStep){return(g_pLabor->RegisterCallback(strHost, iPort, pRedisStep));}
void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify){g_pLabor->AddNodeIdentify(strNodeType, strIdentify);}
void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify){g_pLabor->DelNodeIdentify(strNodeType, strIdentify);}
bool AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx){return(g_pLabor->AddRedisContextAddr(strHost, iPort, ctx));}
void DelRedisContextAddr(const redisAsyncContext* ctx){g_pLabor->DelRedisContextAddr(ctx);}

bool Launch(StepState *pStep,uint32 uiTimeOutMax,uint8 uiToRetry,double dTimeout)
{
	if (pStep == NULL)
	{
		LOG4_ERROR("%s() null step",__FUNCTION__);
		return(false);
	}
	pStep->Init(uiTimeOutMax,uiToRetry);
	return g_pLabor->ExecStep(pStep,0,"","",dTimeout);
}

bool Register(StepState *pStep,uint32 uiTimeOutMax,uint8 uiToRetry,double dTimeout)
{
	if (!g_pLabor->RegisterCallback(pStep,dTimeout))
	{
		LOG4_ERROR("%s() RegisterCallback error",__FUNCTION__);
		SAFE_DELETE(pStep);
		return(false);
	}
	pStep->Init(uiTimeOutMax,uiToRetry);
	return true;
}

bool CoroutineResumeWithTimes(uint32 nMaxTimes){return g_pLabor->m_Coroutine.CoroutineResumeWithTimes(nMaxTimes);}
bool CoroutineNewWithArg(util::coroutine_func func,tagCoroutineArg *arg) {return g_pLabor->m_Coroutine.CoroutineNewWithArg(func,arg);}
int CoroutineNew(util::coroutine_func func,void *ud) {return g_pLabor->m_Coroutine.CoroutineNew(func,ud);}
int CoroutineRunning(){return g_pLabor->m_Coroutine.CoroutineRunning();}
int CoroutineStatus(int coid){return g_pLabor->m_Coroutine.CoroutineStatus(coid);}
bool CoroutineResume(int coid){return g_pLabor->m_Coroutine.CoroutineResume(coid);}
bool CoroutineResume(){return g_pLabor->m_Coroutine.CoroutineResume();}
bool CoroutineYield(){return g_pLabor->m_Coroutine.CoroutineYield();}


}

