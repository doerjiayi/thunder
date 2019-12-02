/*******************************************************************************
* Project:  proto
* @file     RobotCw.h
* @brief    Robot业务命令字定义
* @author   cjy
* @date:    2015年10月12日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_IMCW_H_
#define SRC_IMCW_H_

namespace robot
{

/**
 * @brief Robot业务命令字定义
 * @note Robot业务命令字成对出现，从1001开始编号，并且遵从奇数表示请求命令字，
 * 偶数表示应答命令字，应答命令字 = 请求命令字 + 1
 */
enum E_ROBOT_CW
{
	CMD_UNDEFINE                        = 0,        ///< 未定义
	CMD_REQ_SYS_ERROR					= 999,		///< 系统错误请求（无意义，不会被使用）
	CMD_RSQ_SYS_ERROR					= 1000,		///< 系统错误响应

	// 用户注册更新(1001~2000)
	CMD_REQ_USER_REGISTER               = 1001,     ///< C2S用户请求注册账户（1001/1002）
	CMD_RSP_USER_REGISTER               = 1002,     ///<
    CMD_REQ_USER_UPDATE                 = 1003,     ///< C2S用户请求更新用户信息（1003/1004）
    CMD_RSP_USER_UPDATE                 = 1004,     ///<
    CMD_REQ_USER_UPDATE_CUSTOMER        = 1005,     ///< C2S客服修改用户信息请求（1005/1006）
    CMD_RSP_USER_UPDATE_CUSTOMER        = 1006,
	
	// 用户登录协议（2001~3000）
	CMD_REQ_USER_PRELOGIN    		    = 2001,     ///< C2S用户预登陆（2001/2002）
	CMD_RSP_USER_PRELOGIN        	    = 2002,     ///<
	CMD_REQ_USER_LOGIN      		    = 2003,     ///< C2S用户登录（2003/2004）
	CMD_RSP_USER_LOGIN          	    = 2004,     ///<
	CMD_REQ_USER_LOGOUT                 = 2005,     ///< C2S客户退出（2005/2006）
    CMD_RSP_USER_LOGOUT                 = 2006,     ///<
    CMD_REQ_USER_UNNORMAL_DEVICE        = 2007,     ///< C2S用户设备异常登录确认（2007/2008）
    CMD_RSP_USER_UNNORMAL_DEVICE        = 2008,     ///<
    CMD_REQ_KICK_USER                   = 2009,     ///< S2C用户被踢下线（2009）

    CMD_REQ_SEND_CHAT_MSG               = 5025,     ///< C2S发送单聊请求
    CMD_RSP_SEND_CHAT_MSG               = 5026,     ///<
    CMD_REQ_RECV_CHAT_MSG               = 5027,     ///< S2C接收单聊
    CMD_RSP_RECV_CHAT_MSG               = 5028,     ///<
};

}   // end of namespace robot

#endif /* SRC_IMCW_H_ */
