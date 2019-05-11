/*******************************************************************************
 * Project:  CenterServer
 * @file     CmdNodeRegister.hpp
 * @brief    节点上报
 * @author   cjy
 * @date:    2017年1月13日
 * @note     其它模块向CENTER报告相关信息
 * Modify history:
 ******************************************************************************/
#ifndef CMD_NODE_REPORT_HPP_
#define CMD_NODE_REPORT_HPP_

#include "protocol/oss_sys.pb.h"
#include "cmd/Cmd.hpp"
#include "dbi/MysqlDbi.hpp"
#include "../Comm.hpp"
#include "../NodeSession.h"

namespace core
{
/**
 * @brief   节点上报
 * @author  chenjiayi
 * @date    2015年8月9日
 * @note    各个模块启动时需要向CENTER进行上报数据
 */
class CmdNodeReport : public net::Cmd
{
public:
    CmdNodeReport();
    virtual ~CmdNodeReport();
    virtual bool Init();
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
private:
    bool Response(const net::tagMsgShell& stMsgShell,
                        const MsgHead& oInMsgHead,int iRet);
    NodeSession* pSess;
    bool boInit;
};

} /* namespace core */

#endif /* CMD_NODE_REPORT_HPP_ */
