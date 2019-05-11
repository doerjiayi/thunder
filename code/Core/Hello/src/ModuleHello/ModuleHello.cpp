/*******************************************************************************
 * Project:  Hello
 * @file     ModuleHello.cpp
 * @brief 
 * @author   cjy
 * @date:    2017年2月1日
 * @note
 * Modify history:
 ******************************************************************************/
#include <map>
// Crypto++ Includes
#include "cryptopp/randpool.h"
#include "cryptopp/osrng.h"
#include "cryptopp/rsa.h"

#include "util/UnixTime.hpp"
#include "ModuleHello.hpp"
#include "StepHttpRequestState.hpp"

MUDULE_CREATE(core::ModuleHello);

namespace core
{

ModuleHello::ModuleHello():boTests(false)
{
}

ModuleHello::~ModuleHello()
{
}

bool ModuleHello::Init()
{
	Tests();
    return(true);
}

void ModuleHello::Tests()
{
	if (boTests)return;
	boTests = true;
	TestJson2pbRepeatedFields();
	TestCoroutinue();
	TestCoroutinueAuto();
	{
//        m_RunClock.StartClock("TimeStr2time_t");
//        uint32 time(0);
//        for(int i = 0;i < 10000;++i)
//        {
//            time = util::TimeStr2time_t("2017-12-31 23:59:59");
//        }
//        m_RunClock.EndClock();
//        LOG4_INFO("TimeStr2time_t 10000 times time(%u)",time);
//			TimeStr2time_t 10000 times time(1514735999)
//			10000 times EndClock() net::RunClock TimeStr2time_t use time(55.188000) ms
//			10000 times EndClock() net::RunClock TimeStr2time_t use time(65.248001) ms (加上分配uint32 time的时间)
	}
}

bool ModuleHello::AnyMessage(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
	util::CJsonObject obj;
	if (!obj.Parse(oInHttpMsg.body()))
	{
		LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
		return false;
	}
	std::string strOption;
	obj.Get("option",strOption);
	if ("Echo" == strOption)
	{
		Response(stMsgShell,oInHttpMsg,0);
	}
	//PgAgent
	else if ("PgAgentInsert" == strOption)
	{
		InsertPostgres(stMsgShell,oInHttpMsg,obj("val"),"PGAGENT");
	}
	else if ("PgAgentSetGet" == strOption)
	{
		SetGetPostgres(stMsgShell,oInHttpMsg,obj("val"),"PGAGENT");
	}
	else if ("PgAgentSet" == strOption)
	{
		SetPostgres(stMsgShell,oInHttpMsg,obj("val"),"PGAGENT");
	}
	else if ("PgAgentGet" == strOption)
	{
		GetPostgres(stMsgShell,oInHttpMsg,obj("val"),"PGAGENT");
	}
	else if ("PgAgentAddUp" == strOption)
	{
		int id(0);obj.Get("id",id);
		int sum(0);obj.Get("sum",sum);
		AddUpPostgres(stMsgShell,oInHttpMsg,id,obj("name"),sum,"PGAGENT");
	}
	//Proxy
	else if ("ProxyPgAgentSetGet" == strOption)
	{
		SetGetPostgres(stMsgShell,oInHttpMsg,obj("val"),"PROXYSSDB");
	}
	else if ("ProxyPgAgentSet" == strOption)
	{
		SetPostgres(stMsgShell,oInHttpMsg,obj("val"),"PROXYSSDB");
	}
	else if ("ProxyPgAgentGet" == strOption)
	{
		GetPostgres(stMsgShell,oInHttpMsg,obj("val"),"PROXYSSDB");
	}
	else if ("RedisearchAdd" == strOption)
	{
		std::string strVal;
		if (!obj.Get("val",strVal))
		{
			LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
			return false;
		}
		std::string sDoc;
		obj.Get("doc",sDoc);
		RedisearchAdd(stMsgShell,oInHttpMsg,sDoc,strVal);
	}
	else if ("RedisGEOADD" == strOption)
	{
		std::string strVal;
		if (!obj.Get("val",strVal))
		{
			LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
			return false;
		}
		RedisGEOADD(stMsgShell,oInHttpMsg,strVal);
	}
	else if ("SetValueFromRedis" == strOption)
	{
		std::string strVal;
		if (!obj.Get("val",strVal))
		{
			LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
			return false;
		}
		SetValueFromRedis(stMsgShell,oInHttpMsg,strVal);
	}
	else if ("OnlySetValueFromRedis" == strOption)
	{
		std::string strVal;
		if (!obj.Get("val",strVal))
		{
			LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
			return false;
		}
		OnlySetValueFromRedis(stMsgShell,oInHttpMsg,strVal);
	}
	else if ("OnlyGetValueFromRedis" == strOption)
	{
		std::string strVal;
		if (!obj.Get("val",strVal))
		{
			LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
			return false;
		}
		OnlyGetValueFromRedis(stMsgShell,oInHttpMsg,strVal);
	}
	else if ("SetValueFromSSDB" == strOption)
    {
        std::string strVal;
        if (!obj.Get("val",strVal))
        {
            LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
            return false;
        }
        SetValueFromRedis(stMsgShell,oInHttpMsg,strVal,"PROXYSSDB");
    }
    else if ("OnlySetValueFromSSDB" == strOption)
    {
        std::string strVal;
        if (!obj.Get("val",strVal))
        {
            LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
            return false;
        }
        OnlySetValueFromRedis(stMsgShell,oInHttpMsg,strVal,"PROXYSSDB");
    }
    else if ("OnlyGetValueFromSSDB" == strOption)
    {
        std::string strVal;
        if (!obj.Get("val",strVal))
        {
            LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
            return false;
        }
        OnlyGetValueFromRedis(stMsgShell,oInHttpMsg,strVal,"PROXYSSDB");
    }
	else if ("RedisGEORADIUSBYMEMBER" == strOption)
	{
		std::string strVal;
		if (!obj.Get("val",strVal))
		{
			LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
			return false;
		}
		RedisGEORADIUSBYMEMBER(stMsgShell,oInHttpMsg,strVal);
	}
	else if ("RedisbitmapSETBIT" == strOption)
	{
	    std::string strVal;
        if (!obj.Get("val",strVal))
        {
            LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
            return false;
        }
        RedisbitmapSETBIT(stMsgShell,oInHttpMsg,strVal,obj("key"));
	}
	else if ("RedisbitmapGETBIT" == strOption)
    {
        std::string strVal;
        if (!obj.Get("val",strVal))
        {
            LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
            return false;
        }
        RedisbitmapGETBIT(stMsgShell,oInHttpMsg,strVal,obj("key"));
    }
	else if ("RedisbitmapBITPOS" == strOption)
    {
        std::string strVal;
        if (!obj.Get("val",strVal))
        {
            LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
            return false;
        }
        RedisbitmapBITPOS(stMsgShell,oInHttpMsg,strVal,obj("key"));
    }
	else if ("RedisbitmapGET" == strOption)
	{
	    std::string strVal;
        if (!obj.Get("val",strVal))
        {
            LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
            return false;
        }
        RedisbitmapGET(stMsgShell,oInHttpMsg,strVal,obj("key"));
	}
	else if ("RedisbitmapGET_GET" == strOption)
    {
        std::string strVal;
        if (!obj.Get("val",strVal))
        {
            LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
            return false;
        }
        RedisbitmapGET_GET(stMsgShell,oInHttpMsg,strVal,obj("key"),obj("key2"));
    }
	else if ("SsdbMsgHset" == strOption)
    {
        util::CJsonObject strVal;
        if (!obj.Get("val",strVal))
        {
            LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
            return false;
        }
        SsdbMsgHset(stMsgShell,oInHttpMsg,strVal.ToString(),obj("key"));
    }
	else if ("SsdbMsgHgetall" == strOption)
    {
        SsdbMsgHsetall(stMsgShell,oInHttpMsg,obj("key"));
    }
	else if ("SsdbMsgHscan" == strOption)
    {
        SsdbMsgHscan(stMsgShell,oInHttpMsg,obj("start"),obj("key"));
    }
	else if ("TestDBSELECT" == strOption)
	{
		TestDBSELECT(stMsgShell,oInHttpMsg);
	}
	else if ("HttpsGet" == strOption)
	{
		std::string strVal;
		if (!obj.Get("val",strVal))//"https://www.alipay.com"
		{
			LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
			return false;
		}
		std::string strResponse;
		g_pLabor->HttpsGet(strVal,strResponse,"");//util::CurlClient::eContentType_gzip
		LOG4_TRACE("HttpsGet %s",strResponse.c_str());
		Response(stMsgShell,oInHttpMsg,0);
	}
	else if ("HttpsPost" == strOption)
	{
		std::string strVal;
		if (!obj.Get("val",strVal))//"https://www.alipay.com"
		{
			LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
			return false;
		}
		std::string strResponse;
		g_pLabor->HttpsPost(strVal,"",strResponse);
		LOG4_TRACE("HttpsPost %s",strResponse.c_str());
		Response(stMsgShell,oInHttpMsg,0);
	}
	else if ("RSA" == strOption)
	{
		TestRSA();
		Response(stMsgShell,oInHttpMsg,0);
	}
	else if ("TestCoroutinue" == strOption)
	{
		TestCoroutinue();
		Response(stMsgShell,oInHttpMsg,0);
	}
	else if ("TestHttpRequestState" == strOption)
	{
		TestHttpRequestState(stMsgShell,oInHttpMsg);
	}
	else if ("TestHttpRequestStateFunc" == strOption)
	{
		TestHttpRequestStateFunc(stMsgShell,oInHttpMsg);
	}
	else if ("TestHttpRequestStateFuncDataProxy" == strOption)
	{
		std::string strVal;
		if (!obj.Get("val",strVal))
		{
			LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
			return false;
		}
		TestHttpRequestStateFuncDataProxy(stMsgShell,oInHttpMsg,strVal);
	}
	else if ("TestStepCoFuncDataProxy" == strOption)
	{
	    std::string strVal;
        if (!obj.Get("val",strVal))
        {
            LOG4_WARN("failed to parse %s",oInHttpMsg.body().c_str());
            return false;
        }
        TestStepCoFuncDataProxy(stMsgShell,oInHttpMsg,strVal);
	}
	else
	{
		LOG4_TRACE("no things to do");
		Response(stMsgShell,oInHttpMsg,0);
	}
    return(true);
}

void ModuleHello::InsertPostgres(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType)
{
	if (sValue.size() == 0){LOG4_DEBUG("%s() sValue.size() == 0",__FUNCTION__);return;}
	struct DataStepCustom:public net::StepParam
	{
		DataStepCustom(const std::string &node):nodeType(node){}
		std::string nodeType;
	};
	auto callback = [] (const DataMem::MemRsp &oMemRsp,net::Step* pStep)
	{
		LOG4_TRACE("callback %s",oMemRsp.DebugString().c_str());
		util::CJsonObject oRsp;
		oRsp.Add("code", oMemRsp.err_no());
		oRsp.Add("msg", oMemRsp.err_no() ? "failed":"ok");
		pStep->SendToClient(oRsp.ToString());
	};
	net::DbOperator oDbOperator(0,PG_TB_TEST,DataMem::MemOperate::DbOperate::INSERT);
	oDbOperator.AddDbField("name",sValue);
	LOG4_DEBUG("%s() SetPostgres %s",__FUNCTION__,sValue.c_str());
	net::SendToCallback(new net::DataStep(stMsgShell,oInHttpMsg,new DataStepCustom(nodeType)),oDbOperator.MakeMemOperate(),callback,nodeType);
}

void ModuleHello::SetGetPostgres(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType)
{
	if (sValue.size() == 0){LOG4_DEBUG("%s() sValue.size() == 0",__FUNCTION__);return;}
    struct DataStepCustom:public net::StepParam
    {
        DataStepCustom(const std::string &node):nodeType(node){}
        std::string nodeType;
    };
	auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
	{
		LOG4_TRACE("SetValueFromRedis %s",oRsp.err_msg().c_str());
		if (oRsp.err_no() == 0)
		{
			auto GetValueFromPostgres_callback = [] (const DataMem::MemRsp &oMemRsp,net::Step* pStep)
			{
				LOG4_TRACE("GetValueFromPostgres_callback %s",oMemRsp.DebugString().c_str());
				util::CJsonObject oRsp;
				oRsp.Add("code", oMemRsp.err_no());
				oRsp.Add("msg", oMemRsp.err_no() ? "failed":"ok");
				pStep->SendToClient(oRsp.ToString());
			};
			const std::string &node = ((DataStepCustom*) ((net::DataStep*)pStep)->GetData())->nodeType;
			net::DbOperator oDbOperator(0, PG_TB_TEST,DataMem::MemOperate::DbOperate::SELECT);
			oDbOperator.AddDbField("id");
			oDbOperator.AddDbField("name");
			LOG4_TRACE("%s() GetValueFromPostgres_callback",__FUNCTION__);
			if (!net::SendToCallback(pStep,oDbOperator.MakeMemOperate(),GetValueFromPostgres_callback,node))
			{
				LOG4_WARN("%s() SendToCallback failed",__FUNCTION__);
			}
		}
	};
	net::DbOperator oDbOperator(0, PG_TB_TEST,DataMem::MemOperate::DbOperate::UPDATE);
	oDbOperator.AddDbField("name",sValue);
	oDbOperator.AddCondition(DataMem::MemOperate::DbOperate::Condition::EQ,"id",1);
	LOG4_TRACE("%s() SetGetPostgres %s",__FUNCTION__,sValue.c_str());
	net::SendToCallback(stMsgShell,oInHttpMsg,new DataStepCustom(nodeType),oDbOperator.MakeMemOperate(),callback,nodeType);
}

void ModuleHello::GetPostgres(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType)
{
	if (sValue.size() == 0){LOG4_DEBUG("%s() sValue.size() == 0",__FUNCTION__);return;}
	struct DataStepCustom:public net::StepParam
	{
		DataStepCustom(){}
	};
	auto callback = [] (const DataMem::MemRsp &oMemRsp,net::Step* pStep)
	{
		net::DataStep* pDataStep = (net::DataStep*)pStep;
		LOG4_TRACE("GetPostgres_callback %s",oMemRsp.err_msg().c_str());
		if (oMemRsp.err_no() == 0)
		{
			LOG4_TRACE("GetPostgres_callback ok %s",oMemRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", 0);
			oRsp.Add("msg", "ok");
			pDataStep->SendToClient(oRsp.ToString());
		}
		else
		{
			LOG4_TRACE("GetPostgres_callback failed %s",oMemRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", oMemRsp.err_no());
			oRsp.Add("msg", "failed");
			pDataStep->SendToClient(oRsp.ToString());
		}
	};
	net::DbOperator oDbOperator(0,PG_TB_TEST,DataMem::MemOperate::DbOperate::SELECT);
	oDbOperator.AddDbField("id");
	oDbOperator.AddDbField("name");
	LOG4_TRACE("%s() GetPostgres",__FUNCTION__);
	net::SendToCallback(stMsgShell,oInHttpMsg,new DataStepCustom(),oDbOperator.MakeMemOperate(),callback,nodeType);
}

void ModuleHello::SetPostgres(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType)
{
	if (sValue.size() == 0){LOG4_DEBUG("%s() sValue.size() == 0",__FUNCTION__);return;}
	struct DataStepCustom:public net::StepParam
	{
		DataStepCustom(const std::string &node):nodeType(node){}
		std::string nodeType;
	};
	auto callback = [] (const DataMem::MemRsp &oMemRsp,net::Step* pStep)
	{
		LOG4_TRACE("callback %s",oMemRsp.err_msg().c_str());
		if (oMemRsp.err_no() == 0)
		{
			LOG4_TRACE("callback ok %s",oMemRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", 0);
			oRsp.Add("msg", "ok");
			pStep->SendToClient(oRsp.ToString());
		}
		else
		{
			LOG4_TRACE("callback failed %s",oMemRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", oMemRsp.err_no());
			oRsp.Add("msg", "failed");
			pStep->SendToClient(oRsp.ToString());
		}
	};
	net::DbOperator oDbOperator(0, PG_TB_TEST,DataMem::MemOperate::DbOperate::UPDATE);
	oDbOperator.AddDbField("name",sValue);
	oDbOperator.AddCondition(DataMem::MemOperate::DbOperate::Condition::EQ,"id",1);
	LOG4_DEBUG("%s() SetPostgres %s",__FUNCTION__,sValue.c_str());
	net::SendToCallback(stMsgShell,oInHttpMsg,new DataStepCustom(nodeType),oDbOperator.MakeMemOperate(),callback,nodeType);
}


void ModuleHello::AddUpPostgres(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,uint32 id,const std::string &sName,uint32 sum,const std::string &nodeType)
{
	if (sName.size() == 0 || sum == 0 || id == 0){LOG4_DEBUG("%s() sName.size() == 0 || sum == 0 || id == 0",__FUNCTION__);return;}
	struct DataStepCustom:public net::StepParam
	{
		DataStepCustom(const std::string &node):nodeType(node){}
		std::string nodeType;
	};
	auto callback = [] (const DataMem::MemRsp &oMemRsp,net::Step* pStep)
	{
		net::DataStep* pDataStep = (net::DataStep*)pStep;
		LOG4_TRACE("callback %s",oMemRsp.err_msg().c_str());
		if (oMemRsp.err_no() == 0)
		{
			LOG4_TRACE("callback ok %s",oMemRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", 0);
			oRsp.Add("msg", "ok");
			pDataStep->SendToClient(oRsp.ToString());
		}
		else
		{
			LOG4_TRACE("callback failed %s",oMemRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", oMemRsp.err_no());
			oRsp.Add("msg", "failed");
			pDataStep->SendToClient(oRsp.ToString());
		}
	};
	net::DbOperator oDbOperator(0, "tb_sum",DataMem::MemOperate::DbOperate::CUSTOM);
	char strSql[512];
	snprintf(strSql,sizeof(strSql),"INSERT INTO tb_sum(id,name,sum) VALUES(%u,'%s',%u) on conflict (id) do update set sum=tb_sum.sum+%u",id,sName.c_str(),sum,sum);
	oDbOperator.AddDbField(strSql);
	LOG4_DEBUG("%s() AddUpPostgres %s",__FUNCTION__,sName.c_str());
	net::SendToCallback(stMsgShell,oInHttpMsg,new DataStepCustom(nodeType),oDbOperator.MakeMemOperate(),callback,nodeType);
}

/*
ssdb write + read
Transactions:                  90000 hits
Availability:                 100.00 %
Elapsed time:                  14.67 secs
Data transferred:               1.80 MB
Response time:                  0.05 secs
Transaction rate:            6134.97 trans/sec
Throughput:                     0.12 MB/sec
Concurrency:                  298.36
Successful transactions:       90000
Failed transactions:               0
Longest transaction:            0.12
Shortest transaction:           0.01
 * */
void ModuleHello::SetValueFromRedis(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType)
{
    struct DataStepCustom:public net::StepParam
    {
        DataStepCustom(const std::string &node):nodeType(node){}
        std::string nodeType;
    };
	auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
	{
		LOG4_TRACE("SetValueFromRedis %s",oRsp.err_msg().c_str());
		if (oRsp.err_no() == 0)
		{
			auto GetValueFromRedis_callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
			{
				LOG4_TRACE("GetValueFromRedis %s",oRsp.err_msg().c_str());
				if (oRsp.err_no() == 0)
				{
					LOG4_TRACE("GetValueFromRedis_callback ok %s",oRsp.DebugString().c_str());
					util::CJsonObject oRsp;
					oRsp.Add("code", 0);
					oRsp.Add("msg", "ok");
					pStep->SendToClient(oRsp.ToString());
				}
			};
			const std::string &node = ((DataStepCustom*) ((net::DataStep*)pStep)->GetData())->nodeType;
			char sRedisKey[64];
			snprintf(sRedisKey,sizeof(sRedisKey),TEST_SSDB_KEY);
			net::RedisOperator oRedisOperator(0, sRedisKey,"","GET");
			LOG4_TRACE("%s() GetValueFromRedis",__FUNCTION__);
			if (!net::SendToCallback(pStep,oRedisOperator.MakeMemOperate(),GetValueFromRedis_callback,node))
			{
				LOG4_WARN("%s() SendToCallback failed",__FUNCTION__);
			}
		}
	};
	char sRedisKey[64];
	snprintf(sRedisKey,sizeof(sRedisKey),TEST_SSDB_KEY);
	net::RedisOperator oRedisOperator(0, sRedisKey,"SET");
	oRedisOperator.AddRedisField("",sValue);
	LOG4_DEBUG("%s() SetValueFromRedis %s",__FUNCTION__,sValue.c_str());
	net::SendToCallback(stMsgShell,oInHttpMsg,new DataStepCustom(nodeType),oRedisOperator.MakeMemOperate(),callback,nodeType);
}
/*
ssdb write
Transactions:                  90000 hits
Availability:                 100.00 %
Elapsed time:                  11.99 secs
Data transferred:               1.80 MB
Response time:                  0.04 secs
Transaction rate:            7506.26 trans/sec
Throughput:                     0.15 MB/sec
Concurrency:                  298.88
Successful transactions:       90000
Failed transactions:               0
Longest transaction:            0.08
Shortest transaction:           0.01
 * */
void ModuleHello::OnlySetValueFromRedis(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType)
{
	auto SetValueFromRedis_callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
	{
		LOG4_TRACE("OnlySetValueFromRedis %s",oRsp.err_msg().c_str());
		if (oRsp.err_no() == 0)
		{
			LOG4_TRACE("OnlySetValueFromRedis ok %s",oRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", 0);
			oRsp.Add("msg", "ok");
			pStep->SendToClient(oRsp.ToString());
		}
	};
	char sRedisKey[64];
	snprintf(sRedisKey,sizeof(sRedisKey),TEST_SSDB_KEY);
	net::RedisOperator oRedisOperator(0, sRedisKey,"SET");
	oRedisOperator.AddRedisField("",sValue);
	LOG4_DEBUG("%s() OnlySetValueFromRedis %s",__FUNCTION__,sValue.c_str());
	if (!net::SendToCallback(stMsgShell,oInHttpMsg,oRedisOperator.MakeMemOperate(),SetValueFromRedis_callback,nodeType))
	{
		LOG4_WARN("%s() SendToCallback failed",__FUNCTION__);
	}
}
/*
Transactions:                  90000 hits
Availability:                 100.00 %
Elapsed time:                  11.69 secs
Data transferred:               1.80 MB
Response time:                  0.04 secs
Transaction rate:            7698.89 trans/sec
Throughput:                     0.15 MB/sec
Concurrency:                  298.52
Successful transactions:       90000
Failed transactions:               0
Longest transaction:            0.07
Shortest transaction:           0.00
 * */
void ModuleHello::OnlyGetValueFromRedis(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType)
{
	auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
	{
		LOG4_TRACE("OnlyGetValueFromRedis %s",oRsp.err_msg().c_str());
		if (oRsp.err_no() == 0)
		{
			LOG4_TRACE("OnlyGetValueFromRedis ok %s",oRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", 0);
			oRsp.Add("msg", "ok");
			pStep->SendToClient(oRsp.ToString());
		}
	};
	char sRedisKey[64];
	snprintf(sRedisKey,sizeof(sRedisKey),TEST_SSDB_KEY);
	net::RedisOperator oRedisOperator(0, sRedisKey,"","GET");
	LOG4_DEBUG("%s() OnlyGetValueFromRedis",__FUNCTION__);
	if (!net::SendToCallback(stMsgShell,oInHttpMsg,oRedisOperator.MakeMemOperate(),callback,nodeType))
	{
		LOG4_WARN("%s() SendToCallback failed",__FUNCTION__);
	}
}
/*
FT.ADD {index} {docId} {score}
  [NOSAVE]
  [REPLACE [PARTIAL]]
  [LANGUAGE {language}]
  [PAYLOAD {payload}]
  FIELDS {field} {value} [{field} {value}...]
 * */
void ModuleHello::RedisearchAdd(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sDoc,const std::string &sValue)
{
	if (sDoc.size() == 0||sValue.size() == 0)return;
	/*
	FT.ADD IDX docCn3 1.0 LANGUAGE chinese FIELDS txt "你好罗朋友"
	FT.ADD IDX docCn5 0.5 REPLACE LANGUAGE chinese FIELDS txt "你好罗朋友3"
	FT.SEARCH IDX "你好"
	 * */
	auto RedisearchAdd_callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
	{
		LOG4_TRACE("RedisearchAdd_callback %s",oRsp.err_msg().c_str());
		if (oRsp.err_no() == 0)
		{
			LOG4_TRACE("RedisearchAdd_callback ok %s",oRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", 0);
			oRsp.Add("msg", "ok");
			pStep->SendToClient(oRsp.ToString());
		}
	};
	char sRedisWriteCmd[128];
	snprintf(sRedisWriteCmd,sizeof(sRedisWriteCmd),"FT.ADD");
	net::RedisOperator oRedisOperator(0, "IDX",sRedisWriteCmd,"");
	oRedisOperator.AddRedisField(sDoc,1.0);//docCn3 1.0
	oRedisOperator.AddRedisField("REPLACE");
	oRedisOperator.AddRedisField("LANGUAGE","chinese");//LANGUAGE chinese
	oRedisOperator.AddRedisField("FIELDS","txt");
	oRedisOperator.AddRedisField("",sValue);

	if (!net::SendToCallback(stMsgShell,oInHttpMsg,oRedisOperator.MakeMemOperate(),RedisearchAdd_callback))
	{
		LOG4_WARN("%s() SendToCallback failed",__FUNCTION__);
	}
}
/*
FT.SEARCH http://redisearch.io/Commands/
Format
FT.SEARCH {index} {query} [NOCONTENT] [VERBATIM] [NOSTOPWORDS] [WITHSCORES] [WITHPAYLOADS] [WITHSORTKEYS]
  [FILTER {numeric_field} {min} {max}] ...
  [GEOFILTER {geo_field} {lon} {lat} {raius} m|km|mi|ft]
  [INKEYS {num} {key} ... ]
  [INFIELDS {num} {field} ... ]
  [RETURN {num} {field} ... ]
  [SUMMARIZE [FIELDS {num} {field} ... ] [FRAGS {num}] [LEN {fragsize}] [SEPARATOR {separator}]]
  [HIGHLIGHT [FIELDS {num} {field} ... ] [TAGS {open} {close}]]
  [SLOP {slop}] [INORDER]
  [LANGUAGE {language}]
  [EXPANDER {expander}]
  [SCORER {scorer}]
  [PAYLOAD {payload}]
  [SORTBY {field} [ASC|DESC]]
  [LIMIT offset num]
 * */
void ModuleHello::RedisearchSearch(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue)
{
	/*
	./src/redis-cli FT.ADD IDX docCn3 1.0 LANGUAGE chinese FIELDS txt "你好罗朋友"
	./src/redis-cli FT.SEARCH IDX "你好"
	 * */
	auto RedisearchSearch_callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
	{
		LOG4_TRACE("RedisearchSearch_callback %s",oRsp.err_msg().c_str());
		if (oRsp.err_no() == 0)
		{
			LOG4_TRACE("RedisearchSearch_callback ok %s",oRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", 0);
			oRsp.Add("msg", "ok");
			pStep->SendToClient(oRsp.ToString());
		}
	};
	char sRedisReadCmd[128];
	snprintf(sRedisReadCmd,sizeof(sRedisReadCmd),"FT.SEARCH");
	net::RedisOperator oRedisOperator(0, "IDX","",sRedisReadCmd);
	oRedisOperator.AddRedisField("",sValue);
	if (!net::SendToCallback(stMsgShell,oInHttpMsg,oRedisOperator.MakeMemOperate(),RedisearchSearch_callback))
	{
		LOG4_WARN("%s() SendToCallback failed",__FUNCTION__);
	}
}
/*
返回结果
[imdev@node3 redis-4.0.9]$ ./src/redis-cli -h 192.168.18.68 -p 6379 FT.SEARCH IDX '你好'
1) (integer) 3
2) "docCn2"
3) 1) "txt"
   2) "\xe4\xbd\xa0\xe5\xa5\xbd\xe7\xbd\x97\xe6\x9c\x8b\xe5\x8f\x8b2"
4) "docCn1"
5) 1) "txt"
   2) "\xe4\xbd\xa0\xe5\xa5\xbd\xe7\xbd\x97\xe6\x9c\x8b\xe5\x8f\x8b1"
6) "docCn3"
7) 1) "txt"
   2) "\xe4\xbd\xa0\xe5\xa5\xbd\xe7\xbd\x97\xe6\x9c\x8b\xe5\x8f\x8b3"

err_no: 0
err_msg: "OK"
totalcount: 10
curcount: 10
record_data {  field_info {   col_value: "3"}}
record_data { field_info {   col_value: "docCn2" }}
record_data {  field_info {    col_value: "txt"  }}
record_data {  field_info {    col_value: "\344\275\240\345\245\275\347\275\227\346\234\213\345\217\2132"  }}
record_data {  field_info {    col_value: "docCn1"  }}
record_data {  field_info {    col_value: "txt"  }}
record_data {  field_info {    col_value: "\344\275\240\345\245\275\347\275\227\346\234\213\345\217\2131"  }}
record_data {  field_info {    col_value: "docCn3"  }}
record_data {  field_info {    col_value: "txt"  }}
record_data {  field_info {    col_value: "\344\275\240\345\245\275\347\275\227\346\234\213\345\217\2133"  }}
from: 1
 * */


/*
 * ./src/redis-cli -h 192.168.18.68 -p 6379 GEOADD Guangdong-cities 113.2278442 23.1255978 Guangzhou 113.106308 23.0088312 Foshan 113.7943267 22.9761989 Dongguan 114.0538788 22.5551603 Shenzhen
 * */
void ModuleHello::RedisGEOADD(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue)
{
	auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
	{
		LOG4_TRACE("RedisGEOADD_callback %s",oRsp.err_msg().c_str());
		if (oRsp.err_no() == 0)
		{
			LOG4_TRACE("RedisGEOADD_callback ok %s",oRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", 0);
			oRsp.Add("msg", "ok");
			pStep->SendToClient(oRsp.ToString());
		}
	};
	char sRedisCmd[128];
	snprintf(sRedisCmd,sizeof(sRedisCmd),"GEOADD");
	net::RedisOperator oRedisOperator(0, "Guangdong-cities",sRedisCmd);
	oRedisOperator.AddRedisField("",113.2278442);
	oRedisOperator.AddRedisField("",23.1255978);
	oRedisOperator.AddRedisField("",sValue);
	if (!net::SendToCallback(stMsgShell,oInHttpMsg,oRedisOperator.MakeMemOperate(),callback))
	{
		LOG4_WARN("%s() SendToCallback failed",__FUNCTION__);
	}
}
/*
 * ./src/redis-cli -h 192.168.18.68 -p 6379 GEORADIUSBYMEMBER Guangdong-cities Shenzhen 200 km withdist
1) 1) "Shenzhen"
   2) "0.0000"
2) 1) "Foshan"
   2) "109.4925"
3) 1) "Guangzhou"
   2) "105.8068"
4) 1) "Dongguan"
   2) "53.8680"
5) 1) "Qingyuan"
   2) "144.2208"
 * */
void ModuleHello::RedisGEORADIUSBYMEMBER(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue)
{
	auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
	{
		LOG4_TRACE("RedisearchAdd_callback %s",oRsp.err_msg().c_str());
		if (oRsp.err_no() == 0)
		{
			LOG4_TRACE("RedisearchAdd_callback ok %s",oRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", 0);
			oRsp.Add("msg", "ok");
			pStep->SendToClient(oRsp.ToString());
		}
	};
	char sRedisCmd[128];
	snprintf(sRedisCmd,sizeof(sRedisCmd),"GEORADIUSBYMEMBER");
	net::RedisOperator oRedisOperator(0, "Guangdong-cities",sRedisCmd);
	oRedisOperator.AddRedisField("",sValue);//Shenzhen
	oRedisOperator.AddRedisField("200","km");
	oRedisOperator.AddRedisField("","withdist");//返回距离
	if (!net::SendToCallback(stMsgShell,oInHttpMsg,oRedisOperator.MakeMemOperate(),callback))
	{
		LOG4_WARN("%s() SendToCallback failed",__FUNCTION__);
	}
}
/*
返回结果
[imdev@node3 redis-4.0.9]$ ./src/redis-cli -h 192.168.18.68 -p 6379 GEORADIUSBYMEMBER Guangdong-cities Shenzhen 200 km withdist
1) 1) "Shenzhen"
   2) "0.0000"
2) 1) "Foshan"
   2) "109.4925"
3) 1) "Guangzhou"
   2) "105.8068"
4) 1) "Huizhou"
   2) "105.8068"
5) 1) "Huizhou1"
   2) "105.8068"
6) 1) "Huizhou2"
   2) "105.8068"
7) 1) "\xe6\xa0\xa1\xe9\x95\xbf\xe5\xa5\xbd\xef\xbc\x8c\xe9\xa6\x96\xe9\x95\xbf\xe5\xa5\xbd"
   2) "105.8068"
8) 1) "Dongguan"
   2) "53.8680"
9) 1) "Qingyuan"
   2) "144.2208"

err_no: 0
err_msg: "OK"
totalcount: 18
curcount: 18
record_data {  field_info {    col_value: "Shenzhen"  }}
record_data {  field_info {    col_value: "0.0000"  }}
record_data {  field_info {    col_value: "Foshan"  }}
record_data {  field_info {    col_value: "109.4925"  }}
record_data {  field_info {    col_value: "Guangzhou"  }}
record_data {  field_info {    col_value: "105.8068"  }}
record_data {  field_info {    col_value: "Huizhou"  }}
record_data {  field_info {    col_value: "105.8068"  }}
record_data {  field_info {    col_value: "Huizhou1"  }}
record_data {  field_info {    col_value: "105.8068"  }}
record_data {  field_info {    col_value: "Huizhou2"  }}
record_data {  field_info {    col_value: "105.8068"  }}
record_data {  field_info {    col_value: "\346\240\241\351\225\277\345\245\275\357\274\214\351\246\226\351\225\277\345\245\275"  }}
record_data {  field_info {    col_value: "105.8068"  }}
record_data {  field_info {    col_value: "Dongguan"  }}
record_data {  field_info {    col_value: "53.8680"  }}
record_data {  field_info {    col_value: "Qingyuan"  }}
record_data {  field_info {    col_value: "144.2208"  }}
from: 1
 * */

//SETBIT 4:4:SETBIT 10001 1
void ModuleHello::RedisbitmapSETBIT(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &sKey,const std::string &sNode)
{
	auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
	{
		LOG4_TRACE("RedisbitmapSETBIT %s",oRsp.err_msg().c_str());
		if (oRsp.err_no() == 0)
		{
			LOG4_TRACE("RedisbitmapSETBIT ok %s",oRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", 0);
			oRsp.Add("msg", "ok");
			pStep->SendToClient(oRsp.ToString());
		}
	};
	net::RedisOperator oRedisOperator(0, sKey.size() > 0?sKey:SETBIT_KEY,"SETBIT");
	oRedisOperator.AddRedisField("",sValue);//10001
	oRedisOperator.AddRedisField("",1);
	LOG4_DEBUG("%s() RedisbitmapSETBIT %s",__FUNCTION__,sValue.c_str());
	net::SendToCallback(stMsgShell,oInHttpMsg,oRedisOperator.MakeMemOperate(),callback,sNode);
}
//GETBIT 4:4:SETBIT 10001
void ModuleHello::RedisbitmapGETBIT(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &sKey,const std::string &sNode)
{
    auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
    {
        LOG4_TRACE("RedisbitmapGETBIT %s",oRsp.err_msg().c_str());
        if (oRsp.err_no() == 0)
        {
            LOG4_TRACE("RedisbitmapGETBIT ok %s",oRsp.DebugString().c_str());
            util::CJsonObject oRsp;
            oRsp.Add("code", 0);
            oRsp.Add("msg", "ok");
            pStep->SendToClient(oRsp.ToString());
        }
    };
    net::RedisOperator oRedisOperator(0, sKey.size() > 0?sKey:SETBIT_KEY,"GETBIT");
    oRedisOperator.AddRedisField("",sValue);//10001
    LOG4_DEBUG("%s() RedisbitmapGETBIT %s",__FUNCTION__,sValue.c_str());
    net::SendToCallback(stMsgShell,oInHttpMsg,oRedisOperator.MakeMemOperate(),callback,sNode);
}
//BITPOS 4:4:SETBIT 1
void ModuleHello::RedisbitmapBITPOS(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &sKey,const std::string &sNode)
{
    auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
    {
        LOG4_TRACE("RedisbitmapBITPOS %s",oRsp.err_msg().c_str());
        if (oRsp.err_no() == 0)
        {
            LOG4_TRACE("RedisbitmapBITPOS ok %s",oRsp.DebugString().c_str());
            util::CJsonObject oRsp;
            oRsp.Add("code", 0);
            oRsp.Add("msg", "ok");
            pStep->SendToClient(oRsp.ToString());
        }
    };
    net::RedisOperator oRedisOperator(0, sKey.size() > 0?sKey:SETBIT_KEY,"BITPOS");
    oRedisOperator.AddRedisField("",1);//1
    LOG4_DEBUG("%s() RedisbitmapBITPOS %s",__FUNCTION__,sValue.c_str());
    net::SendToCallback(stMsgShell,oInHttpMsg,oRedisOperator.MakeMemOperate(),callback,sNode);
}
/*
获取2011年11月1日活跃用户列表
首先获取指定的二进制字符串
get 4:4:online?20111101

计算总数count （相当于bitcount）
Int count = 0;
while(value) {
value&= (value- 1);
count ++;
}
获取其中的用户列表(从小到大顺序获取)
vector<int> users;
Int count = 0;
Int tempvalue = value;
while(value) {
value&= (value- 1);
count ++;
users.emplace_back(tempvalue & value);
tempvalue = value;
}
如果需要指定数量（或者指定分页，则只要使用count 来控制）
vector<int> users;
Int count = 0;
Int tempvalue = value;
while(value) {
value&= (value- 1);
If (count >= 10 &&count < 20 )//第二页，每页10个
users.emplace_back(tempvalue & value);
If (count >=20)break;
tempvalue = value;
count ++;
}
 * */
void ModuleHello::RedisbitmapGET(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &sKey,const std::string &sNode)
{
    auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
    {
        LOG4_TRACE("RedisbitmapGET %s",oRsp.err_msg().c_str());
        if (oRsp.err_no() == 0)
        {
            net::DataStep* pDataStep = (net::DataStep*)pStep;
            LOG4_TRACE("RedisbitmapGET ok %s",oRsp.DebugString().c_str());
            if (oRsp.record_data_size() > 0 && oRsp.record_data(0).field_info_size() > 0)
            {
                const std::string& col_value = oRsp.record_data(0).field_info(0).col_value();
                LOG4_TRACE("RedisbitmapGET col_value %s,size:%u",col_value.c_str(),col_value.size());
                std::vector<uint32> usersData;
                ModuleHello::String2UserData(col_value,usersData);
//                计算总数count （相当于bitcount）
                {
                    LOG4_TRACE("count %d",usersData.size());
                }
//                获取其中的用户列表(从小到大顺序获取)
                {
                    for(auto imid:usersData)
                    LOG4_TRACE("user imid:%u",imid);
                }
//                如果需要指定数量（或者指定分页，则只要使用usersData.size() 来控制）
                {
                    for(uint32 i = 0;i < 10 && i < usersData.size();++i)
                    LOG4_TRACE("user imid:%u",usersData[i]);
                }
            }
            util::CJsonObject oRsp;
            oRsp.Add("code", 0);
            oRsp.Add("msg", "ok");
            pDataStep->SendToClient(oRsp.ToString());
        }
    };
    net::RedisOperator oRedisOperator(0, sKey.size() > 0?sKey:SETBIT_KEY,"","GET");
    LOG4_DEBUG("%s() RedisbitmapGET",__FUNCTION__);
    net::SendToCallback(stMsgShell,oInHttpMsg,oRedisOperator.MakeMemOperate(),callback,sNode);
}

void ModuleHello::RedisbitmapGET_GET(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,
                const std::string &sValue,const std::string &sKey1,const std::string &sKey2,const std::string &sNode)
{
    struct DataStepCustom:public net::StepParam
    {
        DataStepCustom(const std::string &sK2,const std::string& node):
            sKey2(sK2),strNode(node){m_RunClock.StartClock("net::RunClock RedisbitmapGET_GET");}
        ~DataStepCustom(){m_RunClock.EndClock();}//EndClock() net::RunClock net::RunClock RedisbitmapGET_GET use time(4.998000) ms
        std::string sKey2;
        std::string strNode;

        std::vector<uint32> usersData1;
        std::vector<uint32> usersData2;
        net::RunClock m_RunClock;
    };
    auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
    {
        LOG4_TRACE("RedisbitmapGET %s",oRsp.err_msg().c_str());
        if (oRsp.err_no() == 0)
        {
            LOG4_TRACE("RedisbitmapGET ok %s",oRsp.DebugString().c_str());
            if (oRsp.record_data_size() > 0 && oRsp.record_data(0).field_info_size() > 0)
            {
                const std::string& col_value = oRsp.record_data(0).field_info(0).col_value();
                LOG4_TRACE("RedisbitmapGET col_value %s,size:%u",col_value.c_str(),col_value.size());
                ModuleHello::String2UserData(col_value,((DataStepCustom*) pStep->GetData())->usersData1);

                auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
                {
                    LOG4_TRACE("RedisbitmapGET %s",oRsp.err_msg().c_str());
                    if (oRsp.err_no() == 0)
                    {
                        LOG4_TRACE("RedisbitmapGET ok %s",oRsp.DebugString().c_str());
                        if (oRsp.record_data_size() > 0 && oRsp.record_data(0).field_info_size() > 0)
                        {
                        	DataStepCustom* pDataStepCustom = (DataStepCustom*)pStep->GetData();
                            const std::string& col_value = oRsp.record_data(0).field_info(0).col_value();
                            ModuleHello::String2UserData(col_value,pDataStepCustom->usersData2);
                            const std::vector<uint32>& usersData1 = pDataStepCustom->usersData1;
                            const std::vector<uint32>& usersData2 = pDataStepCustom->usersData2;
                            ModuleHello::OPUserData(usersData1,usersData2);
                            util::CJsonObject oRsp;
                            oRsp.Add("code", 0);
                            oRsp.Add("msg", "ok");
                            pStep->SendToClient(oRsp.ToString());
                        }
                    }
                };
                net::RedisOperator oRedisOperator(0, ((DataStepCustom*) pStep->GetData())->sKey2,"","GET");
                LOG4_TRACE("%s() RedisbitmapGET usersData2",__FUNCTION__);
                net::SendToCallback(pStep,oRedisOperator.MakeMemOperate(),callback,((DataStepCustom*) pStep->GetData())->strNode);
            }
        }
    };

    net::RedisOperator oRedisOperator(0, sKey1.size() > 0?sKey1:SETBIT_KEY,"","GET");
    LOG4_DEBUG("%s() RedisbitmapGET usersData1",__FUNCTION__);
    net::SendToCallback(stMsgShell,oInHttpMsg,new DataStepCustom(sKey2.size() > 0?sKey2:SETBIT_KEY,sNode),oRedisOperator.MakeMemOperate(),callback,sNode);
}

void ModuleHello::String2UserData(const std::string & col_value,std::vector<uint32>& usersData)
{
   int len = col_value.size();
   const char *data = col_value.data();
   for (;len;len--)
   {
       int pos = col_value.size() - len;
       if (*(data + pos))
       {
           LOG4_TRACE("pos %u",pos);
           char c = * (data + pos);
           for(int i =7;i > -1;--i)
           {
               if (c & (1<<i))
               {
                   usersData.push_back(pos * 8 + (7 - i));
                   LOG4_TRACE("usersData %u",pos * 8 + (7 - i));
               }
           }

       }
   }
}

void ModuleHello::OPUserData(const std::vector<uint32>& usersData1,const std::vector<uint32>& usersData2)
{
    //usersData1  usersData2
    for(auto i:usersData1)LOG4_TRACE("usersData1 %u",i);
    for(auto i:usersData2)LOG4_TRACE("usersData2 %u",i);

    //union  usersData1|usersData2 需要排好序,usersData1和usersData2并集
    {
        std::vector<uint32> usersDataUnion;
        usersDataUnion.resize(usersData1.size() + usersData2.size(),0);
        std::vector<uint32>::iterator set_unionEnd = std::set_union(usersData1.begin(),usersData1.end(),usersData2.begin(),usersData2.end(),usersDataUnion.begin());
        usersDataUnion.resize(set_unionEnd - usersDataUnion.begin());
        for(auto i:usersDataUnion)LOG4_TRACE("usersDataUnion %u",i);
    }
    //Intersection  usersData1&usersData2 需要排好序,usersData1和usersData2交集
    {
        std::vector<uint32> usersIntersection;
        usersIntersection.resize(usersData1.size() + usersData2.size(),0);
        std::vector<uint32>::iterator set_intersectionEnd = std::set_intersection (usersData1.begin(),usersData1.end(),usersData2.begin(),usersData2.end(),usersIntersection.begin());
        usersIntersection.resize(set_intersectionEnd - usersIntersection.begin());
        for(auto i:usersIntersection)LOG4_TRACE("usersIntersection %u",i);
    }
    //Intersection 需要排好序,usersData1有但usersData2没有的
    {
        std::vector<uint32> usersDifference;
        usersDifference.resize(usersData1.size() + usersData2.size(),0);
        std::vector<uint32>::iterator set_differenceEnd = std::set_difference(usersData1.begin(),usersData1.end(),usersData2.begin(),usersData2.end(),usersDifference.begin());
        usersDifference.resize(set_differenceEnd - usersDifference.begin());
        for(auto i:usersDifference)LOG4_TRACE("usersDifference %u",i);
    }
}

void ModuleHello::SsdbMsgHset(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &sKey,const std::string &sNode)
{
    auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
    {
        LOG4_TRACE("SsdbMsg %s",oRsp.err_msg().c_str());
        if (oRsp.err_no() == 0)
        {
            net::DataStep* pDataStep = (net::DataStep*)pStep;
            LOG4_TRACE("SsdbMsg ok %s",oRsp.DebugString().c_str());
            util::CJsonObject oRsp;
            oRsp.Add("code", 0);
            oRsp.Add("msg", "ok");
            pDataStep->SendToClient(oRsp.ToString());
        }
    };
    char strStorageKey[64];
    snprintf(strStorageKey,sizeof(strStorageKey),"%s?%u",sKey.size() > 0?sKey.c_str():MSG_KEY,util::GetDateUint32(::time(NULL)));//1:11:MSG?20111101
    net::RedisOperator oRedisOperator(0, strStorageKey,"HSET");
    oRedisOperator.AddRedisField("",util::GetUniqueId(net::GetNodeId(),net::GetWorkerIndex()));
    oRedisOperator.AddRedisField("",sValue);// 1:11:MSG   {json}
    LOG4_DEBUG("%s() SsdbMsgHset %s,%s",__FUNCTION__,strStorageKey,sValue.c_str());
    net::SendToCallback(stMsgShell,oInHttpMsg,oRedisOperator.MakeMemOperate(),callback,sNode);
}

void ModuleHello::SsdbMsgHsetall(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sKey,const std::string &sNode)
{
    auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
    {
        LOG4_TRACE("SsdbMsgHsetall %s",oRsp.err_msg().c_str());
        if (oRsp.err_no() == 0)
        {
            LOG4_TRACE("SsdbMsgHsetall ok %s",oRsp.DebugString().c_str());
            util::CJsonObject oRsp;
            oRsp.Add("code", 0);
            oRsp.Add("msg", "ok");
            pStep->SendToClient(oRsp.ToString());
        }
    };
    char strStorageKey[64];
    snprintf(strStorageKey,sizeof(strStorageKey),"%s?%u",sKey.size() > 0?sKey.c_str():MSG_KEY,util::GetDateUint32(::time(NULL)));//1:11:MSG?20111101
    net::RedisOperator oRedisOperator(0, sKey.size() > 0?sKey:MSG_KEY,"HGETALL");
    LOG4_DEBUG("%s() SsdbMsgHsetall %s",__FUNCTION__,strStorageKey);
    net::SendToCallback(new net::DataStep(stMsgShell,oInHttpMsg),oRedisOperator.MakeMemOperate(),callback,sNode);
}

void ModuleHello::SsdbMsgHscan(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &key_start,const std::string &sKey,const std::string &sNode)
{
    auto callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
    {
        LOG4_TRACE("SsdbMsgHscan %s",oRsp.err_msg().c_str());
        if (oRsp.err_no() == 0)
        {
            LOG4_TRACE("SsdbMsg ok %s",oRsp.DebugString().c_str());
            util::CJsonObject oRsp;
            oRsp.Add("code", 0);
            oRsp.Add("msg", "ok");
            pStep->SendToClient(oRsp.ToString());
        }
    };
    //hscan 1:11:MSG?20111101 "" "" 10
    char strStorageKey[64];
    snprintf(strStorageKey,sizeof(strStorageKey),"%s?%u",sKey.size() > 0?sKey.c_str():MSG_KEY,util::GetDateUint32(::time(NULL)));//1:11:MSG?20111101
    net::RedisOperator oRedisOperator(0, sKey.size() > 0?sKey:MSG_KEY,"hscan");
    oRedisOperator.AddRedisField("","");
    oRedisOperator.AddRedisField("","");
    oRedisOperator.AddRedisField("",10);
    LOG4_DEBUG("%s() SsdbMsgHscan %s,%s",__FUNCTION__,strStorageKey,key_start.c_str());
    net::SendToCallback(stMsgShell,oInHttpMsg,oRedisOperator.MakeMemOperate(),callback,sNode);
}

void ModuleHello::TestDBSELECT(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
	util::CJsonObject oRequest;
	if (!oRequest.Parse(oInHttpMsg.body())) return;
	std::string strTableName;
	std::string strfield;
	std::string strvalue;
	oRequest.Get("table",strTableName);
	oRequest.Get("field", strfield);
	oRequest.Get("value", strvalue);
	if (strfield.size() == 0 || strvalue.size() == 0 || strTableName.size() == 0)return;

	auto TestDBSELECT_callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
	{
		LOG4_TRACE("TestDBSELECT_callback %s",oRsp.err_msg().c_str());
		if (oRsp.err_no() == 0)
		{
			LOG4_TRACE("TestDBSELECT_callback ok %s",oRsp.DebugString().c_str());
			util::CJsonObject oRsp;
			oRsp.Add("code", 0);
			oRsp.Add("msg", "ok");
			pStep->SendToClient(oRsp.ToString());
		}
	};
	net::DbOperator oDbOper(0, strTableName, DataMem::MemOperate::DbOperate::SELECT);
	oDbOper.AddDbField("ip");
	oDbOper.AddCondition(DataMem::MemOperate::DbOperate::Condition::E_RELATION::MemOperate_DbOperate_Condition_E_RELATION_EQ,strfield,strvalue);
	if (!net::SendToCallback(stMsgShell,oInHttpMsg,oDbOper.MakeMemOperate(),TestDBSELECT_callback))
	{
		LOG4_WARN("%s() SendToCallback failed",__FUNCTION__);
	}
}

void ModuleHello::TestRSA()
{
	//待加密的字符串
	std::string message = "http://www.baidu.com";
	LOG4_TRACE("to deal message = %s, length = %d\n", message.c_str(), strlen(message.c_str()));
	/*
	//自动生成随机数据
	byte seed[600] = "";
	AutoSeededRandomPool rnd;
	rnd.GenerateBlock(seed, sizeof(seed));
	printf("seed = %s\n", (char *)seed, strlen((char *)seed));

	//生成加密的高质量伪随机字节播种池一体化后的熵
	RandomPool randPool;
	randPool.Put(seed, sizeof(seed));
	*/

	CryptoPP::AutoSeededRandomPool rnd;
	CryptoPP::InvertibleRSAFunction params;
	params.GenerateRandomWithKeySize(rnd, 1024);

	LOG4_TRACE("privateKey publicKey");
	CryptoPP::RSA::PrivateKey privateKey(params);
	CryptoPP::RSA::PublicKey publicKey(params);
	{
		LOG4_TRACE("使用OAEP模式");
		//使用OAEP模式
		//RSAES_OAEP_SHA_Decryptor pri(randPool, sizeof(seed));
		//RSAES_OAEP_SHA_Encryptor pub(pri);

		CryptoPP::RSAES_OAEP_SHA_Decryptor pri(privateKey);
		CryptoPP::RSAES_OAEP_SHA_Encryptor pub(publicKey);
		LOG4_TRACE("max plaintext Length = %d,%d", pri.FixedMaxPlaintextLength(), pub.FixedMaxPlaintextLength());
		if (pub.FixedMaxPlaintextLength() > message.length())
		{//待加密文本不能大于最大加密长度
			std::string chilper;
			CryptoPP::StringSource(message, true, new CryptoPP::PK_EncryptorFilter(rnd, pub, new CryptoPP::StringSink(chilper)));
			LOG4_TRACE("PK_EncryptorFilter = %s, length = %d", chilper.c_str(), strlen(chilper.c_str()));

			std::string txt;
			CryptoPP::StringSource(chilper, true, new CryptoPP::PK_DecryptorFilter(rnd, pri, new CryptoPP::StringSink(txt)));
			LOG4_TRACE("PK_DecryptorFilter = %s, length = %d", txt.c_str(), strlen(txt.c_str()));
		}
	}

	{
		LOG4_TRACE("使用PKCS1v15模式");
		//使用PKCS1v15模式
		//RSAES_PKCS1v15_Decryptor pri1(randPool, sizeof(seed));
		//RSAES_PKCS1v15_Encryptor pub1(pri1);
		CryptoPP::RSAES_PKCS1v15_Decryptor pri1(privateKey);
		CryptoPP::RSAES_PKCS1v15_Encryptor pub1(publicKey);
		LOG4_TRACE("max plaintext Length = %d,%d", pri1.FixedMaxPlaintextLength(), pub1.FixedMaxPlaintextLength());
		if (pub1.FixedMaxPlaintextLength() > message.length())
		{//待加密文本不能大于最大加密长度
			std::string chilper;
			CryptoPP::StringSource(message, true, new CryptoPP::PK_EncryptorFilter(rnd, pub1, new CryptoPP::StringSink(chilper)));
			LOG4_TRACE("PK_EncryptorFilter = %s, length = %d", chilper.c_str(), strlen(chilper.c_str()));

			std::string txt;
			CryptoPP::StringSource(chilper, true, new CryptoPP::PK_DecryptorFilter(rnd, pri1, new CryptoPP::StringSink(txt)));
			LOG4_TRACE("PK_DecryptorFilter = %s, length = %d", txt.c_str(), strlen(txt.c_str()));
		}
	}
}
int g_TestCoroutinueTimes = 100000;
void ModuleHello::TestCoroutinue()//用于分隔逻辑
{
	LOG4_TRACE("TestCoroutinue");
	struct Param:public net::tagCoroutineArg {Param(int a1):m_start1(a1){} int m_start1;};
	auto testcoroutinue = []  (void *ud) {
		Param *arg = (Param*)ud;
		int i=0;
		for (;i<g_TestCoroutinueTimes;i++)
		{
			LOG4_TRACE("TestCoroutinue running id(%d),arg n(%d) tid(%u)",net::CoroutineRunning() , arg->m_start1 + i,pthread_self());
			net::CoroutineYield();
		}
		if (i == g_TestCoroutinueTimes)
		LOG4_INFO("TestCoroutinue running id(%d),arg n(%d) tid(%u)",net::CoroutineRunning() , arg->m_start1 + i,pthread_self());
	};
	//两个协程任务，在两个任务之间切换
	net::CoroutineNewWithArg(testcoroutinue,new Param(0));
	net::CoroutineNewWithArg(testcoroutinue,new Param(100));

	LOG4_INFO("%s Coroutine start! tid(%u)",__FUNCTION__,pthread_self());
	m_RunClock.StartClock();
	net::CoroutineResumeWithTimes(g_TestCoroutinueTimes*2);
	m_RunClock.EndClock();
	//20w use time(305.894989) qps 66w
	LOG4_INFO("%s Coroutine end!tid(%u) use time(%lf)",__FUNCTION__,pthread_self(),m_RunClock.LastUseTime());
}

void ModuleHello::TestCoroutinueAuto()
{
	LOG4_TRACE("TestCoroutinue");
	struct Param:public net::tagCoroutineArg {Param(int a1):m_start1(a1){} int m_start1;};
	auto testcoroutinue = []  (void *ud) {
		Param *arg = (Param*)ud;
		int i=0;
		for (;i<g_TestCoroutinueTimes;i++)
		{
			LOG4_TRACE("TestCoroutinueAuto running id(%d),arg n(%d) tid(%u)",net::CoroutineRunning() , arg->m_start1 + i,pthread_self());
			net::CoroutineYield();
		}
		if (i == g_TestCoroutinueTimes)
		LOG4_INFO("TestCoroutinueAuto running id(%d),arg n(%d) tid(%u)",net::CoroutineRunning() , arg->m_start1 + i,pthread_self());
	};
	//两个协程任务，在两个任务之间切换
	net::CoroutineNewWithArg(testcoroutinue,new Param(200));
	net::CoroutineNewWithArg(testcoroutinue,new Param(300));

	LOG4_INFO("%s Coroutine start! tid(%u)",__FUNCTION__,pthread_self());
	m_RunClock.StartClock();
	net::CoroutineResumeWithTimes();
	m_RunClock.EndClock();
	//20w use time(305.053986) qps 66w
	LOG4_INFO("%s Coroutine end!tid(%u) use time(%lf)",__FUNCTION__,pthread_self(),m_RunClock.LastUseTime());
}

void ModuleHello::TestStepCoFuncDataProxy(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &str)
{
    struct StateParam:public net::StepParam
    {
        StateParam(const std::string &s,const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg):
            str(s),shell(stMsgShell),msg(oInHttpMsg){}
        std::string str;
        net::tagMsgShell shell;
        HttpMsg msg;
    };
    auto stateFunc0= [] (net::StepCo* state)
    {
        {
            auto SetValueFromRedis_callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
            {
                LOG4_TRACE("SetValueFromRedis_callback %s",oRsp.DebugString().c_str());
            };
            char sRedisKey[64];
            snprintf(sRedisKey,sizeof(sRedisKey),"1:2:testStepCo");
            net::RedisOperator oRedisOperator(0, sRedisKey,"SET");
            oRedisOperator.AddRedisField("",((StateParam*)state->GetData())->str);
            LOG4_TRACE("%s() stateFunc0 %s",__FUNCTION__,((StateParam*)state->GetData())->str.c_str());
            if (!net::SendToCallback(state,oRedisOperator.MakeMemOperate(),SetValueFromRedis_callback,"PROXYSSDB"))return;
        }
        state->CoroutineYield();//放弃执行，记录状态
        {
            auto GetValueFromRedis_callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
            {
                LOG4_TRACE("GetValueFromRedis_callback %s",oRsp.DebugString().c_str());
            };
            char sRedisKey[64];
            snprintf(sRedisKey,sizeof(sRedisKey),"1:2:testStepCo");
            net::RedisOperator oRedisOperator(0, sRedisKey,"","GET");
            LOG4_TRACE("%s() stateFunc0 %s",__FUNCTION__,((StateParam*)state->GetData())->str.c_str());
            if (!net::SendToCallback(state,oRedisOperator.MakeMemOperate(),GetValueFromRedis_callback,"PROXYSSDB"))return;
        }
    };
    net::StepCo* pstep = new net::StepCo(stMsgShell,oInHttpMsg);
    pstep->AddCoroutinueFunc(stateFunc0);
    pstep->SetSuccFunc([] (net::StepCo* state)
                    {
                        LOG4_TRACE("stateFuncOnSucc");
                        util::CJsonObject oRsp;
                        oRsp.Add("code", 0);
                        oRsp.Add("msg", "succ");
                        state->SendToClient(oRsp.ToString());
                    });
    pstep->SetData(new StateParam(str,stMsgShell,oInHttpMsg));
    net::Launch(pstep);
}

void ModuleHello::Response(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,int iCode)
{
    util::CJsonObject oRsp;
    oRsp.Add("code", iCode);
    oRsp.Add("msg", "ok");
    net::SendToClient(stMsgShell,oInHttpMsg,oRsp.ToString());
}

bool ModuleHello::TestHttpRequestState(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
     // HttpState
    return net::Launch(new StepHttpRequestState(stMsgShell,oInHttpMsg));
}

bool ModuleHello::TestHttpRequestStateFunc(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
	struct StateParam:public net::StepParam
	{
		StateParam():val(3){ for (uint32 i = 1;i <=val;++i)m.insert(std::make_pair(i,i));}
	    uint32 val;
	    std::map<int,int> m;
	    uint32 Inc(){++val;m.insert(std::make_pair(val,val));return val;}
	};

	auto stateFunc0= [] (net::StepState* state)
	{
	    LOG4_DEBUG("%s GetLastState:%u GetCurrentState(%u) val(%u) m(%u)",
	                            __FUNCTION__,state->GetLastState(),state->GetCurrentState(),
	                        ((StateParam*)state->GetData())->Inc(),((StateParam*)state->GetData())->m.size());
	    return state->HttpGet("http://www.baidu.com/");//return bool
	};

	auto stateFunc1= [] (net::StepState* state)
	{
	    LOG4_DEBUG("%s GetLastState:%u GetCurrentState(%u) val(%u) m(%u)",
	                            __FUNCTION__,state->GetLastState(),state->GetCurrentState(),
	                        ((StateParam*)state->GetData())->Inc(),((StateParam*)state->GetData())->m.size());
	    return state->HttpGet("http://www.baidu.com/") && state->SetNextState(3);
	};

	auto stateFunc2= [] (net::StepState* state)
	{
	    LOG4_DEBUG("%s GetLastState:%u GetCurrentState(%u) val(%u) m(%u)",
	                            __FUNCTION__,state->GetLastState(),state->GetCurrentState(),
	                        ((StateParam*)state->GetData())->Inc(),((StateParam*)state->GetData())->m.size());
	    return state->HttpGet("http://www.baidu.com/");
	};
	auto stateFunc3 = [] (net::StepState* state)
	{
	    LOG4_DEBUG("%s GetLastState:%u GetCurrentState(%u) val(%u) m(%u)",
	                            __FUNCTION__,state->GetLastState(),state->GetCurrentState(),
	                        ((StateParam*)state->GetData())->Inc(),((StateParam*)state->GetData())->m.size());
	    return state->HttpGet("http://www.baidu.com/");
	};
	auto  stateFuncOnSucc = [] (net::StepState* state)
	{
	    LOG4_DEBUG("%s GetLastState:%u GetCurrentState(%u) val(%u) m(%u)",
	                            __FUNCTION__,state->GetLastState(),state->GetCurrentState(),
	                        ((StateParam*)state->GetData())->Inc(),((StateParam*)state->GetData())->m.size());
	    util::CJsonObject oRsp;
	    oRsp.Add("code", 0);
	    oRsp.Add("msg", "ok");
	    state->SendToClient(oRsp.ToString());
	};
	auto stateFuncOnFail = [] (net::StepState* state)
	{
	    LOG4_DEBUG("%s GetLastState:%u GetCurrentState(%u) val(%u) m(%u)",
	                        __FUNCTION__,state->GetLastState(),state->GetCurrentState(),
	                    ((StateParam*)state->GetData())->Inc(),((StateParam*)state->GetData())->m.size());
	    util::CJsonObject oRsp;
	    oRsp.Add("code", 1);
	    oRsp.Add("msg", "fail");
	    state->SendToClient(oRsp.ToString());
	};
    net::StepState* pstep = new net::StepState(stMsgShell,oInHttpMsg);
    pstep->AddStateFunc(stateFunc0);
    pstep->AddStateFunc(stateFunc1);
    pstep->AddStateFunc(stateFunc2);
    pstep->AddStateFunc(stateFunc3);
    pstep->SetSuccFunc(stateFuncOnSucc);
    pstep->SetFailFunc(stateFuncOnFail);
    pstep->SetData(new StateParam());
    return net::Launch(pstep);
}

bool ModuleHello::TestHttpRequestStateFuncDataProxy(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &str)
{
	struct StateParam:public net::StepParam
	{
		StateParam(const std::string &s,const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg):
			str(s),shell(stMsgShell),msg(oInHttpMsg){}
		std::string str;
		net::tagMsgShell shell;
		HttpMsg msg;
	};
	auto stateFunc0= [] (net::StepState* state)
	{
		auto SetValueFromRedis_callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
		{
			LOG4_TRACE("SetValueFromRedis_callback %d:%s",oRsp.err_no(),oRsp.err_msg().c_str());
		};
		char sRedisKey[64];
		snprintf(sRedisKey,sizeof(sRedisKey),TEST_SSDB_KEY);
		net::RedisOperator oRedisOperator(0, sRedisKey,"SET");
		oRedisOperator.AddRedisField("",((StateParam*)state->GetData())->str);
		LOG4_TRACE("%s() stateFunc0 %s",__FUNCTION__,((StateParam*)state->GetData())->str.c_str());
		return net::SendToCallback(state,oRedisOperator.MakeMemOperate(),SetValueFromRedis_callback);
	};
	auto stateFunc1= [] (net::StepState* state)
	{
		auto GetValueFromRedis_callback = [] (const DataMem::MemRsp &oRsp,net::Step* pStep)
		{
			LOG4_TRACE("GetValueFromRedis_callback %d:%s",oRsp.err_no(),oRsp.err_msg().c_str());
		};
		char sRedisKey[64];
		snprintf(sRedisKey,sizeof(sRedisKey),TEST_SSDB_KEY);
		net::RedisOperator oRedisOperator(0, sRedisKey,"","GET");
		LOG4_TRACE("%s() stateFunc1 %s",__FUNCTION__,((StateParam*)state->GetData())->str.c_str());
		return net::SendToCallback(state,oRedisOperator.MakeMemOperate(),GetValueFromRedis_callback);
	};
	auto stateFuncOnSucc= [] (net::StepState* state)
	{
		LOG4_TRACE("stateFuncOnSucc");
		util::CJsonObject oRsp;
		oRsp.Add("code", 0);
		oRsp.Add("msg", "succ");
		state->SendToClient(oRsp.ToString());
	};
	auto stateFuncOnFail = [] (net::StepState* state)
	{
		LOG4_TRACE("stateFuncOnFail");
		util::CJsonObject oRsp;
		oRsp.Add("code", 1);
		oRsp.Add("msg", "fail");
		state->SendToClient(oRsp.ToString());
	};
	net::StepState* pstep = new net::StepState(stMsgShell,oInHttpMsg);
	pstep->AddStateFunc(stateFunc0);
	pstep->AddStateFunc(stateFunc1);
	pstep->SetSuccFunc(stateFuncOnSucc);
	pstep->SetFailFunc(stateFuncOnFail);
	pstep->SetData(new StateParam(str,stMsgShell,oInHttpMsg));
	return net::Launch(pstep);
}

void ModuleHello::Base64Encode(const char* data,unsigned int datalen,std::string &strEncode)
{
    int encodedMaxlen = Base64encode_len(datalen);
    char *encoded = new char [encodedMaxlen];
    int encodedlen = Base64encode(encoded,data,datalen);
    strEncode.assign(encoded);//编码后的encoded是普通字符串(末尾增加0)
    delete [] encoded;
    LOG4_DEBUG("Base64Encode data(%s,%d(bin length)),encoded(%s,%d(include added end 0),%d),"
                        "strEncode len(%u)",
                        data,datalen,
                        encoded,encodedlen,encodedMaxlen,
                        strEncode.length());//encodedlen长度包括添加的末尾字节0
}

void ModuleHello::Base64Decode(const char* encoded,unsigned int encodedlen,std::string &strDecode)
{
    int decodedMaxlen = Base64decode_len(encoded);
    char *decoded = new char [decodedMaxlen];
    int decodedlen = Base64decode(decoded,encoded);//add 0 to the end
    strDecode.assign(decoded,decodedlen);//decodedlen长度不包括Base64decode添加的末尾字节0
    delete [] decoded;
    LOG4_DEBUG("Base64Decode encoded(%s,%d(not include added end 0)),decode(%s,%d(not include added end 0),%d),"
                    "strDecode len(%u)",
                    encoded,encodedlen,
                    decoded,decodedlen,decodedMaxlen,
                    strDecode.length());
}

void ModuleHello::PrintBin(const char* data,unsigned int datalen)
{
    char *tmp = new char [datalen+1];
    memcpy(tmp,data,datalen);
    for(unsigned int i = 0;i < datalen;++i)
    {
        if ('\0' == tmp[i])
        {
            tmp[i] = '0';
        }
    }
    tmp[datalen] = 0;
    LOG4_DEBUG("PrintBin data:%s",tmp);
    delete [] tmp;
}

bool ModuleHello::TestJson2pb(const net::tagMsgShell& stMsgShell,
                const HttpMsg& oInHttpMsg)
{
    /*
    http://192.168.18.68:16037/robot/web/hello
    {
        "session_id": 1
    }
    */
    {//parse json to pb msg
        /*
            message MsgBody
            {
                bytes body         = 1;         ///< 消息体主体
                uint64 session_id  = 2;         ///< 会话ID（没有设置的服务器会设置为发送者的appid + userid,需要修改的则为客户端主动填写）
                string session     = 3;         ///< 会话ID（当session_id用整型无法表达时使用）
                bytes additional   = 4;         ///< 接入层附加的数据（客户端无须理会）
            }
         * */
        util::CJsonObject obj;
        if(!obj.Parse(oInHttpMsg.body()))
        {
            LOG4_WARN("it is not json:%s",oInHttpMsg.body().c_str());
            Response(stMsgShell,oInHttpMsg,ERR_PARASE_PROTOBUF);
            return false;
        }
        {//增加body字段，bytes类型,需要先进行Base64编码
            util::CJsonObject bodyJson;
            bodyJson.Add("test_1",1);
            bodyJson.Add("test_2","2");
            bodyJson.Add("test_3",3.0);
            bodyJson.Add("chinese","你好");
            {//二进制
                char cs[] = {'a','b',0x0,'c','d','e',0x0,0x0,0x0,0x0};
                std::string encoded;
                Base64Encode(cs,sizeof(cs),encoded);
                bodyJson.Add("bin",encoded);
                std::string getBintmp;
                bodyJson.Get("bin",getBintmp);
                std::string decoded;
                Base64Decode(getBintmp.c_str(),getBintmp.length(),decoded);
                LOG4_DEBUG("check encoded(%s,%d(not include added end 0)),"
                                "getencoded(%s,%d(not include added end 0)),decoded(%s,%d)",
                                encoded.c_str(),encoded.length(),
                                getBintmp.c_str(),getBintmp.length(),
                                decoded.c_str(),decoded.length());
                /*
                 check encoded(YWIAY2RlAAAAAA==,16(not include added end 0)),getencoded(YWIAY2RlAAAAAA==,16(not
                 include added end 0)),decoded(ab,10)
                 * */
                PrintBin(decoded.c_str(),decoded.length());
            }
            std::string strbody = bodyJson.ToString();
            std::string encoded;
            Base64Encode(strbody.c_str(),strbody.length(),encoded);
            LOG4_DEBUG("strbody(%s,%d),encoded(%s,%d)",
                            strbody.c_str(),strbody.length(),
                            encoded.c_str(),encoded.length());
            /*
             strbody({"test1":1,"test2":"2","test3":3,"chinese":"ä½ å¥½","bin":"YWIAY2RlAAAAAA=="},77),
             encoded(eyJ0ZXN0MSI6MSwidGVzdDIiOiIyIiwidGVzdDMiOjMsImNoaW5lc2UiOiLkvaDlpb0iLCJiaW4iOiJZV0lBWTJSbEFBQUFBQT09In0=,104)
             * */
            obj.Add("body",encoded);//bytes类型的字段需要先进行Base64编码
        }
        {//增加session
            util::CJsonObject additionalJson;
            additionalJson.Add("additional_1",1);
            additionalJson.Add("additional_2","2");
            additionalJson.Add("additional_3",3.0);
            additionalJson.Add("chinese","你好");
            {//二进制
                char cs[] = {'a','b',0x0,'c','d','e',0x0,0x0,0x0,0x0};
                std::string encoded;
                Base64Encode(cs,sizeof(cs),encoded);
                additionalJson.Add("bin",encoded);
                std::string getBintmp;
                additionalJson.Get("bin",getBintmp);
                std::string decoded;
                Base64Decode(getBintmp.c_str(),getBintmp.length(),decoded);
                LOG4_DEBUG("check encoded(%s,%d(not include added end 0)),"
                                "getencoded(%s,%d(not include added end 0)),decoded(%s,%d)",
                                encoded.c_str(),encoded.length(),
                                getBintmp.c_str(),getBintmp.length(),
                                decoded.c_str(),decoded.length());
                /*
                 check encoded(YWIAY2RlAAAAAA==,16(not include added end 0)),
                 getencoded(YWIAY2RlAAAAAA==,16(not include added end 0)),decoded(ab,10)
                 */
                PrintBin(decoded.c_str(),decoded.length());
                obj.Add("session",additionalJson.ToString());
            }
        }
        LOG4_DEBUG("json data:%s",obj.ToString().c_str());
        /*
         json data:
         {
         "session_id":1,
         "body":"eyJ0ZXN0MSI6MSwidGVzdDIiOiIyIiwidGVzdDMiOjMsImNoaW5lc2UiOiLkvaDlpb0iLCJiaW4iOiJZV0lBWTJSbEFBQUFBQT09In0=",
         "session":"{\"additional1\":1,\"additional2\":\"2\",\"additional3\":3,\"chinese\":\"你好\",\"bin\":\"YWIAY2RlAAAAAA==\"}"
         }
        */
        MsgBody oMsgBody;
        {//json to msg
            google::protobuf::util::JsonParseOptions oParseOptions;
            google::protobuf::util::Status oStatus;
            oStatus = google::protobuf::util::JsonStringToMessage(obj.ToString(), &oMsgBody, oParseOptions);
            if(!oStatus.ok())
            {
                LOG4_WARN("failed to JsonStringToMessage(),error_code(%d),error_message(%s):%s",
                                oStatus.error_code(),oStatus.error_message().data(),oInHttpMsg.body().c_str());
                Response(stMsgShell,oInHttpMsg,ERR_PARASE_PROTOBUF);
                return false;
            }
            LOG4_DEBUG("JsonStringToMessage oMsgBody():%s",oMsgBody.DebugString().c_str());
            /*
             * 使用JsonStringToMessage会转码base64编码的body为明文
                JsonStringToMessage oMsgBody():
                body:
                "{\"test1\":1,\"test2\":\"2\",\"test3\":3,\"chinese\":\"\344\275\240\345\245\275\",\"bin\":\"YWIAY2RlAAAAAA==\"}"
                session_id: 1
                session:
                "{\"additional1\":1,\"additional2\":\"2\",\"additional3\":3,\"chinese\":\"\344\275\240\345\245\275\",\"bin\":\"YWIAY2RlAAAAAA==\"}"
             */
        }
        {//msg to json
            google::protobuf::util::JsonOptions oOptions;
            oOptions.add_whitespace = true;
            google::protobuf::util::Status oStatus;
            std::string strJson;
            oStatus = google::protobuf::util::MessageToJsonString(oMsgBody,&strJson,oOptions);
            if(!oStatus.ok())
            {
                LOG4_WARN("failed to JsonStringToMessage(),error_code(%d),error_message(%s):%s",
                                oStatus.error_code(),oStatus.error_message().data(),oInHttpMsg.body().c_str());
                Response(stMsgShell,oInHttpMsg,ERR_PARASE_PROTOBUF);
                return false;
            }
            LOG4_DEBUG("MessageToJsonString strJson():%s",strJson.c_str());
            /*
                             转换oMsgBody为json后，body字段为原来base64编码后的数据（json对象的字符串编码为BYTES的字段）
             MessageToJsonString strJson():
             {
             "body":"eyJ0ZXN0MSI6MSwidGVzdDIiOiIyIiwidGVzdDMiOjMsImNoaW5lc2UiOiLkvaDlpb0iLCJiaW4iOiJZV0lBWTJSbEFBQUFBQT09In0=",
             "sessionId":"1",
             "session":"{\"additional1\":1,\"additional2\":\"2\",\"additional3\":3,\"chinese\":\"ä½ å¥½\",\"bin\":\"YWIAY2RlAAAAAA==\"}"}
             */
            util::CJsonObject jsonObj;
            if(!jsonObj.Parse(strJson))
            {
                LOG4_WARN("failed to parse strJson(%s)",strJson.c_str());
                Response(stMsgShell,oInHttpMsg,ERR_PARASE_PROTOBUF);
                return false;
            }
            LOG4_DEBUG("ok to parse strJson(%s)",strJson.c_str());
            std::string strBody;
            if(jsonObj.Get("body",strBody))
            {
                std::string decoded;
                LOG4_DEBUG("parse strJson body(%u,%s)",strBody.length(),strBody.c_str());
                Base64Decode(strBody.c_str(),strBody.length(),decoded);
                LOG4_DEBUG("ok to parse strJson body:%s",decoded.c_str());
            }
        }
    }
    return true;
}

void ModuleHello::TestProto3Type()
{
    //https://developers.google.com/protocol-buffers/docs/reference/cpp-generated
    /*
     .proto类型   Java 类型     C++类型   备注
    double      double      double
    float       float       float
    int32       int         int32       使用可变长编码方式。编码负数时不够高效——如果你的字段可能含有负数，那么请使用sint32。
    int64       long        int64       使用可变长编码方式。编码负数时不够高效——如果你的字段可能含有负数，那么请使用sint64。
    uint32      int[1]      uint32      Uses variable-length encoding.
    uint64      long[1]     uint64      Uses variable-length encoding.
    sint32      int         int32       使用可变长编码方式。有符号的整型值。编码时比通常的int32高效。
    sint64      long        int64       使用可变长编码方式。有符号的整型值。编码时比通常的int64高效。
    fixed32     int[1]      uint32      总是4个字节。如果数值总是比总是比228大的话，这个类型会比uint32高效。
    fixed64     long[1]     uint64      总是8个字节。如果数值总是比总是比256大的话，这个类型会比uint64高效。
    sfixed32    int         int32       总是4个字节。
    sfixed64    long        int64       总是8个字节。
    bool        boolean     bool
    string      String      string      一个字符串必须是UTF-8编码或者7-bit ASCII编码的文本。
    bytes       ByteString  string      可能包含任意顺序的字节数据。
     * */
    /*
            消息描述中的一个元素可以被标记为“可选的”（optional）。一个格式良好的消息可以包含0个或一个optional的元素。当解 析消息时，如果它不包含optional的元素值，那么解析出来的对象中的对应字段就被置为默认值。默认值可以在消息描述文件中指定。例如，要为 SearchRequest消息的result_per_page字段指定默认值10，在定义消息格式时如下所示：
      optional int32 result_per_page = 3 [default = 10];
            如果没有为optional的元素指定默认值，就会使用与特定类型相关的默认值：对string来说，默认值是空字符串。对bool来说，默认值是false。对数值类型来说，默认值是0。对枚举来说，默认值是枚举类型定义中的第一个值。
     * */
    {
        /*
         class Any {
             public:
              // Packs the given message into this Any using the default type URL
              // prefix “type.googleapis.com”.
              void PackFrom(const google::protobuf::Message& message);

              // Packs the given message into this Any using the given type URL
              // prefix.
              void PackFrom(const google::protobuf::Message& message,
                            const string& type_url_prefix);

              // Unpacks this Any to a Message. Returns false if this Any
              // represents a different protobuf type or parsing fails.
              bool UnpackTo(google::protobuf::Message* message) const;

              // Returns true if this Any represents the given protobuf type.
              template<typename T> bool Is() const;
            }
        message Any {
          // A URL/resource name whose content describes the type of the
          // serialized protocol buffer message.
          //
          // For URLs which use the scheme `http`, `https`, or no scheme, the
          // following restrictions and interpretations apply:
          //
          // * If no scheme is provided, `https` is assumed.
          // * The last segment of the URL's path must represent the fully
          //   qualified name of the type (as in `path/google.protobuf.Duration`).
          //   The name should be in a canonical form (e.g., leading "." is
          //   not accepted).
          // * An HTTP GET on the URL must yield a [google.protobuf.Type][]
          //   value in binary format, or produce an error.
          // * Applications are allowed to cache lookup results based on the
          //   URL, or have them precompiled into a binary to avoid any
          //   lookup. Therefore, binary compatibility needs to be preserved
          //   on changes to types. (Use versioned type names to manage
          //   breaking changes.)
          //
          // Schemes other than `http`, `https` (or the empty scheme) might be
          // used with implementation specific semantics.
          //
          string type_url = 1;

          // Must be a valid serialized protocol buffer of the above specified type.
          bytes value = 2;
        }
         * */
         MsgBody foo;
         MsgBody foo1;
         foo.set_session_id(100);
         foo.set_session("123");
         google::protobuf::Any any;
         any.PackFrom(foo);
         LOG4_DEBUG("any :%s",any.DebugString().c_str());
         /*
          any :[type.googleapis.com/MsgBody] {
              session_id: 100
              session: "123"
            }
          * */
         if (any.UnpackTo(&foo1))
         {
             LOG4_DEBUG("foo1 :%s",foo1.DebugString().c_str());
             /*
              foo1 :session_id: 100
              session: "123"
              * */
         }
    }
    {
        /*
         Map Fields
            For this map field definition:
            map<int32, int32> weight = 1;

            The compiler will generate the following accessor methods:
            const google::protobuf::Map<int32, int32>& weight();: Returns an immutable Map.
            google::protobuf::Map<int32, int32>* mutable_weight();: Returns a mutable Map.
            A google::protobuf::Map is a special container type used in protocol buffers to store map fields. As you can see from its interface below, it uses a commonly-used subset of std::map and std::unordered_map methods.
            template<typename Key, typename T> {
            class Map {
              // Member types
              typedef Key key_type;
              typedef T mapped_type;
              typedef ... value_type;
              // Iterators
              iterator begin();
              const_iterator begin() const;
              const_iterator cbegin() const;
              iterator end();
              const_iterator end() const;
              const_iterator cend() const;
              // Capacity
              int size() const;
              bool empty() const;
              // Element access
              T& operator[](const Key& key);
              const T& at(const Key& key) const;
              T& at(const Key& key);
              // Lookup
              int count(const Key& key) const;
              const_iterator find(const Key& key) const;
              iterator find(const Key& key);
              // Modifiers
              pair<iterator, bool> insert(const value_type& value);
              template<class InputIt>
              void insert(InputIt first, InputIt last);
              size_type erase(const Key& Key);
              iterator erase(const_iterator pos);
              iterator erase(const_iterator first, const_iterator last);
              void clear();
              // Copy
              Map(const Map& other);
              Map& operator=(const Map& other);
            }
            The easiest way to add data is to use normal map syntax, for example:
            std::unique_ptr<ProtoName> my_enclosing_proto(new ProtoName);
            (*my_enclosing_proto->mutable_weight())[my_key] = my_value;
            pair<iterator, bool> insert(const value_type& value) will implicitly cause a deep copy of the value_type instance. The most efficient way to insert a new value into a google::protobuf::Map is as follows:
            T& operator[](const Key& key): map[new_key] = new_mapped;
            Using google::protobuf::Map with standard maps
            google::protobuf::Map supports the same iterator API as std::map and std::unordered_map. If you don't want to use google::protobuf::Map directly, you can convert a google::protobuf::Map to a standard map by doing the following:
            std::map<int32, int32> standard_map(message.weight().begin(),
                                                message.weight().end());
            Note that this will make a deep copy of the entire map.
            You can also construct a google::protobuf::Map from a standard map as follows:
            google::protobuf::Map<int32, int32> weight(standard_map.begin(), standard_map.end());
            Parsing unknown values
            On the wire, a .proto map is equivalent to a map entry message for each key/value pair, while the map itself is a repeated field of map entries. Like ordinary message types, it's possible for a parsed map entry message to have unknown fields: for example a field of type int64 in a map defined as map<int32, string>.
            If there are unknown fields in the wire format of a map entry message, they will be discarded.
            If there is an unknown enum value in the wire format of a map entry message, it's handled differently in proto2 and proto3. In proto2, the whole map entry message is put into the unknown field set of the containing message. In proto3, it is put into a map field as if it is a known enum value.
         * */
        std::map<int,int> standard_map;
        standard_map.insert(std::make_pair(100,200));
        google::protobuf::Map<int32, int32> weight(standard_map.begin(), standard_map.end());\
    }

    {
        /*
         Enumerations
            Given an enum definition like:
            enum Foo {
              VALUE_A = 0;
              VALUE_B = 5;
              VALUE_C = 1234;
            }
            The protocol buffer compiler will generate a C++ enum type called Foo with the same set of values. In addition, the compiler will generate the following functions:
            const EnumDescriptor* Foo_descriptor(): Returns the type's descriptor, which contains information about what values this enum type defines.
            bool Foo_IsValid(int value): Returns true if the given numeric value matches one of Foo's defined values. In the above example, it would return true if the input were 0, 5, or 1234.
            const string& Foo_Name(int value): Returns the name for given numeric value. Returns an empty string if no such value exists. If multiple values have this number, the first one defined is returned. In the above example, Foo_Name(5) would return "VALUE_B".
            bool Foo_Parse(const string& name, Foo* value): If name is a valid value name for this enum, assigns that value into value and returns true. Otherwise returns false. In the above example, Foo_Parse("VALUE_C", &someFoo) would return true and set someFoo to 1234.
            const Foo Foo_MIN: the smallest valid value of the enum (VALUE_A in the example).
            const Foo Foo_MAX: the largest valid value of the enum (VALUE_C in the example).
            const int Foo_ARRAYSIZE: always defined as Foo_MAX + 1.
            Be careful when casting integers to proto2 enums. If an integer is cast to a proto2 enum value, the integer must be one of the valid values for than enum, or the results may be undefined. If in doubt, use the generated Foo_IsValid() function to test if the cast is valid. Setting an enum-typed field of a proto2 message to an invalid value may cause an assertion failure. If an invalid enum value is read when parsing a proto2 message, it will be treated as an unknown field. These semantics have been changed in proto3. It's safe to cast any integer to a proto3 enum value as long as it fits into int32. Invalid enum values will also be kept when parsing a proto3 message and returned by enum field accessors.
            You can define an enum inside a message type. In this case, the protocol buffer compiler generates code that makes it appear that the enum type itself was declared nested inside the message's class. The Foo_descriptor() and Foo_IsValid() functions are declared as static methods. In reality, the enum type itself and its values are declared at the global scope with mangled names, and are imported into the class's scope with a typedef and a series of constant definitions. This is done only to get around problems with declaration ordering. Do not depend on the mangled top-level names; pretend the enum really is nested in the message class.
         * */
    }
    {
        /*
                     使用Oneof
                    为了在.proto定义Oneof字段， 你需要在名字前面加上oneof关键字, 比如下面例子的test_oneof:
                message SampleMessage {
                  oneof test_oneof {
                     string name = 4;
                     SubMessage sub_message = 9;
                  }
                }
                        然后你可以增加oneof字段到 oneof 定义中. 你可以增加任意类型的字段, 但是不能使用 required, optional, repeated 关键字.
                        在产生的代码中, oneof字段拥有同样的 getters 和setters， 就像正常的可选字段一样. 也有一个特殊的方法来检查到底那个字段被设置. 你可以在相应的语言API中找到oneof API介绍.
        Oneof 特性:
                    设置oneof会自动清楚其它oneof字段的值. 所以设置多次后，只有最后一次设置的字段有值.
            SampleMessage message;
            message.set_name(“name”);
            CHECK(message.has_name());
            message.mutable_sub_message();   // Will clear name field.
            CHECK(!message.has_name());

            If the parser encounters multiple members of the same oneof on the wire, only the last member seen is used in the parsed message.
            oneof不支持扩展.
            oneof不能 repeated.
                        反射API对oneof 字段有效.
                        如果使用C++,需确保代码不会导致内存泄漏. 下面的代码会崩溃， 因为sub_message 已经通过set_name()删除了.

            SampleMessage message;
            SubMessage* sub_message = message.mutable_sub_message();
            message.set_name(“name”);      // Will delete sub_message
            sub_message.set_…              // Crashes here
            Again in C++, if you Swap() two messages with oneofs, each message will end up with the other’s oneof case: in the example below, msg1 will have a sub_message and msg2 will have a name.

            SampleMessage msg1;
            msg1.set_name(“name”);
            SampleMessage msg2;
            msg2.mutable_sub_message();
            msg1.swap(&msg2);
            CHECK(msg1.has_sub_message());
            CHECK(msg2.has_name());
         * */
    }
    {
        /*
         Arena Allocation
            Arena allocation is a C++-only feature that helps you optimize your memory usage and improve performance when working with protocol buffers. Enabling arena allocation in your .proto adds additional code for working with arenas to your C++ generated code. You can find out more about the arena allocation API in the Arena Allocation Guide.
         * */
    }
    {
        /*
         Interface
            Given a service definition:
            service Foo {
              rpc Bar(FooRequest) returns(FooResponse);
            }
            The protocol buffer compiler will generate a class Foo to represent this service. Foo will have a virtual method for each method defined in the service definition. In this case, the method Bar is defined as:
            virtual void Bar(RpcController* controller, const FooRequest* request,
                             FooResponse* response, Closure* done);
            The parameters are equivalent to the parameters of Service::CallMethod(), except that the method argument is implied and request and response specify their exact type.
            These generated methods are virtual, but not pure-virtual. The default implementations simply call controller->SetFailed() with an error message indicating that the method is unimplemented, then invoke the done callback. When implementing your own service, you must subclass this generated service and implement its methods as appropriate.
            Foo subclasses the Service interface. The protocol buffer compiler automatically generates implementations of the methods of Service as follows:
            GetDescriptor: Returns the service's ServiceDescriptor.
            CallMethod: Determines which method is being called based on the provided method descriptor and calls it directly, down-casting the request and response messages objects to the correct types.
            GetRequestPrototype and GetResponsePrototype: Returns the default instance of the request or response of the correct type for the given method.
            The following static method is also generated:
            static ServiceDescriptor descriptor(): Returns the type's descriptor, which contains information about what methods this service has and what their input and output types are.
         * */
    }
    {
        /*
         *Stub
            The protocol buffer compiler also generates a "stub" implementation of every service interface, which is used by clients wishing to send requests to servers implementing the service. For the Foo service (above), the stub implementation Foo_Stub will be defined. As with nested message types, a typedef is used so that Foo_Stub can also be referred to as Foo::Stub.
            Foo_Stub is a subclass of Foo which also implements the following methods:
            Foo_Stub(RpcChannel* channel): Constructs a new stub which sends requests on the given channel.
            Foo_Stub(RpcChannel* channel, ChannelOwnership ownership): Constructs a new stub which sends requests on the given channel and possibly owns that channel. If ownership is Service::STUB_OWNS_CHANNEL then when the stub object is deleted it will delete the channel as well.
            RpcChannel* channel(): Returns this stub's channel, as passed to the constructor.
            The stub additionally implements each of the service's methods as a wrapper around the channel. Calling one of the methods simply calls channel->CallMethod().
            The Protocol Buffer library does not include an RPC implementation. However, it includes all of the tools you need to hook up a generated service class to any arbitrary RPC implementation of your choice. You need only provide implementations of RpcChannel and RpcController. See the documentation for service.h for more information.
         * */
    }
}

void ModuleHello::TestJson2pbRepeatedFields()
{//repeated
//      Repeated Numeric Fields
//      For this field definition:
//      repeated int32 foo = 1;
//      The compiler will generate the following accessor methods:
//      int foo_size() const: Returns the number of elements currently in the field.
//      int32 foo(int index) const: Returns the element at the given zero-based index. Calling this method with index outside of [0, foo_size()) yields undefined behavior.
//      void set_foo(int index, int32 value): Sets the value of the element at the given zero-based index.
//      void add_foo(int32 value): Appends a new element to the field with the given value.
//      void clear_foo(): Removes all elements from the field. After calling this, foo_size() will return zero.
//      const RepeatedField<int32>& foo() const: Returns the underlying RepeatedField that stores the field's elements. This container class provides STL-like iterators and other methods.
//      RepeatedField<int32>* mutable_foo(): Returns a pointer to the underlying mutable RepeatedField that stores the field's elements. This container class provides STL-like iterators and other methods.
//      For other numeric field types (including bool), int32 is replaced with the corresponding C++ type according to the scalar value types table.
//
//      Repeated String Fields
//      For either of these field definitions:
//      repeated string foo = 1;
//      repeated bytes foo = 1;
//      The compiler will generate the following accessor methods:
//      int foo_size() const: Returns the number of elements currently in the field.
//      const string& foo(int index) const: Returns the element at the given zero-based index. Calling this method with index outside of [0, foo_size()) yields undefined behavior.
//      void set_foo(int index, const string& value): Sets the value of the element at the given zero-based index.
//      void set_foo(int index, const char* value): Sets the value of the element at the given zero-based index using a C-style null-terminated string.
//      void set_foo(int index, const char* value, int size): Like above, but the string size is given explicitly rather than determined by looking for a null-terminator byte.
//      string* mutable_foo(int index): Returns a pointer to the mutable string object that stores the value of the element at the given zero-based index. Calling this method with index outside of [0, foo_size()) yields undefined behavior.
//      void add_foo(const string& value): Appends a new element to the field with the given value.
//      void add_foo(const char* value): Appends a new element to the field using a C-style null-terminated string.
//      void add_foo(const char* value, int size): Like above, but the string size is given explicitly rather than determined by looking for a null-terminator byte.
//      string* add_foo(): Adds a new empty string element and returns a pointer to it.
//      void clear_foo(): Removes all elements from the field. After calling this, foo_size() will return zero.
//      const RepeatedPtrField<string>& foo() const: Returns the underlying RepeatedPtrField that stores the field's elements. This container class provides STL-like iterators and other methods.
//      RepeatedPtrField<string>* mutable_foo(): Returns a pointer to the underlying mutable RepeatedPtrField that stores the field's elements. This container class provides STL-like iterators and other methods.
//
//      Repeated Enum Fields
//      Given the enum type:
//      enum Bar {
//        BAR_VALUE = 0;
//        OTHER_VALUE = 1;
//      }
//      For this field definition:
//      repeated Bar foo = 1;
//      The compiler will generate the following accessor methods:
//      int foo_size() const: Returns the number of elements currently in the field.
//      Bar foo(int index) const: Returns the element at the given zero-based index. Calling this method with index outside of [0, foo_size()) yields undefined behavior.
//      void set_foo(int index, Bar value): Sets the value of the element at the given zero-based index. In debug mode (i.e. NDEBUG is not defined), if value does not match any of the values defined for Bar, this method will abort the process.
//      void add_foo(Bar value): Appends a new element to the field with the given value. In debug mode (i.e. NDEBUG is not defined), if value does not match any of the values defined for Bar, this method will abort the process.
//      void clear_foo(): Removes all elements from the field. After calling this, foo_size() will return zero.
//      const RepeatedField<int>& foo() const: Returns the underlying RepeatedField that stores the field's elements. This container class provides STL-like iterators and other methods.
//      RepeatedField<int>* mutable_foo(): Returns a pointer to the underlying mutable RepeatedField that stores the field's elements. This container class provides STL-like iterators and other methods.
//
//      Repeated Embedded Message Fields
//      Given the message type:
//      message Bar {}
//      For this field definitions:
//      repeated Bar foo = 1;
//      The compiler will generate the following accessor methods:
//      int foo_size() const: Returns the number of elements currently in the field.
//      const Bar& foo(int index) const: Returns the element at the given zero-based index. Calling this method with index outside of [0, foo_size()) yields undefined behavior.
//      Bar* mutable_foo(int index): Returns a pointer to the mutable Bar object that stores the value of the element at the given zero-based index. Calling this method with index outside of [0, foo_size()) yields undefined behavior.
//      Bar* add_foo(): Adds a new element and returns a pointer to it. The returned Bar will have none of its fields set (i.e. it will be identical to a newly-allocated Bar).
//      void clear_foo(): Removes all elements from the field. After calling this, foo_size() will return zero.
//      const RepeatedPtrField<Bar>& foo() const: Returns the underlying RepeatedPtrField that stores the field's elements. This container class provides STL-like iterators and other methods.
//      RepeatedPtrField<Bar>* mutable_foo(): Returns a pointer to the underlying mutable RepeatedPtrField that stores the field's elements. This container class provides STL-like iterators and other methods.
    LOG4_DEBUG("test_proto3");
    //message test_proto3
    //{
    //    repeated string test_foo1 = 1;
    //    repeated bytes test_foo2 = 2;
    //    repeated int32 test_foo3 = 3;
    //    repeated uint64 test_foo4 = 4;
    //    repeated test_bar test_foo5 = 5;
    //}
    server::test_proto3 testProto;
    //repeated string
    testProto.add_test_foo1("foo1");
    testProto.add_test_foo1("foo11");
    //repeated bytes
    testProto.add_test_foo2("foo2");
    testProto.add_test_foo2("foo22");
    std::string strEncodeTestFoo2;
    Base64Encode("foo2",strlen("foo2"),strEncodeTestFoo2);
    LOG4_DEBUG("strEncodeTestFoo2:%s",strEncodeTestFoo2.c_str());
    //strEncodeTestFoo2:Zm9vMg==
    //repeated int32
    testProto.add_test_foo3(3);
    testProto.add_test_foo3(33);
    //repeated uint64
    testProto.add_test_foo4(4);
    testProto.add_test_foo4(44);
    //repeated test_bar
    //message test_bar
    //{
    //    int32 test_bar1 = 1;
    //    uint64 testBar2 = 2;
    //    string TestBar3 = 3;
    //    bytes Testbar4 = 4;
    //    fixed32 test_Bar5 = 5;
    //    sint64 Test_Bar6 = 6;
    //    double Test_bar7 = 7;
    //}
    server::test_bar* bar = testProto.add_test_foo5();
    bar->set_test_bar1(1);
    bar->set_testbar2(2);
    bar->set_testbar3("3");
    bar->set_testbar4("4");
    bar->set_test_bar5(5);
    bar->set_test_bar6(6);
    bar->set_test_bar7(7.0);
    bar = testProto.add_test_foo5();
    bar->set_test_bar1(11);
    bar->set_testbar2(22);
    bar->set_testbar3("33");
    bar->set_testbar4("44");
    bar->set_test_bar5(55);
    bar->set_test_bar6(66);
    bar->set_test_bar7(7.7);
    std::string strJson;
    {//pb to json
        google::protobuf::util::JsonOptions oOptions;
        google::protobuf::util::Status oStatus;
        oStatus = google::protobuf::util::MessageToJsonString(testProto,&strJson,oOptions);
        if(!oStatus.ok())
        {
            LOG4_WARN("test_proto3 failed to JsonStringToMessage(),error_code(%d),error_message(%s),testProto:%s",
                            oStatus.error_code(),oStatus.error_message().data(),
                            testProto.DebugString().c_str());
            return;
        }
        LOG4_DEBUG("test_proto3 succ to MessageToJsonString strJson:%s",strJson.c_str());
        //test_proto3 succ to MessageToJsonString strJson:{
        // "testFoo1": [
        //  "foo1",
        //  "foo11"
        // ],
        // "testFoo2": [
        //  "Zm9vMg==",
        //  "Zm9vMjI="
        // ],
        // "testFoo3": [
        //  3,
        //  33
        // ],
        // "testFoo4": [
        //  "4",
        //  "44"
        // ],
        // "testFoo5": [
        //  {
        //   "testBar1": 1,
        //   "testBar2": "2",
        //   "testBar3": "3",
        //   "testbar4": "NA==",
        //   "testBar5": 5,
        //   "testBar6": "6",
        //   "testBar7": 7
        //  },
        //  {
        //   "testBar1": 11,
        //   "testBar2": "22",
        //   "testBar3": "33",
        //   "testbar4": "NDQ=",
        //   "testBar5": 55,
        //   "testBar6": "66",
        //   "testBar7": 7.7
        //  }
        // ]
        //}
        util::CJsonObject jsonObj;
        if(!jsonObj.Parse(strJson))
        {
            LOG4_WARN("test_proto3 failed to parse strJson:%s",strJson.c_str());
            return;
        }
        LOG4_DEBUG("ok to parse jsonObj:%s",jsonObj.ToString().c_str());
        //ok to parse jsonObj:{"testFoo1":["foo1","foo11"],"testFoo2":["Zm9vMg==","Zm9vMjI="],
        //"testFoo3":[3,33],"testFoo4":["4","44"],
        //"testFoo5":[{"testBar1":1,"testBar2":"2","testBar3":"3","testbar4":"NA==","testBar5":5,"testBar6":"6","testBar7":7},
        //{"testBar1":11,"testBar2":"22","testBar3":"33","testbar4":"NDQ=","testBar5":55,"testBar6":"66","testBar7":7.700000}]}
        m_RunClock.StartClock("pb to json");
        std::string strTmpJson;
        for(int i = 0;i < 10000;++i)
        {
            strTmpJson.clear();
            google::protobuf::util::JsonOptions oOptions;
            google::protobuf::util::Status oStatus;
            oStatus = google::protobuf::util::MessageToJsonString(testProto,&strTmpJson,oOptions);
            if(!oStatus.ok())
            {
                LOG4_WARN("test_proto3 failed to JsonStringToMessage(),error_code(%d),error_message(%s),testProto:%s",
                                oStatus.error_code(),oStatus.error_message().data(),
                                testProto.DebugString().c_str());
                return;
            }
        }
        m_RunClock.EndClock();
        //EndClock() net::RunClock pb to json use time(251.009003) ms 不加换行和空格
        //EndClock() net::RunClock pb to json use time(1335.982056) ms 加换行和空格
        LOG4_DEBUG("test_proto3 succ to MessageToJsonString 10000 times strJson:%s",strJson.c_str());
        /*
        test_foo1: "foo1"
        test_foo1: "foo11"
        test_foo2: "foo2"
        test_foo2: "foo22"
        test_foo3: 3
        test_foo3: 33
        test_foo4: 4
        test_foo4: 44
        test_foo5 {
          test_bar1: 1
          testBar2: 2
          TestBar3: "3"
          Testbar4: "4"
          test_Bar5: 5
          Test_Bar6: 6
          Test_bar7: 7
        }
        test_foo5 {
          test_bar1: 11
          testBar2: 22
          TestBar3: "33"
          Testbar4: "44"
          test_Bar5: 55
          Test_Bar6: 66
          Test_bar7: 7.7
        }
         * */
    }
    {//json 2 pb
        server::test_proto3 testProto1;
        google::protobuf::util::JsonParseOptions oParseOptions;
        google::protobuf::util::Status oStatus;
        oStatus = google::protobuf::util::JsonStringToMessage(strJson, &testProto1, oParseOptions);
        if(!oStatus.ok())
        {
            LOG4_WARN("test_proto3 failed to JsonStringToMessage,error_code(%d),error_message(%s),strJson:%s",
                            oStatus.error_code(),oStatus.error_message().data(),strJson.c_str());
            return;
        }
        LOG4_DEBUG("test_proto3 JsonStringToMessage testProto1:%s,testProto1 descriptor:%s",
                        testProto1.DebugString().c_str(),testProto1.descriptor()->DebugString().c_str());
        //test_proto3 JsonStringToMessage testProto1:test_foo1: "foo1"
        //test_foo1: "foo11"
        //test_foo2: "foo2"
        //test_foo2: "foo22"
        //test_foo3: 3
        //test_foo3: 33
        //test_foo4: 4
        //test_foo4: 44
        //test_foo5 {
        //  test_bar1: 1
        //  testBar2: 2
        //  TestBar3: "3"
        //  Testbar4: "4"
        //  test_Bar5: 5
        //  Test_Bar6: 6
        //  Test_bar7: 7
        //}
        //test_foo5 {
        //  test_bar1: 11
        //  testBar2: 22
        //  TestBar3: "33"
        //  Testbar4: "44"
        //  test_Bar5: 55
        //  Test_Bar6: 66
        //  Test_bar7: 7.7
        //}
        //,testProto1 descriptor:message test_proto3 {
        //  repeated string test_foo1 = 1;
        //  repeated bytes test_foo2 = 2;
        //  repeated int32 test_foo3 = 3;
        //  repeated uint64 test_foo4 = 4;
        //  repeated .test_bar test_foo5 = 5;
        //}
        m_RunClock.StartClock("json 2 pb");
        for(int i = 0;i < 10000;++i)
        {
        	server::test_proto3 testProto1;
            google::protobuf::util::JsonParseOptions oParseOptions;
            google::protobuf::util::Status oStatus;
            oStatus = google::protobuf::util::JsonStringToMessage(strJson, &testProto1, oParseOptions);
            if(!oStatus.ok())
            {
                LOG4_WARN("test_proto3 failed to JsonStringToMessage,error_code(%d),error_message(%s),strJson:%s",
                                oStatus.error_code(),oStatus.error_message().data(),strJson.c_str());
                return;
            }
        }
        m_RunClock.EndClock();
        //EndClock() net::RunClock json 2 pb use time(659.744995) ms  不加空格和换行
        LOG4_DEBUG("test_proto3 JsonStringToMessage 10000 times testProto1:%s,testProto1 descriptor:%s",
                                testProto1.DebugString().c_str(),testProto1.descriptor()->DebugString().c_str());
    }
}

} /* namespace core */
