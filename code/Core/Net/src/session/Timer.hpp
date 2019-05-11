/*******************************************************************************
 * Project:  Net
 * @file     Timer.hpp
 * @brief 
 * @author   cjy
 * @date:    2016年7月25日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_SESSION_TIMER_HPP_
#define SRC_SESSION_TIMER_HPP_

#include "session/Session.hpp"

namespace net
{

class Timer: public Session
{
public:
    Timer(uint32 ulTimerId, ev_tstamp dTimeout = 60.0, const std::string& strTimerName = "net::Timer");
    Timer(const std::string& strTimerId, ev_tstamp dTimeout = 60.0, const std::string& strTimerName = "net::Timer");
    virtual ~Timer();

protected:
    virtual void SetActiveTime(ev_tstamp activeTime)
    {
        ;
    }

private:
    friend class Worker;
};

} /* namespace net */

#endif /* SRC_SESSION_TIMER_HPP_ */
