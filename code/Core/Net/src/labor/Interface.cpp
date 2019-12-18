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

uint32 GetNodeId(){return(GetLabor()->GetNodeId());}
uint32 GetWorkerIndex(){return(GetLabor()->GetWorkerIndex());}
const std::string& GetNodeType() {return(GetLabor()->GetNodeType());}
const util::CJsonObject& GetCustomConf() {return(GetLabor()->GetCustomConf());}
const std::string& GetWorkerIdentify() {return(GetLabor()->GetWorkerIdentify());}
const std::string& GetWorkPath(){return(GetLabor()->GetWorkPath());}
std::string GetConfigPath(){return(GetLabor()->GetWorkPath() + std::string("/conf/"));}
time_t GetNowTime(){return(GetLabor()->GetNowTime());}
void GetNodeIdentifys(const std::string& strNodeType, std::vector<std::string>& strIdentifys){return(GetLabor()->GetNodeIdentifys(strNodeType,strIdentifys));}

bool SendToClient(const net::tagMsgShell& oInMsgShell,const MsgHead &oInMsgHead,const std::string &strBody)
{
	return GetLabor()->SendToClient(oInMsgShell,oInMsgHead,strBody);
}
bool SendToClient(const net::tagMsgShell& oInMsgShell,const HttpMsg& oInHttpMsg,const std::string &strBody,int iCode,const std::unordered_map<std::string,std::string> &heads)
{
	return GetLabor()->SendToClient(oInMsgShell,oInHttpMsg,strBody,iCode,heads);
}
bool SendToClient(const tagMsgShell& oInMsgShell,const MsgHead& oInMsgHead,const google::protobuf::Message &message,const std::string& additional,uint64 sessionid,const std::string& stressionid,bool boJsonBody)
{
	return GetLabor()->SendToClient(oInMsgShell,oInMsgHead,message,additional,sessionid,stressionid,boJsonBody);
}
bool SendToClient(const std::string& strIdentify,const MsgHead& oInMsgHead,const google::protobuf::Message &message,const std::string& additional,uint64 sessionid,const std::string& stressionid,bool boJsonBody)
{
	return GetLabor()->SendToClient(strIdentify,oInMsgHead,message,additional,sessionid,stressionid,boJsonBody);
}

//发送远程过程回调
bool SendToCallback(Session* pSession,const DataMem::MemOperate* pMemOper,SessionCallbackMem callback,const std::string &nodeType,uint32 uiCmd)
{
    return GetLabor()->SendToCallback(pSession,pMemOper,callback,nodeType,uiCmd,-1);
}

bool SendToCallback(Session* pSession,uint32 uiCmd,const std::string &strBody,SessionCallback callback,const std::string &nodeType)
{
    return GetLabor()->SendToCallback(pSession,uiCmd,strBody,callback,nodeType,-1);
}

bool SendToCallback(Session* pSession,uint32 uiCmd,const std::string &strBody,net::SessionCallback callback,const net::tagMsgShell& stMsgShell)
{
	return GetLabor()->SendToCallback(pSession,uiCmd,strBody,callback,stMsgShell,-1);
}
bool SendToCallback(net::Step* pUpperStep,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,const std::string &nodeType,uint32 uiCmd)
{
    return GetLabor()->SendToCallback(pUpperStep,pMemOper,callback,nodeType,uiCmd,-1);
}

bool SendToCallback(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,StepParam *data,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,const std::string &nodeType,uint32 uiCmd)
{
    return GetLabor()->SendToCallback(new DataStep(stMsgShell,oInHttpMsg,data),pMemOper,callback,nodeType,uiCmd,-1);
}

bool SendToCallback(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const DataMem::MemOperate* pMemOper,StepCallbackMem callback,const std::string &nodeType,uint32 uiCmd)
{
    return GetLabor()->SendToCallback(new DataStep(stMsgShell,oInHttpMsg),pMemOper,callback,nodeType,uiCmd,-1);
}

bool SendToCallback(net::Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,const std::string &nodeType)
{
    return GetLabor()->SendToCallback(pUpperStep,uiCmd,strBody,callback,nodeType,-1);
}
bool SendToCallback(net::Step* pUpperStep,uint32 uiCmd,const std::string &strBody,StepCallback callback,const tagMsgShell& stMsgShell)
{
	return GetLabor()->SendToCallback(pUpperStep,uiCmd,strBody,callback,stMsgShell,-1);
}
bool AddMsgShell(const std::string& strIdentify, const net::tagMsgShell& stMsgShell){return(GetLabor()->AddMsgShell(strIdentify, stMsgShell));}
void DelMsgShell(const std::string& strIdentify, const net::tagMsgShell& stMsgShell){GetLabor()->DelMsgShell(strIdentify,stMsgShell);}

bool ParseMsgBody(const MsgBody& oInMsgBody,google::protobuf::Message &message){return GetLabor()->ParseMsgBody(oInMsgBody,message);}

bool RegisterCallback(net::Step* pStep){return(GetLabor()->RegisterCallback(pStep));}
void DeleteCallback(net::Step* pStep){GetLabor()->DeleteCallback(pStep);}
bool RegisterCallback(net::MysqlStep* pMysqlStep){return GetLabor()->RegisterCallback(pMysqlStep);}

bool RegisterCallback(net::Session* pSession){return(GetLabor()->RegisterCallback(pSession));}

void DeleteCallback(net::Session* pSession){GetLabor()->DeleteCallback(pSession);}

Session* GetSession(net::uint64 uiSessionId, const std::string& strSessionClass){return(GetLabor()->GetSession(uiSessionId, strSessionClass));}
Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass){return(GetLabor()->GetSession(strSessionId, strSessionClass));}

bool SendTo(const net::tagMsgShell& stMsgShell){return(GetLabor()->SendTo(stMsgShell));}
bool SendTo(const net::tagMsgShell& stMsgShell, const MsgHead& oMsgHead, const MsgBody& oMsgBody){return(GetLabor()->SendTo(stMsgShell, oMsgHead, oMsgBody));}
bool SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody){return(GetLabor()->SendTo(strIdentify, oMsgHead, oMsgBody));}
bool SendToSession(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody){return(GetLabor()->SendToSession(strNodeType, oMsgHead, oMsgBody));}
bool SendToNext(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody){return(GetLabor()->SendToNext(strNodeType, oMsgHead, oMsgBody));}
bool SendToWithMod(const std::string& strNodeType, uint32 uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody){return(GetLabor()->SendToWithMod(strNodeType, uiModFactor, oMsgHead, oMsgBody));}
bool SendToConHash(const std::string& strNodeType, uint32 uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody){return(GetLabor()->SendToConHash(strNodeType, uiModFactor, oMsgHead, oMsgBody));}
bool RegisterCallback(const std::string& strIdentify, net::RedisStep* pRedisStep){return(GetLabor()->RegisterCallback(strIdentify, pRedisStep));}
bool RegisterCallback(const std::string& strHost, int iPort, net::RedisStep* pRedisStep){return(GetLabor()->RegisterCallback(strHost, iPort, pRedisStep));}
void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify){GetLabor()->AddNodeIdentify(strNodeType, strIdentify);}
void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify){GetLabor()->DelNodeIdentify(strNodeType, strIdentify);}
bool AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx){return(GetLabor()->AddRedisContextAddr(strHost, iPort, ctx));}
void DelRedisContextAddr(const redisAsyncContext* ctx){GetLabor()->DelRedisContextAddr(ctx);}

bool Launch(StepState *pStep,uint32 uiTimeOutMax,uint8 uiToRetry,double dTimeout)
{
	if (pStep == NULL)
	{
		LOG4_ERROR("%s() null step",__FUNCTION__);
		return(false);
	}
	pStep->Init(uiTimeOutMax,uiToRetry);
	return GetLabor()->ExecStep(pStep,0,"","",dTimeout);
}

bool Register(StepState *pStep,uint32 uiTimeOutMax,uint8 uiToRetry,double dTimeout)
{
	if (!GetLabor()->RegisterCallback(pStep,dTimeout))
	{
		LOG4_ERROR("%s() RegisterCallback error",__FUNCTION__);
		SAFE_DELETE(pStep);
		return(false);
	}
	pStep->Init(uiTimeOutMax,uiToRetry);
	return true;
}

void SkipNonsenseLetters(std::string& word)
{
	GetLabor()->m_IgnoreChars.SkipNonsenseLetters(word);
}

void SkipFormatLetters(std::string& word)
{
	GetLabor()->m_IgnoreChars.SkipFormatLetters(word);
}

bool GetConfig(util::CJsonObject& oConf,const std::string &strConfFile)
{
	std::ifstream fin(strConfFile.c_str());
	//配置信息输入流
	if (fin.good())
	{
		//解析配置信息 JSON格式
		std::stringstream ssContent;
		ssContent << fin.rdbuf();
		if (!oConf.Parse(ssContent.str()))
		{
			//配置文件解析失败
			LOG4_ERROR("Read conf (%s) error,it's maybe not a json file!",strConfFile.c_str());
			ssContent.str("");
			fin.close();
			return false;
		}
		ssContent.str("");
		fin.close();
		return true;
	}
	else
	{
		//配置信息流读取失败
		LOG4_ERROR( "Open conf (%s) error!",strConfFile.c_str());
		return false;
	}
}

bool ExecStep(Step* pStep,int iErrno, const std::string& strErrMsg, const std::string& strErrShow,ev_tstamp dTimeout)
{
	return GetLabor()->ExecStep(pStep,iErrno,strErrMsg,strErrShow,dTimeout);
}


bool CoroutineResumeWithTimes(uint32 nMaxTimes){return GetLabor()->m_Coroutine.CoroutineResumeWithTimes(nMaxTimes);}
bool CoroutineNewWithArg(util::coroutine_func func,tagCoroutineArg *arg) {return GetLabor()->m_Coroutine.CoroutineNewWithArg(func,arg);}
int CoroutineNew(util::coroutine_func func,void *ud) {return GetLabor()->m_Coroutine.CoroutineNew(func,ud);}
int CoroutineRunning(){return GetLabor()->m_Coroutine.CoroutineRunning();}
int CoroutineStatus(int coid){return GetLabor()->m_Coroutine.CoroutineStatus(coid);}
bool CoroutineResume(int coid){return GetLabor()->m_Coroutine.CoroutineResume(coid);}
bool CoroutineResume(){return GetLabor()->m_Coroutine.CoroutineResume();}
bool CoroutineYield(){return GetLabor()->m_Coroutine.CoroutineYield();}


}

