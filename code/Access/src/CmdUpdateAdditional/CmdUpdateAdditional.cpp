/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdUpdateAdditional.cpp
 * @brief    更新Additional
 * @author   ty
 * @date:    2019年8月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdUpdateAdditional.hpp"
#include "util/json/CJsonObject.hpp"
#include "StepUpdateAdditional.hpp"
#include "user.pb.h"
#include "common.pb.h"
#include "user_basic.pb.h"
#include <iostream>

MUDULE_CREATE(im::CmdUpdateAdditional);

namespace im
{

bool CmdUpdateAdditional::AnyMessage(
                const net::tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
	net::ExecStep(new StepUpdateAdditional(stMsgShell, oInMsgHead, oInMsgBody));
    return(true);
}

} /* namespace im */
