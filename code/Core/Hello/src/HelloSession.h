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

#include "ProtoError.h"
#include "util/json/CJsonObject.hpp"
#include "dbi/MysqlDbi.hpp"
#include "session/Session.hpp"
#include "NetDefine.hpp"
#include "NetError.hpp"
#include "step/Step.hpp"
#include "cmd/Cmd.hpp"

#define HELLO_SESSIN_ID (20000)

namespace core
{

class HelloSession: public net::Session
{
public:
    HelloSession(double session_timeout = 100.0)
                    : net::Session(HELLO_SESSIN_ID, session_timeout), boInit(false),
                      m_recvCounter(0),m_succCounter(0),m_uiCurrentTime(0),m_ValidTimeDelay(0)
    {
    }
    virtual ~HelloSession()
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
    const util::CJsonObject & GetLocateDataRequest() {return m_objModuleLocateDataRequest;}
    //权限字段
    const std::string&  GetAccessControlAllowOrigin()const {return m_AccessControlAllowOrigin;}
    const std::string&  GetAccessControlAllowHeaders()const {return m_AccessControlAllowHeaders;}
    const std::string&  GetAccessControlAllowMethods()const {return m_AccessControlAllowMethods;}
    uint32 GetValidTimeDelay()const {return m_ValidTimeDelay;}

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
    /*
     *数据库连接配置,如：
       "dbip":"192.168.18.68",
       "dbport":3395,
       "dbuser":"robot",
       "dbpwd":"robot123456",
       "dbname":"db_im3_center",
       "dbcharacterset":"utf8",
     * */
    uint64 m_uiCurrentTime; //当前时间

    util::CJsonObject m_objModuleLocateDataRequest;
    std::string m_AccessControlAllowOrigin;
    std::string m_AccessControlAllowHeaders;
    std::string m_AccessControlAllowMethods;

    uint32 m_ValidTimeDelay;
};

HelloSession* GetHelloSession();

}
;

#endif /* CODE_COLLECTSERVER_SRC_COLLECTSESSION_H_ */
