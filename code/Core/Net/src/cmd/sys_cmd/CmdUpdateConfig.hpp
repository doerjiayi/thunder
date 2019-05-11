/*******************************************************************************
* * Project:  bolt
 * @file     CmdUpdateConfig.hpp
 * @brief    更新配置
 * @author   chenjiayi
 * @date:    2016年11月5日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_UPDATE_CONFIG_HPP_
#define SRC_UPDATE_CONFIG_HPP_

#include "cmd/Cmd.hpp"

namespace net
{

class CmdUpdateConfig : public Cmd
{
public:
    CmdUpdateConfig();
    virtual ~CmdUpdateConfig();
    virtual bool AnyMessage(
                    const tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
private:
    bool ReadConfig();
    std::string m_ReqConfigFileName;
    util::CJsonObject m_ReqConfigContent;
    int m_ReqConfigType;
};

} /* namespace bolt */

#endif /* SRC_CMD_SYS_CMD_CMDBEAT_HPP_ */
