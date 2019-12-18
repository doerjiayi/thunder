/*******************************************************************************
 * Project:  Net
 * @file     Session.hpp
 * @brief    会话基类
 * @author   cjy
 * @date:    2019年7月28日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SESSION_HPP_
#define SESSION_HPP_

#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"
#include "libev/ev.h"         // need ev_tstamp
#include "NetDefine.hpp"
#include "NetError.hpp"
#include "labor/Labor.hpp"

namespace net
{

class Worker;

class Session
{
public:
    Session(uint64 ulSessionId, ev_tstamp dSessionTimeout = 60.0, const std::string& strSessionClass = "net::Session");
    Session(const std::string& strSessionId, ev_tstamp dSessionTimeout = 60.0, const std::string& strSessionClass = "net::Session");
    virtual ~Session();
    /**
     * @brief 会话超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;
    const std::string& GetSessionId() const{return(m_strSessionId);}
    const std::string& GetSessionClass() const{return(m_strSessionClassName);}
    void SetPermanent(){m_boPermanent = true;}//设置会话为永久会话（表示会话不会因超时而注销，用于异步发送的参数）
    bool IsPermanent()const{return m_boPermanent;}
    bool Init(const util::CJsonObject &conf){return true;}//需要初始化的则继承
public:
protected:
    /**
     * @brief 设置会话最近刷新时间
     */
    virtual void SetActiveTime(ev_tstamp activeTime){m_activeTime = activeTime;}
    /**
     * @brief 获取会话刷新时间
     */
    ev_tstamp GetActiveTime() const{return(m_activeTime);}
    /**
     * @brief 获取会话刷新时间
     */
    ev_tstamp GetTimeout() const{return(m_dSessionTimeout);}
    /**
     * @brief 是否注册了回调
     * @return 注册结果
     */
    bool IsRegistered() const{return(m_bRegistered);}
private:
    /**
     * @brief 设置为已注册状态
     */
    void SetRegistered(){m_bRegistered = true;}
private:
    bool m_bRegistered;
    ev_tstamp m_dSessionTimeout;
    ev_tstamp m_activeTime;
    std::string m_strSessionId;
    std::string m_strSessionClassName;
    ev_timer* m_pTimeoutWatcher;

    bool m_boPermanent;
    friend class Worker;
};

template <typename T> T* MakeSession(const std::string& strSessionId, const std::string& strSessionClass,ev_tstamp dSessionTimeout=60.0,const util::CJsonObject &conf = util::CJsonObject())
{
	T* pSession = (T*) GetLabor()->GetSession(strSessionId,strSessionClass);
	if (pSession)
	{
		return (pSession);
	}
	pSession = new T(strSessionId,dSessionTimeout,strSessionClass);
	if (pSession == NULL)
	{
		LOG4_ERROR("error %d: new %s() error!",net::ERR_NEW,strSessionClass.c_str());
		return (NULL);
	}
	if (GetLabor()->RegisterCallback(pSession))
	{
		if (!pSession->Init(conf))
		{
			GetLabor()->DeleteCallback(pSession);
			return NULL;
		}
		return (pSession);
	}
	else
	{
		LOG4_ERROR( "register pSession error!");
		delete pSession;
		pSession = NULL;
	}
	return (NULL);
}

template <typename T> T* MakeSession(uint64 uiSessionId, const std::string& strSessionClass,ev_tstamp dSessionTimeout=60.0,const util::CJsonObject &conf = util::CJsonObject())
{
	T* pSession = (T*) GetLabor()->GetSession(uiSessionId,strSessionClass);
	if (pSession)
	{
		return (pSession);
	}
	pSession = new T(uiSessionId,dSessionTimeout,strSessionClass);
	if (pSession == NULL)
	{
		LOG4_ERROR("error %d: new %s() error!",net::ERR_NEW,strSessionClass.c_str());
		return (NULL);
	}
	if (GetLabor()->RegisterCallback(pSession))
	{
		if (!pSession->Init(conf))
		{
			GetLabor()->DeleteCallback(pSession);
			return NULL;
		}
		return (pSession);
	}
	else
	{
		LOG4_ERROR( "register pSession error!");
		delete pSession;
		pSession = NULL;
	}
	return (NULL);
}


} /* namespace net */

#endif /* SESSION_HPP_ */
