/*******************************************************************************
* Project:  Thunder
* @file     OssError.hpp
* @brief    系统错误定义
* @author   cjy
* @date:    2017年8月5日
* @note
* Modify history:
******************************************************************************/
#ifndef OSSERROR_HPP_
#define OSSERROR_HPP_

namespace thunder
{

/**
 * @brief 系统错误码定义
 */
enum E_ERROR_NO
{
    ERR_OK                              = 0,        ///< 正确

    ///<10000-15000 框架错误
    ERR_PARASE_PROTOBUF                 = 10000,    ///< 解析Protobuf出错
    ERR_NO_SUCH_WORKER_INDEX            = 10001,    ///< 未知的Worker进程编号
    ERR_UNKNOWN_CMD                     = 10002,    ///< 未知命令字
    ERR_NEW                             = 10003,    ///< new出错（无法分配堆内存）
    ERR_DISCONNECT                      = 10005,    ///< 已存在的连接发生错误
    ERR_NO_CALLBACK                     = 10006,    ///< 回调不存在或已超时
    ERR_DATA_TRANSFER                   = 10007,    ///< 数据传输出错
    ERR_REPEAT_REGISTERED               = 10008,    ///< 重复注册
    ERR_SERVERINFO                      = 10009,    ///< 服务器信息错误
    ERR_SESSION                         = 10010,    ///< 获取会话错误
    ERR_BODY_JSON                       = 10011,    ///< 消息体json解析错误
    ERR_SERVERINFO_RECORD               = 10012,    ///< 存档服务器信息错误
    ERR_TIMEOUT                         = 10013,    ///< 超时
    ERR_REG_STEP_ERROR                  = 10018,    ///< 注册Step失败
    ERR_STEP_EMIT_LIMIT                 = 10019,    ///< STEP emit 次数超过限制


    ///<15000-15099 存储类错误代码(对应返回客户端的为系统错误)
    ERR_SENDUSESET_MSG                       = 15001,    ///< 发送存储userset失败
    ERR_STOREUSERSET_MSG                     = 15002,    ///< 存储userset 消息失败
    ERR_STOREUSERSET_MSGLIST                 = 15003,    ///< 存储userset消息列表失败
    ERR_LOGIC_NO_DATA                        = 15004,    ///< LOGIC 没有查询数据
    ERR_NO_DATA                              = 15005,    ///< dataproxy没有查询到数据
    ERR_OPERATOR_MONGO_ERROR                 = 15007,    ///< 操作芒果数据库失败
    ERR_DATA_PROXY_CALLBACK                  = 15008,    ///< data proxy 返回数据失败

    ERR_REDIS_AND_DB_CMD_NOT_MATCH           = 15009,    ///< redis读写操作与DB读写操作不匹配
    ERR_REDIS_NIL_AND_DB_FAILED              = 15010,    ///< redis结果集为空，但发送DB操作失败
    ERR_NO_RIGHT                             = 15011,    ///< 数据操作权限不足
    ERR_QUERY                                = 15012,    ///< 查询出错，如拼写SQL错误
    ERR_REDIS_STRUCTURE_WITH_DATASET         = 15013,    ///< redis数据结构由DB的各字段值序列化（或串联）而成，请求与存储不符
    ERR_REDIS_STRUCTURE_WITHOUT_DATASET      = 15014,    ///< redis数据结构并非由DB的各字段值序列化（或串联）而成，请求与存储不符
    ERR_DB_FIELD_NUM                         = 15015,    ///< redis数据结构由DB的各字段值序列化（或串联）而成，请求的字段数量错误
    ERR_DB_FIELD_ORDER_OR_FIELD_NAME         = 15016,    ///< redis数据结构由DB的各字段值序列化（或串联）而成，请求字段顺序或字段名错误
    ERR_KEY_FIELD                            = 15017,    ///< redis数据结构由DB的各字段值序列化（或串联）而成，指定的key_field错误或未指定，或未在对应表的数据字典中找到该字段
    ERR_KEY_FIELD_VALUE                      = 15018,    ///< redis数据结构指定的key_field所对应的值缺失或值为空
    ERR_JOIN_FIELDS                          = 15019,    ///< redis数据结构由DB字段串联而成，串联的字段错误
    ERR_LACK_JOIN_FIELDS                     = 15020,    ///< redis数据结构由DB字段串联而成，缺失串联字段
    ERR_REDIS_STRUCTURE_NOT_DEFINE           = 15021,    ///< redis数据结构未在DataProxy的配置中定义
    ERR_INVALID_CMD_FOR_HASH_DATASET         = 15022,    ///< redis hash数据结构由DB的各字段值序列化（或串联）而成，而请求中的hash命令不当
    ERR_DB_TABLE_NOT_DEFINE                  = 15023,    ///< 表未在DataProxy的配置中定义
    ERR_DB_OPERATE_MISSING                   = 15024,    ///< redis数据结构存在对应的DB表，但数据请求缺失对数据库表操作
    ERR_LACK_TABLE_FIELD                     = 15026,    ///< 数据库表字段缺失
    ERR_TABLE_FIELD_NAME_EMPTY               = 15027,    ///< 数据库表字段名为空
    ERR_UNDEFINE_REDIS_OPERATE               = 15028,    ///< 未定义或不支持的redis数据操作（只支持string和hash的dataset update操作）
    ERR_REDIS_READ_WRITE_CMD_NOT_MATCH       = 15029,    ///< redis读命令与写命令不对称

    ERR_INCOMPLET_DATAPROXY_DATA             = 15030,    ///< DataProxy请求数据包不完整
    ERR_INVALID_REDIS_ROUTE                  = 15031,    ///< 无效的redis路由信息
    ERR_REDIS_NODE_NOT_FOUND                 = 15032,    ///< 未找到合适的redis节点
    ERR_REGISTERCALLBACK_REDIS               = 15033,    ///< 注册RedisStep错误
    ERR_REDIS_CMD                            = 15034,    ///< redis命令执行出错
    ERR_UNEXPECTED_REDIS_REPLY               = 15035,    ///< 不符合预期的redis结果
    ERR_RESULTSET_EXCEED                     = 15036,    ///< 数据包超过protobuf最大限制
    ERR_LACK_CLUSTER_INFO                    = 15037,    ///< 缺少集群信息
    ERR_REDIS_CONN_CLOSE                     = 15038,    ///< redis连接已关闭

};

}   // namespace thunder


#endif /* OSSERROR_HPP_ */
