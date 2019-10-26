/*
 * HelloSession.cpp
 *
 *  Created on: 2016骞�11鏈�23鏃�
 *      Author: chenjiayi
 */
#include "HelloSession.h"

namespace core
{

bool HelloSession::Init(const util::CJsonObject& conf)
{
    if(boInit)
    {
        return true;
    }
    conf.Get("module_locate_data_request", m_objModuleLocateDataRequest);
    conf.Get("access_control_allow_origin",m_AccessControlAllowOrigin);
    conf.Get("access_control_allow_headers",m_AccessControlAllowHeaders);
    conf.Get("access_control_allow_methods",m_AccessControlAllowMethods);
    if (!conf.Get("valid_time_delay", m_ValidTimeDelay))
    {
        m_ValidTimeDelay = 60;
    }
    LOG4_INFO("%s valid time delay:%d",__FUNCTION__,m_ValidTimeDelay);
    LOG4_DEBUG("%s() objModuleLocateDataRequest(%s)",
                        __FUNCTION__,m_objModuleLocateDataRequest.ToString().c_str());
    SetCurrentTime();
    boInit = true;
    return true;
}

HelloSession* GetHelloSession()
{
    HelloSession* pSess = (HelloSession*) net::GetSession(HELLO_SESSIN_ID);
    if (pSess)
    {
        return (pSess);
    }
    pSess = new HelloSession();
    if (pSess == NULL)
    {
        LOG4_ERROR("error %d: new HelloSession() error!",ERR_NEW);
        return (NULL);
    }
    util::CJsonObject   oCurrentConf;
	if (!net::GetConfig(oCurrentConf,net::GetConfigPath() + std::string("HelloCmd.json")))
	{
		delete pSess;
		pSess = NULL;
		return (NULL);
	}
    if (net::RegisterCallback(pSess))
    {
        if (!pSess->Init(oCurrentConf))
        {
            g_pLabor->DeleteCallback(pSess);
            LOG4_ERROR("HelloSession init error!");
            return (NULL);
        }
        LOG4_DEBUG("register HelloSession ok!");
        return (pSess);
    }
    else
    {
        LOG4_ERROR("register HelloSession error!");
        delete pSess;
        pSess = NULL;
    }
    return (NULL);
}


}
;
//name space robot
