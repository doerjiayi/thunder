/*******************************************************************************
* Project:  proto
* @file     ImErrorMapping.h
* @brief    IM错误与系统错误映射
* @author   cjy
* @date:    2016年4月9日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_IMERRORMAPPING_H_
#define SRC_IMERRORMAPPING_H_

#include "NetError.hpp"
#include "ImError.h"

namespace im
{

inline int robot_err_code(int code)
{
    switch (code)
    {
        case net::ERR_OK:                               return ERR_OK;
        case net::ERR_PARASE_PROTOBUF:                  return ERR_PARASE_PROTOBUF;
        case net::ERR_UNKNOWN_CMD:                      return ERR_UNKNOWN_CMD;
        case net::ERR_SERVERINFO:                       return ERR_SERVERINFO;
        case net::ERR_BODY_JSON:                        return ERR_BODY_JSON;
        case net::ERR_SERVERINFO_RECORD:                return ERR_SERVERINFO_RECORD;
        case net::ERR_TIMEOUT:                          return ERR_TIMEOUT;
    }
    if (code < 10000)
    {
        return ERR_SYSTEM_ERROR;//系统错误
    }
    else if (code >= 10000 && code < 20000)  // 此种情况应在上面的switch里处理
    {
        return ERR_SERVER_ERROR;//服务器错误
    }
    else if (code >= 20000 && code < 30000)
    {
        return code;//逻辑错误
    }
    return ERR_UNKNOWN;
}

inline const char * robot_err_msg(int code)
{
    switch (code)
    {
        case ERR_OK:                                    return "成功。";

        case ERR_PARASE_PROTOBUF:                       return "协议错误";
        case ERR_UNKNOWN_CMD:                           return "未知的命令字";
        case ERR_SERVERINFO:                            return "系统错误";
        case ERR_BODY_JSON:                             return "协议错误";
        case ERR_SERVERINFO_RECORD:                     return "系统错误";
        case ERR_TIMEOUT:                               return "超时";

        case ERR_UNKNOWN:                               return "未知错误[20200]";
        case ERR_UNKNOW_CMD:                            return "未知命令字[20201]";
        case ERR_SYSTEM_ERROR:                          return "系统错误[20202]";          ///< 系统错误（数据库错误等，通过查询后台Server日志可以找到更具体错误码）
        case ERR_SERVER_BUSY:                           return "系统繁忙[20203]";
        case ERR_SERVER_ERROR:                          return "服务器错误[20204]";
        case ERR_SERVER_TIMEOUT:                        return "系统繁忙，请稍后再尝试[20205]";
        case ERR_UNKNOWN_RESPONSE_CMD:                  return "未知响应命令字[20207]";    ///< 未知响应命令字，通常为对端没有找到对应Cmd处理者，由框架层返回错误
        case ERR_ASYNC_TIMEOUT:                         return "系统繁忙，请稍后再尝试[20208]";
        case ERR_LOAD_CONFIGFILE:                       return "系统错误[20209]";          ///< 模块加载配置文件失败
        case ERR_LOGIC_SERVER_TIMEOUT:                  return "系统繁忙，请稍后再尝试[20213]";    ///< 逻辑Server处理超时
        case ERR_PROC_RELAY:                            return "系统繁忙，请稍后再尝试[20214]";    ///< 数据转发到其它进程失败
        case ERR_SEND_TO_LOGIG_MSG:                     return "系统繁忙，请稍后再尝试[20215]";   ///< 发送消息到逻辑服务器失败
        case ERR_REG_SESSION:                           return "系统繁忙，请稍后再尝试[20216]";   ///< session注册失败
        case ERR_SESSION_CREATE:                        return "系统繁忙，请稍后再尝试[20217]";   ///< 创建 session 失败
        case ERR_MALLOC_FAILED:                         return "系统繁忙，请稍后再尝试[20218]";   ///< new对象失败
        case ERR_SEQUENCE:                              return "系统繁忙，请稍后再尝试[20219]";   ///< 错误序列号
        case ERR_RC5ENCRYPT_ERROR:                      return "加密失败[20220]";                ///< Rc5Encrypt 加密失败
        case ERR_RC5DECRYPT_ERROR:                      return "解密失败[20221]";                ///< Rc5Decrypt 解密失败
        case ERR_REQ_FREQUENCY:                         return "请求过于频繁[20222]";            ///< 请求过于频繁（未避免被攻击而做的系统保护）
        case ERR_NO_SESSION_ID_IN_MSGBODY:              return "数据错误[20301]";                ///< MsgBody缺少session_id
        case ERR_NO_ADDITIONAL_IN_MSGBODY:              return "数据错误[20302]";                ///< MsgBody缺少additional
        case ERR_MSG_BODY_DECODE:                       return "数据错误[20303]";                ///< msg body解析出错
        case ERR_INVALID_PROTOCOL:                      return "协议错误[20304]";                ///< 协议错误
        case ERR_PACK_INFO_ERROR:                       return "数据错误[20305]";                ///< 部分信息打成PB协议包
        case ERR_PARSE_PACK_ERROR:                      return "协议错误[20306]";                ///< 解析PB协议包
        case ERR_REQ_MISS_PB_PARAM:                     return "参数缺失[20307]";             ///< 请求缺少参数(pb中带的参数不全)
        case ERR_PROTOCOL_FORMAT:                       return "协议格式错误[20308]";             ///< 协议格式错误
        case ERR_LIST_INCOMPLETE:                       return "参数缺失[20309]";                 ///< 参数列表缺失或不全
        case ERR_BEYOND_RANGE:                          return "参数值错误[20310]";               ///< 传入的参数值超过规定范围
        case ERR_PARMS_VALUES_MISSING:                  return "参数缺失[20311]";                  ///< 传入的参数在程序传递或解析过程中丢失
        case ERR_JSON_PRASE_FAILED:                     return "协议格式错误[20312]";              ///< JSON 数据解析失败
        case ERR_INVALID_DATA:                          return "数据错误[20313]";                 ///< 错误数据
        case ERR_INVALID_SESSION_ID:                    return "协议错误[20314]";                 ///< 错误的session路由信息
        case ERR_INVALID_PARMS:                         return "无效参数[20315]";                 ///< 无效参数


        case ERR_USER_OFFLINE:                          return "用户不在线"; ///< 用户处于离线状态
        case ERR_USER_KICED_OFFLINE:                    return "您的账号在其他设备登录，请重新登录";//你的帐号已经别处登录，您被迫下线
        case ERR_PRELOGIN_QRY_IDENTITY:                 return "系统繁忙，请稍后再尝试";///< 预登录从center查询ip,port出错
        case ERR_QUERY_UINFO_FROM_BUSINESS:             return "系统繁忙，请稍后再尝试";//从业务查询用户信息失败
        case ERR_CHECK_PRE_LOGIN_TOKEN:                     return "身份信息错误，请重新登录";
        case ERR_DEVICE_KICED_OFFLINE:                  return "您的账号在其他设备登录，请重新登录!";
        case ERR_USER_NOT_EXIST:                        return "用户不存在";
        case ERR_USER_INFO_REGISTER:                    return "用户信息注册失败";
        case ERR_USER_ALREADY_EXIST:                    return "用户已经存在";
        case ERR_MOBILE_ALREADY_EXIST:                  return "电话已经存在";
        case ERR_NICKNAME_ALREADY_EXIST:                return "昵称已经存在";
        case ERR_MAIL_ALREADY_EXIST:                    return "邮件已经存在";

        case ERR_DATA_LOAD_USER_INFO:                   return "系统繁忙，请稍后再尝试[21015]";    ///< 加载用户信息失败
        case ERR_DATA_LOAD_USER_ONLINE_STATUS:          return "系统繁忙，请稍后再尝试[21016]";    ///< 加载用户状态失败
        case ERR_USER_PASSWORD:                         return "用户密码错误[21017]";///用户密码错误
        case ERR_USER_CONFIRM_PASSWORD:                 return "用户确认密码不同[21018]";//用户确认密码不同
        case ERR_USER_PASSWORD_LENGTH:                  return "用户密码长度不够[21019]";//用户密码长度不够
		case ERR_DATA_LOAD_RECETION_SESSION:            return "系统繁忙，请稍后再尝试[21020]";    ///< 加载客服接待会话失败
		case ERR_REGISTER_USER_TYPE_MAX_NUM:            return "注册指定用户类型数量超过最大限制数量[21021]"; ///注册指定用户类型数量超过最大限制数量
		case ERR_REGISTERING_USER_NOW:                  return "该用户正在创建中[21022]";//该用户正在创建中

        case ERR_DATA_NAME_REPETED:                     return "命名数据重复[22001]"; ///< 命名数据重复

        case ERR_SENSITIVE_WORDS_EXIST:                 return "敏感词已存在[27001]";//敏感词已存在
        case ERR_SENSITIVE_WORDS_NOT_EXIST:             return "敏感词不存在[27002]";//敏感词不存在
        case ERR_SENSITIVE_LOADDING:                    return "等待敏感词加载[27003]";//等待敏感词加载
        case ERR_NO_CUSTOMER_SERVICE_ONLINE:            return "当前没有客服在线[27004]";//当前没有客服在线
        case ERR_INVALID_SESSION:                       return "无效会话[27005]";//无效会话
        case ERR_NO_OPERATION_PERMISSIONS:              return "没有操作权限[27006]"; ///< 没有操作权限

        case ERR_SERVER_CONFIG_EXIST:                   return "相同更新配置已存在[28001]";//相同更新配置已存在
        case ERR_SERVER_NODE_NO_EXIST:                  return "不存在该节点[28002]";//不存在该节点
        case ERR_SERVER_NODE_ALREADY_OFFLINE:           return "该节点已下线[28003]";   ///该节点已下线
        case ERR_SERVER_NODE_ALREADY_ONLINE:            return "该节点已上线[28004]";   ///该节点已上线
        case ERR_SERVER_CENTER_RESTART_SCRIPT:          return "中心节点使用脚本关闭或重启节点[28008]";//中心节点使用脚本关闭或重启节点
        case ERR_SERVER_CENTER_NO_SUSPEND:              return "中心节点不会被挂起路由[28009]";//中心节点不会被挂起路由
        case ERR_SERVER_CENTER_NO_ROUTES_RESTORE:       return "中心节点不需要恢复路由[28010]";//中心节点不需要恢复路由
        case ERR_SERVER_LOGIC_CONFIG_NONE_RELOAD:       return "逻辑配置无需重新加载库[28011]";//逻辑配置无需重新加载库
        case ERR_SERVER_LOGIC_CONFIG_RELOAD_FAIL:       return "逻辑配置重新加载库失败[28012]";//逻辑配置重新加载库失败
        case ERR_SERVER_CENTER_NO_OPERATION:            return "中心节点不需要操作[28013]";   ///中心节点不需要操作
        case ERR_SERVER_CENTER_OPERATION_NO_TARGET:     return "中心节点操作需要指定操作节点[28014]";   ///中心节点操作需要指定操作节点
        case ERR_SERVER_NO_SUCH_ONLINE_NODE:            return "没有这个在线节点[28015]";   ///没有这个在线节点
        case ERR_SERVER_NODE_OFFLINE_NEED_MORE_NODES:   return "在线节点挂起需要更多的在线节点[28016]";//在线节点挂起需要更多的在线节点

        case ERR_SERVER_NODE_PRELOGIN_NO_GATE:          return "没有合适网关[29001]";//没有合适网关
  }

    if (code < 10000)
    {
        return "系统错误";
    }
    else if (code >= 10000 && code < 20000)  // 此种情况应在上面的switch里处理
    {
        return "服务器错误";
    }
    else if (code >= 20000 && code < 30000)  // 此种情况应在上面的switch里处理
    {
        return "逻辑错误";
    }
    return "未知错误";
}

}

#endif /* SRC_IMERRORMAPPING_H_ */
