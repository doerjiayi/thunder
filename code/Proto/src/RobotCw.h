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

    CMD_REQ_ENTER_ROBOT_SESSION         = 4001,      //C2S客户请求进入机器人会话(4001/4002)
    CMD_REQ_ROBOT_SINGLE_MESSAGE        = 4003,      //C2S客户发送机器人单聊消息(4003/4004)
    CMD_REQ_GET_ROBOT_PRE_QUESTION_LIST = 4005,      //C2S用户获取智能前置问题列表（4005/4006）
    CMD_REQ_INQUERY_ROBOT_PRE_QUESTION  = 4007,      //C2S客户咨询智能前置问题(4007/4008)
    CMD_REQ_EDIT_ROBOT_PRE_QUESTION     = 4009,      //C2S客服编辑智能前置问题(4009/4010)

    CMD_REQ_ENTER_SESSION               = 5001,     ///< C2S客户请求进入人工会话
    CMD_RSP_ENTER_SESSION               = 5002,     ///<
    CMD_REQ_ENTER_SESSION_NOTIFY        = 5003,     ///< S2C客户进入人工会话通知
    CMD_RSP_ENTER_SESSION_NOTIFY        = 5004,     ///<
    CMD_REQ_SWITCH_ONLINE               = 5005,     ///< 客服切换在线状态
    CMD_RSP_SWITCH_ONLINE               = 5006,     ///<

    CMD_REQ_CLOSE_SESSION               = 5013,     ///< 客服结束会话
    CMD_RSP_CLOSE_SESSION               = 5014,
    CMD_REQ_CLOSE_SESSION_NOTIFY        = 5015,     ///< 客服结束会话向客户端发送通知
    CMD_RSP_CLOSE_SESSION_NOTIFY        = 5016,

    CMD_REQ_SEND_CHAT_MSG               = 5025,     ///< C2S发送单聊请求
    CMD_RSP_SEND_CHAT_MSG               = 5026,     ///<
    CMD_REQ_RECV_CHAT_MSG               = 5027,     ///< S2C接收单聊
    CMD_RSP_RECV_CHAT_MSG               = 5028,     ///<

    CMD_REQ_CUSTOMER_SERVICE_TRANSFER_NTF = 5029,   ///< 客服转接客服通知
    CMD_RSP_CUSTOMER_SERVICE_TRANSFER_NTF = 5030,
    CMD_REQ_PUSH_SESSION_QUEUE_LEN      = 5031,     ///< 服务端向用户推送客服排队会话人数
    CMD_RSP_PUSH_SESSION_QUEUE_LEN      = 5032,

    CMD_REQ_PUSH_HISTORY_SESSION_MESSAGES = 6005,     //S2C服务器推送最近会话历史消息(6005)

    CMD_REQ_PUSH_FILE_SERVER_CONFIG     = 25001,//S2C推送文件服务器的配置

    CMD_REQ_EDIT_AI_QUESTION            = 11005
};

}   // end of namespace robot

#endif /* SRC_IMCW_H_ */
