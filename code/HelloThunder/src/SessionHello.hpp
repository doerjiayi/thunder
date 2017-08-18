/*******************************************************************************
 * Project:  HelloThunder
 * @file     SessionHello.hpp
 * @brief 
 * @author   chenjiayi
 * @date:    2017年10月23日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_SESSIONHELLO_HPP_
#define SRC_SESSIONHELLO_HPP_
#include <time.h>
#include <sys/time.h>
#include "session/Session.hpp"

namespace hello
{

//函数运行时间计算类
class CustomClock
{
public:
    CustomClock()
    {
        m_desc = NULL;
        boStart = false;
    }
    CustomClock(const char* desc,const log4cplus::Logger &logger)
    {
        Start(desc,logger);
    }
    void Start(const char* desc,const log4cplus::Logger &logger)
    {
        if(!boStart)
        {
            m_desc = desc;
            m_logger = logger;
            StartClock();
            boStart = true;
        }
    }
    ~CustomClock()
    {
        EndClock();
    }
    void StartClock()
    {
        gettimeofday(&m_cBeginClock,NULL);
    }
    void EndClock()
    {
        if (boStart)
        {
            gettimeofday(&m_cEndClock,NULL);
            float useTime=1000000*(m_cEndClock.tv_sec-m_cBeginClock.tv_sec)+
                            m_cEndClock.tv_usec-m_cBeginClock.tv_usec;
            useTime/=1000;
            LOG4CPLUS_INFO_FMT(m_logger,"%s() CustomClock %s use time(%lf) ms",__FUNCTION__,m_desc,useTime);
            boStart = false;
        }
    }
    bool boStart;
    timeval m_cBeginClock;
    timeval m_cEndClock;
    const char* m_desc;
    log4cplus::Logger m_logger;
};

class SessionHello: public thunder::Session
{
public:
    SessionHello(unsigned int ulSessionId, ev_tstamp dSessionTimeout = 60.);
    virtual ~SessionHello();

    virtual thunder::E_CMD_STATUS Timeout()
    {
    	if (GetSessionId() == "20000")//常驻会话
    	{
    		return(thunder::STATUS_CMD_RUNNING);
    	}
        return(thunder::STATUS_CMD_COMPLETED);
    }

    int GetHelloNum() const
    {
        return(m_iSayHelloNum);
    }

    void AddHelloNum(int iNum)
    {
        m_iSayHelloNum += iNum;
    }

private:
    int m_iSayHelloNum;
};

} /* namespace hello */

#endif /* SRC_SESSIONHELLO_HPP_ */
