/*******************************************************************************
 * Project:  AsyncServer
 * @file     CmdUpdateAdditional.hpp
 * @brief    更新Additional
 * @author   ty
 * @date:    2019年9月18日
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef CMD_UPDATE_ADDITIONAL_HPP_
#define CMD_UPDATE_ADDITIONAL_HPP_

#include "protocol/oss_sys.pb.h"
#include "cmd/Cmd.hpp"
#include "CmdUpdateAdditional.hpp"

namespace im
{
/**
 * @brief   更新Additional通知
 * @author  hty
 * @date    2019年8月9日
 * @note    处理更新被禁数据
 */
class CmdUpdateAdditional : public  net::Cmd
{
public:
    CmdUpdateAdditional(){}
    virtual ~CmdUpdateAdditional(){}
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace im */

#endif /* CMD_UPDATE_ADDITIONAL_HPP_ */
