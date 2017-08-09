/*******************************************************************************
 * Project:  Thunder
 * @file     StepLog.hpp
 * @brief    发送日志
 * @author   cjy
 * @date:    2015年11月26日
 * @note 发送日志步骤是一个公用状态机，需要使用用户信息数据的Cmd，预先new好自己
 * 的业务StepT，再将自己的StepT作为new StepLog时的一个参数传入，登记并执行
 * StepLog，StepLog的成功回调后会自定调用StepT。
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEPLOG_HPP_
#define SRC_STEPLOG_HPP_
#include "step/Step.hpp"
#include "storage/behaviourlog.pb.h"

namespace thunder
{

/**
 * @brief 发送日志
 * @note 发送日志步骤是一个公用状态机，需要使用用户信息数据的Cmd，预先new好自己的业务StepT，
 * 再将自己的StepT作为new StepLog时的一个参数传入，登记并执行StepLog，StepLog的
 * 成功回调后会自定调用StepT。
 */
class StepLog: public thunder::Step
{
public:
    /**
     * @brief 发送日志步骤
     * @param pNextStep 成功加载用户信息后需执行的下一个状态机
     */
    StepLog(const ::google::protobuf::Message &behaviourLog, thunder::uint32 logType,thunder::Step* pNextStep = NULL,const std::string &nodeType = "ESAGENT_W");
    virtual ~StepLog();

    /**
     * @brief 发送日志回调
     * @note 加载用户信息回调成功则会自动执行下一状态机的Emit()回到具体业务逻辑状态机控制
     * @param stMsgShell 消息外壳
     * @param oInMsgHead 消息头
     * @param oInMsgBody 消息体
     * @param data 附加数据
     * @return 运行结果
     */
    virtual thunder::E_CMD_STATUS Callback(
        const thunder::MsgShell& stMsgShell,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody,
        void* data = NULL);

    /**
     * @brief 加载用户信息超时回调
     * @return 运行结果
     */
    virtual thunder::E_CMD_STATUS Timeout();

    /**
     * @brief 发起加载用户信息
     * @note 参数都在构造时通过构造函数传入，执行Emit()函数反而增添填充无用参数的麻烦，所以
     * 建议用Emit()替代Emit()
     * @return 运行结果
     */
    virtual thunder::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg = "", const std::string& strErrShow = "");

private:
    BehaviourLog::behaviour_log_req m_behaviour_log;
    std::string m_nodeType;
};

} /* namespace robot */

#endif /* SRC_STEPLOADUSER_HPP_ */
