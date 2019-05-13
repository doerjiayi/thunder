/*******************************************************************************
 * Project:  Net
 * @file     Session.cpp
 * @brief 
 * @author   bwarliao
 * @date:    2015年7月28日
 * @note
 * Modify history:
 ******************************************************************************/
#include "step/Step.hpp"
#include "Session.hpp"

namespace net
{

Session::Session(uint64 ulSessionId, ev_tstamp dSessionTimeout, const std::string& strSessionClass)
    : m_bRegistered(false), m_dSessionTimeout(dSessionTimeout), m_activeTime(0),
      m_strSessionClassName(strSessionClass),m_pTimeoutWatcher(0),m_boPermanent(false)
{
    char szSessionId[32] = {0};
    snprintf(szSessionId, sizeof(szSessionId), "%llu", ulSessionId);
    m_strSessionId = szSessionId;
}

Session::Session(const std::string& strSessionId, ev_tstamp dSessionTimeout, const std::string& strSessionClass)
    : m_bRegistered(false), m_dSessionTimeout(dSessionTimeout), m_activeTime(0),
      m_strSessionId(strSessionId), m_strSessionClassName(strSessionClass),m_pTimeoutWatcher(0),m_boPermanent(false)
{
}

Session::~Session()
{
	SAFE_DELETE(m_pTimeoutWatcher)
}

} /* namespace net */
