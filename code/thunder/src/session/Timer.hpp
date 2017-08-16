/*******************************************************************************
 * Project:  Thunder
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

namespace thunder
{

class Timer: public Session
{
public:
    Timer(uint32 ulTimerId, ev_tstamp dTimeout = 60.0, const std::string& strTimerName = "thunder::Timer");
    Timer(const std::string& strTimerId, ev_tstamp dTimeout = 60.0, const std::string& strTimerName = "thunder::Timer");
    virtual ~Timer();

protected:
    virtual void SetActiveTime(ev_tstamp activeTime)
    {
        ;
    }

private:
    friend class ThunderWorker;
};

} /* namespace thunder */

#endif /* SRC_SESSION_TIMER_HPP_ */
