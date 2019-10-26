/*******************************************************************************
 * Project:  WebServer
 * @file     ModuleHello.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年10月31日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_MODULESHAKE_MODULESHAKE_HPP_
#define SRC_MODULESHAKE_MODULESHAKE_HPP_
#include <openssl/sha.h>

#include "ProtoError.h"
#include "../HelloSession.h"
#include "util/encrypt/base64.h"
#include "cmd/Module.hpp"

namespace core
{
/*
GET /im/web/shake HTTP/1.1
Host: 192.168.18.68:10001
Connection: Upgrade
Pragma: no-cache
Cache-Control: no-cache
Upgrade: websocket
Origin: file://
Sec-WebSocket-Version: 13
User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.102 UBrowser/5.7.16173.12 Safari/537.36
Accept-Encoding: gzip, deflate
Accept-Language: zh-CN,zh;q=0.8
Sec-WebSocket-Key: zR6gcZngWdJ/fDOcSlEOyw==
Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits
 * */
//WebSocket request
typedef struct WebsocketRequest {
    std::string req;
    std::string connection;
    std::string upgrade;
    std::string host;
    std::string origin;
    std::string cookie;
    std::string sec_websocket_key;
    std::string sec_websocket_version;
    std::string sec_webSocket_protocol;
} ws_req_t;

/*
HTTP/1.1 101 Switching Protocols
Connection: upgrade
Sec-WebSocket-Accept: dRpPUmRvUKku/5wuyRiZkdZCeG8=
Upgrade: websocket
 * */
//WebSocket response
typedef struct WebsocketResponse {
    std::string resp;
    std::string date;
    std::string connection;
    std::string server;
    std::string upgrade;
    std::string access_control_allow_origin;
    std::string access_control_allow_credentials;
    std::string sec_websocket_accept;
    std::string access_control_allow_headers;
} ws_resp_t;

class ModuleShake: public net::Module
{
public:
    ModuleShake();
    virtual ~ModuleShake();
    bool Init();
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const HttpMsg& oInHttpMsg);
    bool ParseWebsocketHandshake(HttpMsg oHttpMsg,ws_req_t &ws_req);
private:
    bool ResponseWebsocketResponseHandshake(const net::tagMsgShell& stMsgShell,const ws_req_t &ws_req,
                        const HttpMsg& oHttpMsg);
    bool ResponseHttp(const net::tagMsgShell& stMsgShell, const HttpMsg& oInHttpMsg,
                        int iCode,const std::string &msg);
private:
    std::string GenerateKey(const std::string &key);
    const std::string& ToString(const HttpMsg& oHttpMsg);
    std::string m_strHttpString;
    bool m_boInit;
};

} /* namespace core */

#endif /* SRC_MODULESHAKE_MODULESHAKE_HPP_ */
