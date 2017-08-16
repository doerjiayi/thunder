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

#include "../ModuleHello/StepGetHelloName.hpp"
#include "../ModuleHello/StepHello.hpp"
#include "../ModuleHello/StepHttpRequest.hpp"
#include "cmd/Cmd.hpp"
#include "cmd/Module.hpp"

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
class ModuleHello: public thunder::Module
{
public:
	ModuleHello();
    virtual ~ModuleHello();
    virtual bool AnyMessage(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
private:
    bool TestStepHello(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
    bool TestHttpRequest(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
    bool TestRedisCmd(const thunder::MsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
    StepHello* pStepHello;
    StepGetHelloName* pStepGetHelloName;
    StepHttpRequest* pStepHttpRequest;
};

} /* namespace hello */

#endif /* CMDHELLO_HPP_ */
