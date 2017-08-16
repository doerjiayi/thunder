/*******************************************************************************
 * Project:  HelloThunder
 * @file     CmdHello.hpp
 * @brief    Server测试程序
 * @author   chenjiayi
 * @date:    2017年8月24日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef CMDHELLO_HPP_
#define CMDHELLO_HPP_

#include "cmd/Cmd.hpp"
#include "StepHello.hpp"
#include "StepGetHelloName.hpp"
#include "StepHttpRequest.hpp"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief 创建函数声明
 * @note 插件代码编译成so后存放到PluginServer的plugin目录，PluginServer加载动态库后调用create()
 * 创建插件类实例。
 */
thunder::Cmd* create();
#ifdef __cplusplus
}
#endif

namespace hello
{

class CmdHello: public thunder::Cmd
{
public:
    CmdHello();
    virtual ~CmdHello();

    virtual bool AnyMessage(
                    const thunder::MsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
private:
    bool TestStepHello(const thunder::MsgShell& stMsgShell,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody);
    bool TestHttpRequest(const thunder::MsgShell& stMsgShell,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody);
    bool TestRedisCmd(const thunder::MsgShell& stMsgShell,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody);
    StepHello* pStepHello;
    StepGetHelloName* pStepGetHelloName;
    StepHttpRequest* pStepHttpRequest;
};

} /* namespace hello */

#endif /* CMDHELLO_HPP_ */
