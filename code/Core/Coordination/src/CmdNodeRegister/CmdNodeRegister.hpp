/*******************************************************************************
 * Project:  Beacon
 * @file     CmdRegister.hpp
 * @brief    节点注册
 * @author   bwar
 * @date:    Sep 19, 2016
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDNODEREGISTER_HPP_
#define SRC_CMDNODEREGISTER_HPP_

#include "Comm.hpp"
#include "SessionOnlineNodes.hpp"
namespace coor
{

class CmdNodeRegister: public net::Cmd
{
public:
    CmdNodeRegister() = default;
    virtual ~CmdNodeRegister() = default;

    virtual bool Init();
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);
protected:
    bool InitFromDb(const util::CJsonObject& oDbConf);
    bool InitFromLocal(const util::CJsonObject& oLocalConf);

private:
    SessionOnlineNodes* m_pSessionOnlineNodes = nullptr;
};

} /* namespace coor */

#endif /* SRC_CMDNODEREGISTER_CMDNODEREGISTER_HPP_ */
