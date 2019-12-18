/*******************************************************************************
 * Project:  Hello
 * @file     ModuleEcho.cpp
 * @brief 
 * @author   cjy
 * @date:    2017年2月1日
 * @note
 * Modify history:
 ******************************************************************************/
#include "ModuleEcho.hpp"

#include <map>
#include "util/UnixTime.hpp"
#include "util/HashCalc.hpp"
#include "util/StringCoder.hpp"

MUDULE_CREATE(im::ModuleEcho);

namespace im
{

ModuleEcho::ModuleEcho()
{
}

ModuleEcho::~ModuleEcho()
{
}

bool ModuleEcho::Init()
{
    return(true);
}
//curl 'http://192.168.3.6:27008/Echo'
bool ModuleEcho::AnyMessage(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg)
{
	LOG4_DEBUG("oInHttpMsg:%s",oInHttpMsg.DebugString().c_str());
	Response(stMsgShell,oInHttpMsg,200);
	return true;
}


void ModuleEcho::Response(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,int iCode)
{
    util::CJsonObject oRsp;
    oRsp.Add("code", iCode);
    oRsp.Add("msg", "ok");
    GetLabor()->SendToClient(stMsgShell,oInHttpMsg,oRsp.ToString());
}

} /* namespace core */
