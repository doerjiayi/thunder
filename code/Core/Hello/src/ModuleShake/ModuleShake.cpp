/*******************************************************************************
 * Project:  WebServer
 * @file     ModuleHello.cpp
 * @brief 	 用于支持websocket的握手协议
 * @author   cjy
 * @date:    2016年10月31日
 * @note
 * Modify history:
 ******************************************************************************/
#include <time.h>
#include "ModuleShake.hpp"

MUDULE_CREATE(core::ModuleShake);

namespace core
{

ModuleShake::ModuleShake():m_boInit(false)
{
}

ModuleShake::~ModuleShake()
{
}

bool ModuleShake::Init()
{
    if (m_boInit)
    {
        return true;
    }
    HelloSession* pSession = GetHelloSession();
    if(!pSession)
    {
        LOG4_ERROR("failed to get GetNodeSession");
        return false;
    }
    m_boInit = true;
    return true;
}

bool ModuleShake::AnyMessage(
                const net::tagMsgShell& stMsgShell,
                const HttpMsg& oInHttpMsg)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    ws_req_t ws_req;
    if(!ParseWebsocketHandshake(oInHttpMsg,ws_req))
    {
        LOG4_DEBUG("%s() failed to ParseWebsocketHandshake", __FUNCTION__);
        return ResponseHttp(stMsgShell,oInHttpMsg,1000,"failed to ParseWebsocketHandshake");
    }
    if(strcasecmp(ws_req.upgrade.c_str(),"websocket") == 0)//websocket升级握手消息
    {
        LOG4_DEBUG("ParseWebsocketHandshake ok");
        return ResponseWebsocketResponseHandshake(stMsgShell,ws_req,oInHttpMsg);
    }
    else//普通的http消息
    {
        LOG4_DEBUG("ParseWebsocketHandshake other msg");
        return ResponseHttp(stMsgShell,oInHttpMsg,1000,"ParseWebsocketHandshake other msg");
    }
    return (true);
}

bool ModuleShake::ResponseWebsocketResponseHandshake(const net::tagMsgShell& stMsgShell,const ws_req_t &ws_req,
                const HttpMsg& oInHttpMsg)
{
    LOG4_DEBUG("WebsocketResponseHandshake");
    HttpMsg oOutHttpMsg;
    oOutHttpMsg.set_type(HTTP_RESPONSE);
    oOutHttpMsg.set_status_code(101);
    oOutHttpMsg.set_http_major(oInHttpMsg.http_major());
    oOutHttpMsg.set_http_minor(oInHttpMsg.http_minor());
    oOutHttpMsg.set_url("Switching Protocols");
    //Connection
    HttpMsg::Header* header = oOutHttpMsg.add_headers();
    header->set_header_name("Connection");
    header->set_header_value("upgrade");
    //Upgrade
    header = oOutHttpMsg.add_headers();
    header->set_header_name("Upgrade");
    header->set_header_value("websocket");
    //Sec-WebSocket-Accept
    header = oOutHttpMsg.add_headers();
    header->set_header_name("Sec-WebSocket-Accept");
    header->set_header_value(GenerateKey(ws_req.sec_websocket_key));
    //Access-Control-Allow-Credentials
    header = oOutHttpMsg.add_headers();
    header->set_header_name("Access-Control-Allow-Credentials");//是否允许请求带有验证信息
    header->set_header_value("true");
    //Access-Control-Allow-Headers
    header = oOutHttpMsg.add_headers();
    header->set_header_name("Access-Control-Allow-Headers");//允许自定义的头部
    header->set_header_value("content-type");
    //Access-Control-Allow-Origin
    header = oOutHttpMsg.add_headers();
    header->set_header_name("Access-Control-Allow-Headers");//允许任何来自任意域的跨域请求,需要以后控制
    header->set_header_value("*");
    //Server
    header = oOutHttpMsg.add_headers();
    header->set_header_name("Server");
    header->set_header_value("WebSocketServer");
    //Origin
    if (!ws_req.origin.empty())//普通的HTTP请求也会带有，在CORS中专门作为Origin信息供后端比对,表明来源域
    {
        header = oOutHttpMsg.add_headers();
        header->set_header_name("Origin");
        header->set_header_value(ws_req.origin);
    }
    //Sec-WebSocket-Protocol
    if (!ws_req.sec_webSocket_protocol.empty())
    {
        header = oOutHttpMsg.add_headers();
        header->set_header_name("Sec-WebSocket-Protocol");
        header->set_header_value(ws_req.sec_webSocket_protocol);
    }
    LOG4_DEBUG("%s", ToString(oOutHttpMsg).c_str());
    return g_pLabor->SendTo(stMsgShell, oOutHttpMsg);
}

std::string ModuleShake::GenerateKey(const std::string &key)
{
    //sha-1
    std::string tmp = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char digest[20] = {0};
    SHA1((const unsigned char*)tmp.c_str(), tmp.length(), digest);
    //base64 encode
    char szToBase64[128] = {0};
    Base64encode(szToBase64, (const char*)digest, 20);
    return std::string(szToBase64);
}

const std::string& ModuleShake::ToString(const HttpMsg& oHttpMsg)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    m_strHttpString.clear();
    char prover[16];
    sprintf(prover, "HTTP/%u.%u", oHttpMsg.http_major(), oHttpMsg.http_minor());

    if (HTTP_REQUEST == oHttpMsg.type())
    {
        if (HTTP_POST == oHttpMsg.method() || HTTP_GET == oHttpMsg.method())
        {
            m_strHttpString += http_method_str((http_method)oHttpMsg.method());
            m_strHttpString += " ";
        }
        m_strHttpString += oHttpMsg.url();
        m_strHttpString += " ";
        m_strHttpString += prover;
        m_strHttpString += "\r\n";
    }
    else
    {
        m_strHttpString += prover;
        if (oHttpMsg.status_code())
        {
            char tmp[10];
            sprintf(tmp, " %u\r\n", oHttpMsg.status_code());
            m_strHttpString += tmp;
        }
    }
    for (int i = 0; i < oHttpMsg.headers_size(); ++i)
    {
        m_strHttpString += oHttpMsg.headers(i).header_name();
        m_strHttpString += ":";
        m_strHttpString += oHttpMsg.headers(i).header_value();
        m_strHttpString += "\r\n";
    }
    m_strHttpString += "\r\n";

    if (oHttpMsg.body().size())
    {
        m_strHttpString += oHttpMsg.body();
        m_strHttpString += "\r\n\r\n";
    }
    return(m_strHttpString);
}

bool ModuleShake::ParseWebsocketHandshake(HttpMsg oInHttpMsg,ws_req_t &ws_req)
{
    /*最初的握手http包，如：
    GET /chat HTTP/1.1
    Host: server.example.com
    Upgrade: websocket
    Connection: Upgrade
    Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
    Origin: http://example.com
    Sec-WebSocket-Protocol: chat, superchat
    Sec-WebSocket-Version: 13
    * */
    for (int i = 0; i < oInHttpMsg.headers_size(); ++i)
    {
        if (std::string("Connection") == oInHttpMsg.headers(i).header_name())
        {
            ws_req.connection = oInHttpMsg.headers(i).header_value();
        }
        else if (std::string("Upgrade") == oInHttpMsg.headers(i).header_name())
        {
            ws_req.upgrade = oInHttpMsg.headers(i).header_value();
        }
        else if (std::string("Host") == oInHttpMsg.headers(i).header_name())
        {
            ws_req.host = oInHttpMsg.headers(i).header_value();
        }
        else if (std::string("Origin") == oInHttpMsg.headers(i).header_name())
        {
            ws_req.origin = oInHttpMsg.headers(i).header_value();
        }
        else if (std::string("Cookie") == oInHttpMsg.headers(i).header_name())
        {
            ws_req.cookie = oInHttpMsg.headers(i).header_value();
        }
        else if (std::string("Sec-WebSocket-Key") == oInHttpMsg.headers(i).header_name())
        {
            ws_req.sec_websocket_key = oInHttpMsg.headers(i).header_value();
        }
        else if (std::string("Sec-WebSocket-Version") == oInHttpMsg.headers(i).header_name())
        {
            ws_req.sec_websocket_version = oInHttpMsg.headers(i).header_value();
        }
        else if (std::string("Sec-WebSocket-Protocol") == oInHttpMsg.headers(i).header_name())
        {
            ws_req.sec_webSocket_protocol = oInHttpMsg.headers(i).header_value();
        }
    }
    return true;
}
bool ModuleShake::ResponseHttp(const net::tagMsgShell& stMsgShell, const HttpMsg& oInHttpMsg,
                int iCode,const std::string &msg)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    HttpMsg oOutHttpMsg;
    oOutHttpMsg.set_type(HTTP_RESPONSE);
    oOutHttpMsg.set_method(HTTP_POST);
    oOutHttpMsg.set_status_code(server_err_code(iCode));
    oOutHttpMsg.set_http_major(oInHttpMsg.http_major());
    oOutHttpMsg.set_http_minor(oInHttpMsg.http_minor());
    oOutHttpMsg.set_body(msg);
    g_pLabor->SendTo(stMsgShell, oOutHttpMsg);
    return(true);
}





} /* namespace core */
