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

namespace coor
{

class CmdNodeRegister: public net::Cmd
{
public:
    CmdNodeRegister(int32 iCmd);
    virtual ~CmdNodeRegister();

    virtual bool Init();
    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);
protected:
    bool InitFromDb(const util::CJsonObject& oDbConf);
    bool InitFromLocal(const util::CJsonObject& oLocalConf);

private:
    std::shared_ptr<SessionOnlineNodes> m_pSessionOnlineNodes;
};

} /* namespace coor */

#endif /* SRC_CMDNODEREGISTER_CMDNODEREGISTER_HPP_ */
