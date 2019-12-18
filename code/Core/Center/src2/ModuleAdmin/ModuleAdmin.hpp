/*******************************************************************************
 * Project:  NebulaCenter
 * @file     ModuleAdmin.hpp
 * @brief    Nebula集群管理
 * @author   Bwar
 * @date:    2018年12月8日
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef MODULEADMIN_HPP
#define MODULEADMIN_HPP

#include "Comm.hpp"
#include "SessionOnlineNodes.hpp"
#include "StepGetConfig.hpp"
#include "StepSetConfig.hpp"

namespace coor
{

/**
 * @brief 集群管理
 * @note 集群管理通过将命令和参数封在JSON体并填充http body，再post到Center节点。
 * 命令管理的JSON体格式如下：
 * {
 *     "cmd":"show",
 *     "args":["ip_white"]
 * }
 *
 * 命令帮助：
 *     show:
 *         show ip_white
 *         show subscription
 *         show subscription ${node_type}
 *         show nodes
 *         show nodes ${node_type}
 *         show node_report ${node_type}
 *         show node_report ${node_type} ${node_identify}
 *         show node_detail ${node_type}
 *         show node_detail ${node_type} ${node_identify}
 *         show beacon
 *     get:
 *         get node_config ${node_identify}
 *         get node_custom_config ${node_identify}
 *         get custom_config ${node_identify} ${config_file_relative_path} ${config_file_name}
 *     set:
 *         set node_config ${node_type} ${config_file_content}
 *         set node_config ${node_type} ${node_identify} ${config_file_content}
 *         set node_config_from_file ${node_type} ${config_file}
 *         set node_config_from_file ${node_type} ${node_identify} ${config_file}
 *         set node_custom_config ${node_type} ${config_content}
 *         set node_custom_config ${node_type} ${node_identify} ${config_content}
 *         set node_custom_config_from_file ${node_type} ${config_file}
 *         set node_custom_config_from_file ${node_type} ${node_identify} ${config_file}
 *         set custom_config ${node_type} ${config_file_name} ${config_file_content}
 *         set custom_config ${node_type} ${config_file_relative_path} ${config_file_name} ${config_file_content}
 *         set custom_config ${node_type} ${node_identify} ${config_file_relative_path} ${config_file_name} ${config_file_content}
 *         set custom_config_from_file ${node_type} ${config_file}
 *         set custom_config_from_file ${node_type} ${config_file_relative_path} ${config_file}
 *         set custom_config_from_file ${node_type} ${node_identify} ${config_file_relative_path} ${config_file}
 */ 
class ModuleAdmin: public net::Module
{
public:
    ModuleAdmin() = default;
    virtual ~ModuleAdmin() = default;

    virtual bool Init();

    virtual bool AnyMessage(
                    const net::tagMsgShell& stMsgShell,
                    const HttpMsg& oHttpMsg);
protected:
    void ResponseOptions(
            const net::tagMsgShell& stMsgShell, const HttpMsg& oInHttpMsg);
    void Show(util::CJsonObject& oCmdJson, util::CJsonObject& oResult) const;
    void Get(const net::tagMsgShell& stMsgShell,
            int32 iHttpMajor, int32 iHttpMinor,
            util::CJsonObject& oCmdJson, util::CJsonObject& oResult);
    void Set(const net::tagMsgShell& stMsgShell,
            int32 iHttpMajor, int32 iHttpMinor,
            util::CJsonObject& oCmdJson, util::CJsonObject& oResult);

private:
    SessionOnlineNodes* m_pSessionOnlineNodes = nullptr;
};

} /* namespace coor */

#endif /* MODULEADMIN_HPP */

