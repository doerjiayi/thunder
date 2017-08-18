/*******************************************************************************
 * Project:  HelloThunder
 * @file     CmdHello.cpp
 * @brief 
 * @author   cjy
 * @date:    2017年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include <sys/syscall.h>
#include "ModuleHello.hpp"
#include "../CustomCoroutinue.hpp"

#ifdef __cplusplus
extern "C" {
#endif
thunder::Cmd* create()
{
    thunder::Cmd* pCmd = new hello::ModuleHello();
    return(pCmd);
}
#ifdef __cplusplus
}
#endif

namespace hello
{

ModuleHello::ModuleHello()
    : pStepHello(NULL), pStepGetHelloName(NULL), pStepHttpRequest(NULL),pSession(NULL)
{
}

ModuleHello::~ModuleHello()
{
}

bool ModuleHello::Init()
{
	pSession = (SessionHello*)GetSession(20000);
	if (pSession == NULL)
	{
		pSession = new SessionHello(20000, 300.0);
		if(!RegisterCallback(pSession))
		{
			delete pSession;
			pSession = NULL;
		}
	}
	if (pSession)
	{
		TestCoroutinue2();
//		TestCoroutinueBenchMark();
	}
	return(true);
}

bool ModuleHello::AnyMessage(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
    LOG4CPLUS_DEBUG_FMT(GetLogger(), "%s()", __FUNCTION__);
    return GetLabor()->SendTo(stMsgShell, oInHttpMsg);//空载测试
//    TestHttpRequest(stMsgShell,oInHttpMsg);
//    return(true);
}


bool ModuleHello::TestStepHello(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
	pStepHello = new StepHello(stMsgShell, oInHttpMsg);
	if (pStepHello == NULL)
	{
		LOG4CPLUS_ERROR_FMT(GetLogger(), "error %d: new StepFromClient() error!", thunder::ERR_NEW);
		return(false);
	}
	if (RegisterCallback(pStepHello))
	{
		if (thunder::STATUS_CMD_RUNNING == pStepHello->Emit())
		{
			return(true);
		}
		DeleteCallback(pStepHello);
		return(false);
	}
	else
	{
		delete pStepHello;
		pStepHello = NULL;
		return(false);
	}
	return true;
}

bool ModuleHello::TestRedisCmd(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
	 // RedisStep
	pStepGetHelloName = new StepGetHelloName(stMsgShell, oInHttpMsg);
	pStepGetHelloName->RedisCmd()->SetHashKey("123456");
	pStepGetHelloName->RedisCmd()->SetCmd("hget");
	pStepGetHelloName->RedisCmd()->Append("1:2:123456");
	pStepGetHelloName->RedisCmd()->Append("name");
	//    pStepGetHelloName->RedisCmd()->Append("lilei");
	//    pStepGetHelloName->RedisCmd()->Append("lilei2");
	if (!RegisterCallback("192.168.18.78", 22125, pStepGetHelloName))
	{
		delete pStepGetHelloName;
		pStepGetHelloName = NULL;
		return(false);
	}
	return true;
}

bool ModuleHello::TestHttpRequest(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
	 // HttpStep
	pStepHttpRequest = new StepHttpRequest(stMsgShell,oInHttpMsg);
	if (pStepHttpRequest == NULL)
	{
		LOG4CPLUS_ERROR_FMT(GetLogger(), "error %d: new StepHttpRequest() error!", thunder::ERR_NEW);
		return(false);
	}

	if (RegisterCallback(pStepHttpRequest))
	{
		if (thunder::STATUS_CMD_RUNNING == pStepHttpRequest->Emit())
		{
			return(true);
		}
		DeleteCallback(pStepHttpRequest);
		return(false);
	}
	else
	{
		delete pStepHttpRequest;
		pStepHttpRequest = NULL;
		return(false);
	}
	return(true);
}

void CoroutineArgs::CoroutineFunc(void *ud) {
	CoroutineArgs* arg = (CoroutineArgs*)ud;
	int start = arg->n;
	for (int i=0;i<3;i++)
	{
		arg->pSession->AddHelloNum(2);
		LOG4CPLUS_TRACE_FMT(arg->labor->GetLogger(),"coroutine running(%d),arg(%d) tid(%u) HelloNum(%d) coroutineName(%s)",
				arg->labor->CoroutineRunning(CoroutineName) , start + i,pthread_self(),
				arg->pSession->GetHelloNum(),arg->CoroutineName.c_str());

		SuspendCoroutine(arg);
	}
}

void CoroutineArgs2::CoroutineFunc(void *ud) {
	CoroutineArgs2* arg = (CoroutineArgs2*)ud;
	for (int i=0;i<3;i++)
	{
		arg->pSession->AddHelloNum(2);
		LOG4CPLUS_TRACE_FMT(arg->labor->GetLogger(),"coroutine running(%d),param(%s) tid(%u) HelloNum(%d) coroutineName(%s)",
				arg->labor->CoroutineRunning(CoroutineName) , arg->param.c_str(),pthread_self(),
				arg->pSession->GetHelloNum(),arg->CoroutineName.c_str());

		SuspendCoroutine(arg);
	}
}

void ModuleHello::TestCoroutinue()
{
	CoroutineArgs arg1 = {GetLabor(), 0 ,pSession};
	CoroutineArgs arg2 = {GetLabor(), 100 ,pSession};
	//int co1 = GetLabor()->CoroutineNew(CoroutineArgs::CoroutineName,CoroutineArgs::CoroutineFunc,&arg1);
	int co1 = MakeCoroutine(CoroutineArgs,arg1);
	int co2 = MakeCoroutine(CoroutineArgs,arg2);
	LOG4CPLUS_TRACE_FMT(GetLogger(), "Coroutine start! tid(%u)",pthread_self());
	while (GetLabor()->CoroutineStatus(CoroutineArgs::CoroutineName,co1)
			|| GetLabor()->CoroutineStatus(CoroutineArgs::CoroutineName,co2))
	{
		GetLabor()->CoroutineResume(CoroutineArgs::CoroutineName,co1);
		GetLabor()->CoroutineResume(CoroutineArgs::CoroutineName,co2);
	}
	LOG4CPLUS_TRACE_FMT(GetLogger(), "Coroutine end!tid(%u)",pthread_self());
}

/*
详细流程：
创建协程
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1210] CoroutineNew coroutine_new co1:0
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1218] CoroutineNew coroutine coid(0) status(1)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1210] CoroutineNew coroutine_new co1:1
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1218] CoroutineNew coroutine coid(1) status(1)
启动协程，唤醒0号协程
[2017-08-17 15:40:59,582][TRACE] [ModuleHello.cpp:146] Coroutine start!tid(2330580960) HelloNum(2) coroutineName(Coroutine1)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1257] CoroutineStatus coroutine_status coid(0) status(1)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1257] CoroutineStatus coroutine_status coid(1) status(1)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1232] CoroutineResume coroutine_resume coid(0) status(1)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1275] CoroutineRunning coroutine_status running_id(0)
执行0号协程，然后退出0号协程,回到主协程，唤醒1号协程
[2017-08-17 15:40:59,582][TRACE] [ModuleHello.cpp:135] coroutine running(0),arg(0) tid(2330580960) HelloNum(4) coroutineName(Coroutine2)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1242] CoroutineYield coroutine_yield running_id(0) status(2)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1232] CoroutineResume coroutine_resume coid(1) status(1)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1275] CoroutineRunning coroutine_status running_id(1)
执行1号协程，然后退出1号协程,回到主协程，唤醒0号协程
[2017-08-17 15:40:59,582][TRACE] [ModuleHello.cpp:135] coroutine running(1),arg(100) tid(2330580960) HelloNum(6) coroutineName(Coroutine1)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1242] CoroutineYield coroutine_yield running_id(1) status(2)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1257] CoroutineStatus coroutine_status coid(0) status(3)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1257] CoroutineStatus coroutine_status coid(1) status(3)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1232] CoroutineResume coroutine_resume coid(0) status(3)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1275] CoroutineRunning coroutine_status running_id(0)
执行0号协程，然后退出0号协程,回到主协程，唤醒1号协程
[2017-08-17 15:40:59,582][TRACE] [ModuleHello.cpp:135] coroutine running(0),arg(1) tid(2330580960) HelloNum(8) coroutineName(Coroutine2)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1242] CoroutineYield coroutine_yield running_id(0) status(2)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1232] CoroutineResume coroutine_resume coid(1) status(3)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1275] CoroutineRunning coroutine_status running_id(1)
执行1号协程，然后退出1号协程,回到主协程，唤醒0号协程
[2017-08-17 15:40:59,582][TRACE] [ModuleHello.cpp:135] coroutine running(1),arg(101) tid(2330580960) HelloNum(10) coroutineName(Coroutine1)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1242] CoroutineYield coroutine_yield running_id(1) status(2)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1257] CoroutineStatus coroutine_status coid(0) status(3)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1257] CoroutineStatus coroutine_status coid(1) status(3)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1232] CoroutineResume coroutine_resume coid(0) status(3)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1275] CoroutineRunning coroutine_status running_id(0)
执行0号协程，然后退出0号协程,回到主协程，唤醒1号协程
[2017-08-17 15:40:59,582][TRACE] [ModuleHello.cpp:135] coroutine running(0),arg(2) tid(2330580960) HelloNum(12) coroutineName(Coroutine2)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1242] CoroutineYield coroutine_yield running_id(0) status(2)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1232] CoroutineResume coroutine_resume coid(1) status(3)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1275] CoroutineRunning coroutine_status running_id(1)
执行1号协程，然后退出1号协程,回到主协程，尝试唤醒0号协程，已执行完毕，尝试唤醒1号协程，已执行完毕，获取0号协程状态，已是dead状态,不再唤醒该协程，获取1号协程状态，已是dead状态,不再唤醒该协程
[2017-08-17 15:40:59,582][TRACE] [ModuleHello.cpp:135] coroutine running(1),arg(102) tid(2330580960)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1242] CoroutineYield coroutine_yield running_id(1) status(2)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1257] CoroutineStatus coroutine_status coid(0) status(3)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1257] CoroutineStatus coroutine_status coid(1) status(3)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1232] CoroutineResume coroutine_resume coid(0) status(3)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1232] CoroutineResume coroutine_resume coid(1) status(3)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1257] CoroutineStatus coroutine_status coid(0) status(0)
[2017-08-17 15:40:59,582][TRACE] [../src/labor/process/NodeWorker.cpp:1257] CoroutineStatus coroutine_status coid(1) status(0)
协程结束
[2017-08-17 15:40:59,582][TRACE] [ModuleHello.cpp:153] Coroutine end!
 * */

void ModuleHello::TestCoroutinue2()
{
	{
		CoroutineArgs arg1 = {GetLabor(), 0 ,pSession};
		CoroutineArgs arg2 = {GetLabor(), 100 ,pSession};
		//GetLabor()->CoroutineNew(CoroutineArgs::CoroutineName,CoroutineArgs::CoroutineFunc,&arg1);
		//两个协程对象
		MakeCoroutine(CoroutineArgs,arg1);
		MakeCoroutine(CoroutineArgs,arg2);
		LOG4CPLUS_TRACE_FMT(GetLogger(), "%s Coroutine start! tid(%u) &arg1:%p",__FUNCTION__,pthread_self(),&arg1);
		RunCoroutine(CoroutineArgs);
		LOG4CPLUS_TRACE_FMT(GetLogger(), "%s Coroutine end!tid(%u)",__FUNCTION__,pthread_self());
	}
	{
		//两个协程对象
		CoroutineArgs2 arg1 = {GetLabor(), "param1" ,pSession};
		CoroutineArgs2 arg2 = {GetLabor(), "param2" ,pSession};
		//GetLabor()->CoroutineNew(CoroutineArgs::CoroutineName,CoroutineArgs::CoroutineFunc,&arg1);
		MakeCoroutine(CoroutineArgs2,arg1);
		MakeCoroutine(CoroutineArgs2,arg2);
		LOG4CPLUS_TRACE_FMT(GetLogger(), "%s Coroutine start! tid(%u) &arg1:%p",__FUNCTION__,pthread_self(),&arg1);
		RunCoroutine(CoroutineArgs2);
		LOG4CPLUS_TRACE_FMT(GetLogger(), "%s Coroutine end!tid(%u)",__FUNCTION__,pthread_self());
	}
}

/*
 * 过程如下
[2017-08-19 02:04:36,125][DEBUG] [../src/labor/NodeLabor.cpp:30] CoroutineNew new CoroutineSchedule coroutineName(1514472400)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:51] CoroutineNew coroutine coid(0) status(1) coroutineName(CoroutineName)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:51] CoroutineNew coroutine coid(1) status(1) coroutineName(CoroutineName)
[2017-08-19 02:04:36,125][TRACE] [ModuleHello.cpp:248] TestCoroutinue2 Coroutine start! tid(1593948288) &arg1:0x7ffc20c18c20
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(0) status(1)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(0)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(0) status(1)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:150] CoroutineRunning coroutine_status running_id(0) coroutineName(CoroutineName)
[2017-08-19 02:04:36,125][TRACE] [ModuleHello.cpp:148] coroutine running(0),arg(0) tid(1593948288) HelloNum(2) coroutineName(CoroutineName)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:130] CoroutineYield coroutine_yield running_id(0) status(2)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(1) status(1)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(1)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(1) status(1)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:150] CoroutineRunning coroutine_status running_id(1) coroutineName(CoroutineName)
[2017-08-19 02:04:36,125][TRACE] [ModuleHello.cpp:148] coroutine running(1),arg(100) tid(1593948288) HelloNum(4) coroutineName(CoroutineName)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:130] CoroutineYield coroutine_yield running_id(1) status(2)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(0) status(3)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(0)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(0) status(3)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:150] CoroutineRunning coroutine_status running_id(0) coroutineName(CoroutineName)
[2017-08-19 02:04:36,125][TRACE] [ModuleHello.cpp:148] coroutine running(0),arg(1) tid(1593948288) HelloNum(6) coroutineName(CoroutineName)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:130] CoroutineYield coroutine_yield running_id(0) status(2)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(1) status(3)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(1)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(1) status(3)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:150] CoroutineRunning coroutine_status running_id(1) coroutineName(CoroutineName)
[2017-08-19 02:04:36,125][TRACE] [ModuleHello.cpp:148] coroutine running(1),arg(101) tid(1593948288) HelloNum(8) coroutineName(CoroutineName)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:130] CoroutineYield coroutine_yield running_id(1) status(2)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(0) status(3)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(0)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(0) status(3)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:150] CoroutineRunning coroutine_status running_id(0) coroutineName(CoroutineName)
[2017-08-19 02:04:36,125][TRACE] [ModuleHello.cpp:148] coroutine running(0),arg(2) tid(1593948288) HelloNum(10) coroutineName(CoroutineName)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:130] CoroutineYield coroutine_yield running_id(0) status(2)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(1) status(3)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(1)
[2017-08-19 02:04:36,125][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(1) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:150] CoroutineRunning coroutine_status running_id(1) coroutineName(CoroutineName)
[2017-08-19 02:04:36,126][TRACE] [ModuleHello.cpp:148] coroutine running(1),arg(102) tid(1593948288) HelloNum(12) coroutineName(CoroutineName)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:130] CoroutineYield coroutine_yield running_id(1) status(2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(0) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(0)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(0) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(1) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(1)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(1) status(3)
[2017-08-19 02:04:36,126][TRACE] [ModuleHello.cpp:250] TestCoroutinue2 Coroutine end!tid(1593948288)


[2017-08-19 02:04:36,126][DEBUG] [../src/labor/NodeLabor.cpp:30] CoroutineNew new CoroutineSchedule coroutineName(1514472432)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:51] CoroutineNew coroutine coid(0) status(1) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:51] CoroutineNew coroutine coid(1) status(1) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [ModuleHello.cpp:259] TestCoroutinue2 Coroutine start! tid(1593948288) &arg1:0x7ffc20c18ba0
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(0) status(1)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(0)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(0) status(1)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:150] CoroutineRunning coroutine_status running_id(0) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [ModuleHello.cpp:161] coroutine running(0),param(param1) tid(1593948288) HelloNum(14) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:130] CoroutineYield coroutine_yield running_id(0) status(2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(1) status(1)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(1)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(1) status(1)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:150] CoroutineRunning coroutine_status running_id(1) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [ModuleHello.cpp:161] coroutine running(1),param(param2) tid(1593948288) HelloNum(16) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:130] CoroutineYield coroutine_yield running_id(1) status(2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(0) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(0)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(0) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:150] CoroutineRunning coroutine_status running_id(0) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [ModuleHello.cpp:161] coroutine running(0),param(param1) tid(1593948288) HelloNum(18) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:130] CoroutineYield coroutine_yield running_id(0) status(2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(1) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(1)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(1) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:150] CoroutineRunning coroutine_status running_id(1) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [ModuleHello.cpp:161] coroutine running(1),param(param2) tid(1593948288) HelloNum(20) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:130] CoroutineYield coroutine_yield running_id(1) status(2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(0) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(0)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(0) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:150] CoroutineRunning coroutine_status running_id(0) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [ModuleHello.cpp:161] coroutine running(0),param(param1) tid(1593948288) HelloNum(22) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:130] CoroutineYield coroutine_yield running_id(0) status(2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(1) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(1)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(1) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:150] CoroutineRunning coroutine_status running_id(1) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [ModuleHello.cpp:161] coroutine running(1),param(param2) tid(1593948288) HelloNum(24) coroutineName(CoroutineName2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:130] CoroutineYield coroutine_yield running_id(1) status(2)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(0) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(0)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(0) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:84] CoroutineResume coroutine_status coid(1) status(3)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:92] CoroutineResume CoroutineResume coid(1)
[2017-08-19 02:04:36,126][TRACE] [../src/labor/NodeLabor.cpp:101] CoroutineResume coroutine_resume coid(1) status(3)
[2017-08-19 02:04:36,126][TRACE] [ModuleHello.cpp:261] TestCoroutinue2 Coroutine end!tid(1593948288)

*/

void ModuleHello::TestCoroutinueBenchMark()
{
	m_CustomClock.Start("TestCoroutinueBenchMark",GetLogger());
	for (int i = 0;i < 10000;++i)
	{
		CoroutineArgs arg1 = {GetLabor(), 0 ,pSession};
		CoroutineArgs arg2 = {GetLabor(), 100 ,pSession};
		MakeCoroutine(CoroutineArgs,arg1);
		MakeCoroutine(CoroutineArgs,arg2);
		LOG4CPLUS_TRACE_FMT(GetLogger(), "%s Coroutine start! tid(%u) &arg1:%p",__FUNCTION__,pthread_self(),&arg1);
		RunCoroutine(CoroutineArgs);
		LOG4CPLUS_TRACE_FMT(GetLogger(), "%s Coroutine end!tid(%u)",__FUNCTION__,pthread_self());
	}
	m_CustomClock.EndClock();
	//CustomClock TestCoroutinueBenchMark use time(165.996994) ms
	////120000 switch Coroutine + 20000 new Coroutine + 20000 delete Coroutine
}


}
/* namespace hello */
