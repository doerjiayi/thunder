/*******************************************************************************
* Project:  Net
* @file     NetError.hpp
* @brief    系统错误定义
* @author   cjy
* @date:    2015年8月5日
* @note
* Modify history:
******************************************************************************/
#ifndef NETERROR_HPP_
#define NETERROR_HPP_

namespace net
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
    ERR_REDIS_CONN_CLOSE                = 10004,    ///< redis连接已关闭
    ERR_DISCONNECT                      = 10005,    ///< 已存在的连接发生错误
    ERR_NO_CALLBACK                     = 10006,    ///< 回调不存在或已超时
    ERR_DATA_TRANSFER                   = 10007,    ///< 数据传输出错
    ERR_REPEAT_REGISTERED               = 10008,    ///< 重复注册
    ERR_SERVERINFO                      = 10009,    ///< 服务器信息错误
    ERR_SESSION                         = 10010,    ///< 获取会话错误
    ERR_BODY_JSON                       = 10011,    ///< 消息体json解析错误
    ERR_SERVERINFO_RECORD               = 10012,    ///< 存档服务器信息错误
	ERR_TIMEOUT 						= 10013,    ///< 超时
};

}   // namespace net


#endif /* NETERROR_HPP_ */
