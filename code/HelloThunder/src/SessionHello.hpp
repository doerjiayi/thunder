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

#include "session/Session.hpp"

namespace hello
{

class SessionHello: public thunder::Session
{
public:
    SessionHello(unsigned int ulSessionId, ev_tstamp dSessionTimeout = 60.);
    virtual ~SessionHello();

    virtual thunder::E_CMD_STATUS Timeout()
    {
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
