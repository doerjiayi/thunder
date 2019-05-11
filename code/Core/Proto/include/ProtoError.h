/*******************************************************************************
* Project:  proto
* @file     RobotError.h
* @brief    系统错误定义
* @author   cjy
* @date:    2015年10月12日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_ANALYSISERROR_H_
#define SRC_ANALYSISERROR_H_
#include "NetError.hpp"

namespace core
{

/**
 * @brief 业务命令字定义
 * @note 业务命令字成对出现，从1001开始编号，并且遵从奇数表示请求命令字，
 * 偶数表示应答命令字，应答命令字 = 请求命令字 + 1
 */
enum E_SYS_CW
{
	CMD_UNDEFINE                        = 0,        ///< 未定义
	CMD_REQ_SYS_ERROR					= 999,		///< 系统错误请求（无意义，不会被使用）
	CMD_RSQ_SYS_ERROR					= 1000,		///< 系统错误响应

};

/**
 * @brief 系统错误码定义
 * @note 系统错误码从20001开始编号
 */
enum E_ERROR_NO
{
    /////////////////////////////////<错误代码按功能协议划分:

    ERR_OK                                   = 0,        ///< 正确

    //返回客户端错误码
    ERR_RESPONSE_OK                          = 200,    ///< 回应客户端正确
    ERR_INVALID_PARAMS                       = 400,    ///< 请求参数有误
    ERR_APPKEY_AUTHCODE                      = 403,    ///< appkey验证错误
    ERR_SERVER_LOGIC_ERROR                   = 500,    ///< server逻辑错误

	/* 存储代理错误码段  11000~11999 */
	ERR_INCOMPLET_DATAPROXY_DATA        = 11001,    ///< DataProxy请求数据包不完整
	ERR_INVALID_REDIS_ROUTE             = 11002,    ///< 无效的redis路由信息
	ERR_REDIS_NODE_NOT_FOUND            = 11003,    ///< 未找到合适的redis节点
	ERR_REGISTERCALLBACK_REDIS          = 11004,    ///< 注册RedisStep错误
	ERR_REDIS_CMD                       = 11005,    ///< redis命令执行出错
	ERR_UNEXPECTED_REDIS_REPLY          = 11006,    ///< 不符合预期的redis结果
	ERR_RESULTSET_EXCEED                = 11007,    ///< 数据包超过protobuf最大限制
	ERR_LACK_CLUSTER_INFO               = 11008,    ///< 缺少集群信息
	ERR_REDIS_AND_DB_CMD_NOT_MATCH      = 11010,    ///< redis读写操作与DB读写操作不匹配
	ERR_REDIS_NIL_AND_DB_FAILED         = 11011,    ///< redis结果集为空，但发送DB操作失败
	ERR_NO_RIGHT                        = 11012,    ///< 数据操作权限不足
	ERR_QUERY                           = 11013,    ///< 查询出错，如拼写SQL错误
	ERR_REDIS_STRUCTURE_WITH_DATASET    = 11014,    ///< redis数据结构由DB的各字段值序列化（或串联）而成，请求与存储不符
	ERR_REDIS_STRUCTURE_WITHOUT_DATASET = 11015,    ///< redis数据结构并非由DB的各字段值序列化（或串联）而成，请求与存储不符
	ERR_DB_FIELD_NUM                    = 11016,    ///< redis数据结构由DB的各字段值序列化（或串联）而成，请求的字段数量错误
	ERR_DB_FIELD_ORDER_OR_FIELD_NAME    = 11017,    ///< redis数据结构由DB的各字段值序列化（或串联）而成，请求字段顺序或字段名错误
	ERR_KEY_FIELD                       = 11018,    ///< redis数据结构由DB的各字段值序列化（或串联）而成，指定的key_field错误或未指定，或未在对应表的数据字典中找到该字段
	ERR_KEY_FIELD_VALUE                 = 11019,    ///< redis数据结构指定的key_field所对应的值缺失或值为空
	ERR_JOIN_FIELDS                     = 11020,    ///< redis数据结构由DB字段串联而成，串联的字段错误
	ERR_LACK_JOIN_FIELDS                = 11021,    ///< redis数据结构由DB字段串联而成，缺失串联字段
	ERR_REDIS_STRUCTURE_NOT_DEFINE      = 11022,    ///< redis数据结构未在DataProxy的配置中定义
	ERR_INVALID_CMD_FOR_HASH_DATASET    = 11023,    ///< nosql hash数据结构由DB的各字段值序列化（或串联）而成，而请求中的hash命令不当
	ERR_DB_TABLE_NOT_DEFINE             = 11024,    ///< 表未在DataProxy的配置中定义
	ERR_DB_OPERATE_MISSING              = 11025,    ///< redis数据结构存在对应的DB表，但数据请求缺失对数据库表操作
	ERR_LACK_TABLE_FIELD                = 11027,    ///< 数据库表字段缺失
	ERR_TABLE_FIELD_NAME_EMPTY          = 11028,    ///< 数据库表字段名为空
	ERR_UNDEFINE_REDIS_OPERATE          = 11029,    ///< 未定义或不支持的redis数据操作（只支持string和hash的dataset update操作）
	ERR_REDIS_READ_WRITE_CMD_NOT_MATCH  = 11030,    ///< redis读命令与写命令不对称


    ///<20000-30000 模块公共错误代码
    // 20000~20199 基础错误码
    ERR_PARASE_PROTOBUF                 = 20000,    ///< 解析Protobuf出错
    ERR_NO_SUCH_WORKER_INDEX            = 20001,    ///< 未知的Worker进程编号
    ERR_UNKNOWN_CMD                     = 20002,    ///< 未知命令字
    ERR_NEW                             = 20003,    ///< new出错（无法分配堆内存）
    ERR_DISCONNECT                      = 20005,    ///< 已存在的连接发生错误
    ERR_NO_CALLBACK                     = 20006,    ///< 回调不存在或已超时
    ERR_DATA_TRANSFER                   = 20007,    ///< 数据传输出错
    ERR_REPEAT_REGISTERED               = 20008,    ///< 重复注册
    ERR_SERVERINFO                      = 20009,    ///< 服务器信息错误
    ERR_BODY_JSON                       = 20011,    ///< 消息体json解析错误
    ERR_SERVERINFO_RECORD               = 20012,    ///< 存档服务器信息错误
    ERR_TIMEOUT                         = 20013,    ///< 超时
    ERR_REG_STEP_ERROR                  = 20018,    ///< 注册Step失败
    ERR_STEP_EMIT_LIMIT                 = 20019,    ///< STEP emit 次数超过限制

    ///<20200-20299 系统类错误代码
    ERR_UNKNOWN                              = 20200,    ///< 未知错误
    ERR_UNKNOW_CMD                           = 20201,    ///< 未知命令字
    ERR_SYSTEM_ERROR                         = 20202,    ///< 系统错误（数据库错误等，通过查询后台Server日志可以找到更具体错误码）
    ERR_SERVER_BUSY             	         = 20203,    ///< 服务器繁忙
    ERR_SERVER_ERROR			             = 20204,    ///< 服务器错误（给客户端的错误响应）
    ERR_SERVER_TIMEOUT                       = 20205,    ///< 服务器处理超时
    ERR_UNKNOWN_RESPONSE_CMD                 = 20207,    ///< 未知响应命令字，通常为对端没有找到对应Cmd处理者，由框架层返回错误
    ERR_ASYNC_TIMEOUT                        = 20208,    ///< 异步操作超时
    ERR_LOAD_CONFIGFILE		                 = 20209,    ///< 模块加载配置文件失败
    ERR_LOGIC_SERVER_TIMEOUT                 = 20213,    ///< 逻辑Server处理超时
    ERR_PROC_RELAY                           = 20214,    ///< 数据转发到其它进程失败
    ERR_SEND_TO_LOGIG_MSG                    = 20215,    ///< 发送消息到逻辑服务器失败
    ERR_REG_SESSION                          = 20216,    ///< session注册失败
    ERR_SESSION_CREATE                       = 20217,    ///< 创建 session 失败
    ERR_SESSION                              = 20218,    ///< 获取 session 失败
    ERR_MALLOC_FAILED		                 = 20219,    ///< new对象失败
    ERR_SEQUENCE                             = 20220,    ///< 错误序列号
    ERR_RC5ENCRYPT_ERROR		             = 20221,    ///< Rc5Encrypt 加密失败
    ERR_RC5DECRYPT_ERROR		             = 20222,    ///< Rc5Decrypt 解密失败
    ERR_REQ_FREQUENCY                        = 20223,    ///< 请求过于频繁（未避免被攻击而做的系统保护）
    ERR_REQ_MISS_PARAM                       = 20224,    ///< 请求参数缺失

    ///<20300-20399 协议类错误代码
    ERR_NO_SESSION_ID_IN_MSGBODY             = 20301,    ///< MsgBody缺少session_id
    ERR_NO_ADDITIONAL_IN_MSGBODY	         = 20302,    ///< MsgBody缺少additional
    ERR_MSG_BODY_DECODE		                 = 20303,    ///< msg body解析出错
    ERR_INVALID_PROTOCOL                     = 20304,    ///< 协议错误
    ERR_PACK_INFO_ERROR			             = 20305,    ///< 部分信息打成PB协议包
    ERR_PARSE_PACK_ERROR		             = 20306,    ///< 解析PB协议包
    ERR_REQ_MISS_PB_PARAM		             = 20307,    ///< 请求缺少参数(pb中带的参数不全)
    ERR_PROTOCOL_FORMAT                      = 20308,    ///< 协议格式错误
    ERR_LIST_INCOMPLETE 		             = 20309,    ///< 参数列表缺失或不全
    ERR_BEYOND_RANGE		                 = 20310,    ///< 传入的参数值超过规定范围
    ERR_PARMS_VALUES_MISSING	             = 20311,    ///< 传入的参数在程序传递或解析过程中丢失
    ERR_JSON_PRASE_FAILED		             = 20312,    ///< JSON 数据解析失败
    ERR_INVALID_DATA                         = 20313,    ///< 错误数据
    ERR_INVALID_SESSION_ID                   = 20314,    ///< 错误的session路由信息
    ERR_INVALID_PARMS                        = 20315,    ///< 无效参数

    ///<27000~27999 官方或第三方消息推送  消息推送
    ERR_INVALID_SESSION                      = 27005,    ///< 无效会话
    ERR_NO_OPERATION_PERMISSIONS             = 27006,    ///< 没有操作权限

    ///<28000~28999 服务器维护
    ERR_SERVER_CONFIG_EXIST                  =  28001,   ///< 相同更新配置已存在
    ERR_SERVER_NODE_NO_EXIST                 =  28002,   ///不存在该节点
    ERR_SERVER_NODE_ALREADY_OFFLINE          =  28003,   ///该节点已下线
    ERR_SERVER_NODE_ALREADY_ONLINE           =  28004,   ///该节点已上线
    ERR_SERVER_NODE_NOT_MASTER               =  28005,   ///不是主节点
    ERR_SERVER_SELF_ONLINE                   =  28006,   ///上线自己
    ERR_SERVER_SELF_OFFLINE                  =  28007,   ///下线自己
    ERR_SERVER_CENTER_RESTART_SCRIPT         =  28008,   ///中心节点使用脚本关闭或重启节点
    ERR_SERVER_CENTER_NO_SUSPEND             =  28009,   ///中心节点不会被挂起路由
    ERR_SERVER_CENTER_NO_ROUTES_RESTORE      =  28010,   ///中心节点不需要恢复路由
    ERR_SERVER_LOGIC_CONFIG_NONE_RELOAD      =  28011,   ///逻辑配置无需重新加载库
    ERR_SERVER_LOGIC_CONFIG_RELOAD_FAIL      =  28012,   ///逻辑配置重新加载库失败
    ERR_SERVER_CENTER_NO_OPERATION           =  28013,   ///中心节点不需要操作
    ERR_SERVER_CENTER_OPERATION_NO_TARGET    =  28014,   ///中心节点操作需要指定操作节点
    ERR_SERVER_NO_SUCH_ONLINE_NODE           =  28015,   ///没有这个在线节点
    ERR_SERVER_NODE_OFFLINE_NEED_MORE_NODES  =  28016,   ///在线节点挂起需要更多的在线节点

    ERR_SERVER_NODE_PRELOGIN_NO_GATE         =  29001,   ///没有合适网关
};

//ERROR MAPPING
inline int http_err_code(int code)
{
    switch (code)
    {
        case ERR_OK:                                      return ERR_RESPONSE_OK;
        case ERR_RESPONSE_OK:                             return ERR_RESPONSE_OK;
        case ERR_INVALID_PARAMS:                          return ERR_INVALID_PARAMS;
        case ERR_SERVER_LOGIC_ERROR:                      return ERR_SERVER_LOGIC_ERROR;
    }
    return ERR_SERVER_LOGIC_ERROR;//系统错误
}

inline const char * http_err_msg(int code)
{
    switch (code)
    {
        case ERR_OK:                                      return "ok";
        case ERR_RESPONSE_OK:                             return "ok";
        case ERR_INVALID_PARAMS:                          return "invalid parameters";
        case ERR_APPKEY_AUTHCODE:                         return "failed to auth app key";
        case ERR_SERVER_LOGIC_ERROR:                      return "logic error";
    }
    return "logic error";
}

inline int server_err_code(int code)
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



inline const char * server_err_msg(int code)
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

#endif /* SRC_IMERROR_H_ */
