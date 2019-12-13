/*******************************************************************************
 * Project:  Hello
 * @file     ModuleHello.hpp
 * @brief 
 * @author   cjy
 * @date:    2016年4月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ModuleInterface_ModuleInterface_HPP_
#define SRC_ModuleInterface_ModuleInterface_HPP_
#include <map>

#include "google/protobuf/util/json_util.h"
#include "google/protobuf/map.h"
#include "google/protobuf/any.pb.h"
#include "test_proto3.pb.h"
#include "util/encrypt/base64.h"
#include "cmd/Module.hpp"
#include "cmd/Cmd.hpp"
#include "step/Step.hpp"
#include "step/HttpStep.hpp"
#include "ImError.h"
#include "../InterfaceSession.h"
#include "util/CommonUtils.hpp"

namespace im
{

#define GET_TOKEN_GEN (10001)

class ModuleHello: public net::Module
{
public:
    ModuleHello();
    virtual ~ModuleHello();
    virtual bool Init();
    virtual bool AnyMessage(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
    void GenKey(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
    void VerifyKey(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
private:
    void Response(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,int iCode);
};

} /* namespace core */

#endif /* SRC_ModuleInterface_ModuleInterface_HPP_ */
