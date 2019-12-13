/*
 * HelloSession.h
 *
 *  Created on: 2015年10月21日
 *      Author: chen
 */
#ifndef CODE_COLLECTSERVER_SRC_COLLECTSESSION_H_
#define CODE_COLLECTSERVER_SRC_COLLECTSESSION_H_
#include <string>
#include <map>
#include <set>

#include "util/json/CJsonObject.hpp"
#include "dbi/MysqlDbi.hpp"
#include "session/Session.hpp"
#include "NetDefine.hpp"
#include "NetError.hpp"
#include "step/Step.hpp"
#include "cmd/Cmd.hpp"
#include "ImError.h"

#define HELLO_SESSIN_ID (20000)

namespace im
{

class InterfaceSession: public net::Session
{
public:
	InterfaceSession(uint64 ulSessionId, ev_tstamp dSessionTimeout, const std::string& strSessionClass)
		: net::Session(ulSessionId, dSessionTimeout,strSessionClass), boInit(false),m_recvCounter(0),m_succCounter(0),m_uiCurrentTime(0)
    {
    }
    virtual ~InterfaceSession()
    {
    }
    bool Init(const util::CJsonObject& conf);
    net::E_CMD_STATUS Timeout()
    {
        return net::STATUS_CMD_RUNNING;
    }
    void SetCurrentTime()
    {
        m_uiCurrentTime = ::time(NULL);
    }
    void IncrRecv()
    {
        if (m_recvCounter & 0x100000000)//4294967296
        {m_recvCounter = 0;m_succCounter = 0;}
        ++m_recvCounter;
    }
    void IncrSucc(){++m_succCounter;}
    uint64 GetRecv()const{return m_recvCounter;}
    uint64 GetSucc()const{return m_succCounter;}
    uint64 GetFailed()const{return m_recvCounter - m_succCounter;}
private:
    bool boInit;
    uint64 m_recvCounter;
    uint64 m_succCounter;
    uint64 m_uiCurrentTime; //当前时间
};

InterfaceSession* GetInterfaceSession();

}
;

#endif /* CODE_COLLECTSERVER_SRC_COLLECTSESSION_H_ */
