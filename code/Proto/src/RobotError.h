/*******************************************************************************
* Project:  proto
* @file     RobotError.h
* @brief    系统错误定义
* @author   cjy
* @date:    2015年10月12日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_IMERROR_H_
#define SRC_IMERROR_H_

namespace robot
{

/**
 * @brief 系统错误码定义
 * @note 系统错误码从20001开始编号
 */
enum E_ERROR_NO
{
    /////////////////////////////////<错误代码按功能协议划分:

    ERR_OK                                   = 0,        ///< 正确

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

    ///<21000~21999	用户相关命令字，如用户注册、用户登录、修改用户资料等	用户行为
    ERR_SERVER_AUTHCODE            	         = 21001,    ///< 验证码错误，请重新登录
    ERR_USER_OFFLINE                         = 21002,    ///< 用户处于离线状态
    ERR_USER_KICED_OFFLINE                   = 21003,    ///< 你的帐号已经别处登录，您被迫下线
    ERR_PRELOGIN_QRY_IDENTITY                = 21005,    ///< 预登录从查询ip,port出错
    ERR_QUERY_UINFO_FROM_BUSINESS            = 21006,    ///< 从业务查询用户信息失败
    ERR_CHECK_LOGIN_TOKEN                    = 21007,    ///< 校验登录token失败
    ERR_DEVICE_KICED_OFFLINE                 = 21008,    ///< 你的帐号已经在同一时间，同一设备多次登录，您被迫下线!
    ERR_USER_NOT_EXIST                       = 21009,    ///< 用户不存在
    ERR_USER_INFO_REGISTER                   = 21010,    ///< 用户信息注册失败
    ERR_USER_ALREADY_EXIST                   = 21011,    ///< 用户已经存在
    ERR_MOBILE_ALREADY_EXIST                 = 21012,    ///< 电话已经存在
    ERR_NICKNAME_ALREADY_EXIST               = 21013,    ///< 昵称已经存在
    ERR_MAIL_ALREADY_EXIST                   = 21014,    ///< 邮件已经存在
    ERR_DATA_LOAD_USER_INFO                  = 21015,    ///< 加载用户信息失败
    ERR_DATA_LOAD_USER_ONLINE_STATUS         = 21016,    ///< 加载用户状态失败
    ERR_USER_PASSWORD                        = 21017,    ///< 用户密码错误
    ERR_USER_CONFIRM_PASSWORD                = 21018,    ///< 用户确认密码不同
    ERR_USER_PASSWORD_LENGTH                 = 21019,    ///< 用户密码长度不够
	ERR_DATA_LOAD_RECETION_SESSION           = 21020,    ///< 加载客服接待会话失败
	ERR_REGISTER_USER_TYPE_MAX_NUM           = 21021,    ///< 注册指定用户类型数量超过最大限制数量
	ERR_REGISTERING_USER_NOW                 = 21022,    ///< 该用户正在创建中

    ///<22000~26999	聊天相关命令字，如单聊等
    ERR_DATA_NAME_REPETED                    = 22001,    ///< 命名数据重复

    ///<27000~27999	官方或第三方消息推送	消息推送
    ERR_SENSITIVE_WORDS_EXIST                = 27001,    ///< 敏感词已存在
    ERR_SENSITIVE_WORDS_NOT_EXIST            = 27002,    ///< 敏感词不存在
    ERR_SENSITIVE_LOADDING                   = 27003,    ///< 等待敏感词加载
    ERR_NO_CUSTOMER_SERVICE_ONLINE           = 27004,    ///< 当前没有在线客服
    ERR_INVALID_SESSION                      = 27005,    ///< 无效会话
    ERR_NO_OPERATION_PERMISSIONS             = 27006,    ///< 没有操作权限
    ERR_ALREADY_IN_SESSION                   = 27007,    ///< 已经在会话中
    ERR_CUSTOMER_SERVICE_TEAM_NO_MEMBER      = 27008,    ///< 客服组
    ERR_ALREADY_EVALUATED                    = 27009,    ///< 已经评价客服

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


}

#endif /* SRC_IMERROR_H_ */
