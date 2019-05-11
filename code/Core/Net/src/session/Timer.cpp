/*******************************************************************************
 * Project:  Net
 * @file     Timer.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年7月25日
 * @note
 * Modify history:
 ******************************************************************************/
#include "session/Timer.hpp"

namespace net
{

Timer::Timer(uint32 ulTimerId, ev_tstamp dTimeout, const std::string& strTimerName)
    : net::Session(ulTimerId, dTimeout, strTimerName)
{
}

Timer::Timer(const std::string& strTimerId, ev_tstamp dTimeout, const std::string& strTimerName)
    : net::Session(strTimerId, dTimeout, strTimerName)
{
}

Timer::~Timer()
{
}

} /* namespace net */
