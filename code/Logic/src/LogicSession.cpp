/*
 * LogicSession.cpp
 *
 *  Created on: 2017年1月19日
 *      Author: chenjiayi
 */
#include <iostream>
#include "LogicSession.h"
#include "util/json/CJsonObject.hpp"

namespace robot
{

LogicSession* g_pLogicSession = NULL;

bool LogicSession::Init(const util::CJsonObject& conf)
{
    if(boInit)return true;
    setCurrentTime();
    boInit = true;
    return true;
}

net::E_CMD_STATUS LogicSession::Timeout()
{
	setCurrentTime();
	auto iter = m_tokenM.begin();
	while(iter != m_tokenM.end())
	{
		if (iter->second.m_uiTimeOut <= m_currenttime)
		{
			LOG4_INFO("strToken(%s) has been time out.(%u,%u)!",
					iter->second.strToken.c_str(),iter->second.m_uiTimeOut,m_currenttime);
			m_tokenM.erase(iter++);
		}
		else
		{
			iter++;
		}
	}
	return net::STATUS_CMD_RUNNING;
}

LogicSession* GetLogicSession()
{
	if (g_pLogicSession) return g_pLogicSession;
	std::string strConfigPath = net::GetConfigPath() + std::string("/LogicCmd.json");
    util::CJsonObject oCurrentConf;       ///< 当前加载的配置
    if (!net::GetConfig(oCurrentConf,strConfigPath))
    {
    	LOG4_ERROR("Open conf (%s) error!",strConfigPath.c_str());
    	return NULL;
    }
    return (g_pLogicSession = net::MakeSession<LogicSession>(ROBOT_SESSIN_ID,std::string("robot::LogicSession"),1.0,oCurrentConf));
}


}
;
//name space robot
