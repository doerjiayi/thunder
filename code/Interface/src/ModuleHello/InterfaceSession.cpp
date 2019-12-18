/*
 * InterfaceSession.cpp
 *
 *  Created on: 2016骞�11鏈�23鏃�
 *      Author: chenjiayi
 */
#include "InterfaceSession.h"

namespace robot
{

InterfaceSession* g_pInterfaceSession = NULL;

bool InterfaceSession::Init(const util::CJsonObject& conf)
{
    if(boInit)
    {
        return true;
    }
    SetCurrentTime();
    boInit = true;
    return true;
}



InterfaceSession* GetInterfaceSession()
{
	if (g_pInterfaceSession) return g_pInterfaceSession;
	util::CJsonObject oCurrentConf;       ///< 当前加载的配置
	if (!net::GetConfig(oCurrentConf,net::GetConfigPath() + std::string("/InterfaceCmd.json")))
	{
		LOG4_ERROR("Open conf (%s) error!",(net::GetConfigPath() + std::string("/InterfaceCmd.json")).c_str());
		return NULL;
	}
	return (g_pInterfaceSession = net::MakeSession<InterfaceSession>(HELLO_SESSIN_ID,std::string("robot::InterfaceSession"),5.0,oCurrentConf));
}


}
;
//name space robot
