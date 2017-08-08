/*******************************************************************************
 * Project:  Starship
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

namespace oss
{

class Timer: public Session
{
public:
    Timer(uint32 ulTimerId, ev_tstamp dTimeout = 60.0, const std::string& strTimerName = "oss::Timer");
    Timer(const std::string& strTimerId, ev_tstamp dTimeout = 60.0, const std::string& strTimerName = "oss::Timer");
    virtual ~Timer();

protected:
    virtual void SetActiveTime(ev_tstamp activeTime)
    {
        ;
    }

private:
    friend class OssWorker;
};

} /* namespace oss */

#endif /* SRC_SESSION_TIMER_HPP_ */
