/*******************************************************************************
* Project:  proto
* @file     ImCw.h
* @brief    IM业务命令字定义
* @author   lbh
* @date:    2019年10月12日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_IMCW_H_
#define SRC_IMCW_H_

namespace im
{

/**
 * @brief IM业务命令字定义
 * @note IM业务命令字成对出现，从1001开始编号，并且遵从奇数表示请求命令字，
 * 偶数表示应答命令字，应答命令字 = 请求命令字 + 1
 */
enum E_IM_CW
{
	CMD_UNDEFINE                        = 0,        ///< 未定义
	CMD_REQ_SYS_ERROR					= 999,		///< 系统错误请求（无意义，不会被使用）
	CMD_RSQ_SYS_ERROR					= 1000,		///< 系统错误响应

	// 用户相关命令字，如用户注册、用户登录、修改用户资料等，号段：1001~2000
	CMD_REQ_USER_LOGIN                  = 1001,     ///< 用户登录请求
	CMD_RSP_USER_LOGIN                  = 1002,     ///< 用户登录应答
    CMD_REQ_USER_LOGOUT                 = 1003,     ///< 用户退出请求
    CMD_RSP_USER_LOGOUT                 = 1004,     ///< 
    CMD_RSP_USER_KICED_OFFLINE          = 1005,     ///< 用户被迫下线通知
	// 用户关系命令字，如关注、取消关注、添加好友、删除好友、拉黑、举报等，号段：2001~3000
	
	CMD_REQ_USER_ATTENTION              = 2001,     ///< C2S用户关注（2001/2002）
    CMD_RSP_USER_ATTENTION              = 2002,
    CMD_REQ_USER_CANCEL_ATTENTION       = 2003,     ///<C2S用户取消关注（2003/2004）
    CMD_RSP_USER_CANCEL_ATTENTION       = 2004,     ///<C2S用户取消关注（2003/2004）


	CMD_REQ_FRIEND_ATTENTION_NTF        = 2019,     ///<S2C关注通知(2019/2020)
	CMD_RSP_FRIEND_ATTENTION_NOTICE_NTF = 2020,     ///<S2C关注通知应答(2019/2020)

    CMD_REQ_FRIENDS_ADD_DEL_NTF         = 2021,     ///<S2C好友关系操作通知（2021/2022）
    CMD_RSP_FRIENDS_ADD_DEL_NTF         = 2022,     ///<S2C好友关系操作通知（2021/2022）
	CMD_REQ_ATTENTION_AUTO				= 2025,		///<通讯录好友自动关注请求
	CMD_RSP_ATTENTION_AUTO				= 2026,		///<通讯录好友自动关注响应
	CMD_REQ_QUERY_SERVICE_INFO			= 2027,		///<钱小二基本信息查询请求
	CMD_RSP_QUERY_SERVICE_INFO			= 2027,		///<钱小二基本信息查询请求
	CMD_REQ_ADDR_BOOK_FRIEND			= 2031,		///<通讯录好友控制请求
	CMD_RSP_ADDR_BOOK_FRIEND			= 2032,		///<通讯录好友控制响应
	

	// 群管理命令字，如申请入群、退群、踢人、邀请加群等，号段：3001~4000
	CMD_REQ_GROUP_APPLY_NTF             = 3027,     ///< S2C申请入群管理员审核通知
	CMD_RSP_GROUP_APPLY_NTF             = 3028,     ///< S2C申请入群管理员审核通知应答
	CMD_REQ_GROUP_INVIT_NTF             = 3029,     ///< S2C邀请入群管理员审核通知
	CMD_RSP_GROUP_INVIT_NTF             = 3030,     ///< S2C邀请入群管理员审核通知应答
	CMD_REQ_GROUP_ADDDEL_NTF            = 3031,     ///< S2C群成员变更通知
	CMD_RSP_GROUP_ADDDEL_NTF            = 3032,     ///< S2C群成员变更通知应答
	CMD_REQ_GROUP_MEMBER_CHANGE_NTF     = 3031,     ///< S2C群成员变更通知
	CMD_RSP_GROUP_MEMBER_CHANGE_NTF		= 3032,     ///< S2C群成员变更通知应答
	CMD_REQ_GROUP_NTF                   = 3033,     ///< S2C群通知
	CMD_RSP_GROUP_NTF                   = 3034,     ///< S2C群通知应答
	CMD_REQ_GROUP_VERIFY_NTF            = 3035,     ///< S2C入群审核结果通知
	CMD_RSP_GROUP_VERIFY_NTF            = 3036,     ///< S2C入群审核结果通知应答
	CMD_REQ_GROUP_UPDATE_NTF            = 3037,     ///< S2C群信息更新通知
	CMD_RSP_GROUP_UPDATE_NTF            = 3038,     ///< S2C群信息更新通知应答
	CMD_REQ_GROUP_REMOVE_USER_NTF       = 3041,     ///< S2C用户被踢出群个人通知
	CMD_RSP_GROUP_REMOVE_USER_NTF       = 3042,     ///< S2C用户被踢出群个人通知

	// 聊天相关命令字，如单聊、群聊、屏蔽消息等，号段：4001~5000
	CMD_REQ_P2P_MSG_SEND		        = 4001,     ///< 发送单聊消息
	CMD_RSP_P2P_MSG_SEND_ACK	        = 4002,     ///< 发送单聊应答
	CMD_REQ_P2P_MSG_RECV		        = 4003,     ///< 接收单聊消息
	CMD_RSP_P2P_MSG_RECV_ACK	        = 4004,     ///< 接收单聊应答

	CMD_REQ_GROUP_MSG_SEND = 4005,     //< 发送群聊消息（客户端->服务器）
	CMD_RSP_GROUP_MSG_SEND_ACK = 4006,     //< 发送群聊消息回执
	CMD_REQ_GROUP_MSG_RECV = 4007,     //< 接收群聊消息(服务器->客户端)
	CMD_RSP_GROUP_MSG_RECV_ACK = 4008,     //< 接收群聊消息回执

	CMD_REQ_P2P_MSG_OFFLINE = 4009,     ///< 请求单聊离线消息
	CMD_RSP_P2P_MSG_OFFLINE_ACK = 4010,     ///< 请求单聊离线消息应答
	CMD_REQ_P2P_MSG_OFFLINE_PUSH = 4013,     ///< 单聊离线消息推送
	CMD_RSQ_P2P_MSG_OFFLINE_PUSH_ACK = 4014,     ///< 单聊离线消息推送应答

	CMD_REQ_GROUP_MSG_OFFLINE = 4011,     ///< 请求群聊离线消息
	CMD_RSP_GROUP_MSG_OFFLINE_ACK = 4012,     ///< 请求群聊离线消息应答
	CMD_REQ_GROUP_MSG_OFFLINE_PUSH = 4015,     ///< 群聊离线消息推送
	CMD_RSQ_GROUP_MSG_OFFLINE_PUSH_ACK = 4016,     ///< 群聊离线消息推送应答

    // IM管理相关命令字，如禁言、禁止登录、封号等，号段：5001~6000

    // 社交相关命令字，如分享、发表说说、评论、回复评论、点赞等，号段：6001~7000
	CMD_REQ_IM_MESSAGE_RED_DOT_SPEAK_NTF = 6001,     ///< 系统推送红点通知
	CMD_RSQ_IM_MESSAGE_RED_DOT_SPEAK_NTF_ACK = 6002,     ///< 红点推送通知应答
	CMD_REQ_IM_MESSAGE_SPEAK_PERSONAL_NTF = 6003,     ///< 系统推送与说说相关的提醒(评论回复/点赞/分享)通知
	CMD_REQ_IM_MESSAGE_SPEAK_PERSONAL_NTF_ACK = 6004,     ///< 与说说相关的提醒(评论回复/点赞/分享)通知推送应答

	// 公众号
	CMD_REQ_IM_OFFICIAL_SEND_MSG = 64101,     ///< 内部发送公从号消息到Logic
	CMD_REQ_IM_OFFICIAL_SEND_MSG_ACK = 64102,     ///< 响应

	CMD_REQ_USER_BEHAVIOR_STATISTICAL = 64401,  ///< 用户行为数据统计请求
	CMD_RSP_USER_BEHAVIOR_STATISTICAL = 64402,  ///< 用户行为数据统计响应

	//创建业务群
	CMD_REQ_IM_CREATE_PROJECT_GROUP = 10001,//创建项目群
	CMD_REQ_IM_CREATE_PROJECT_GROUP_ACK = 10002,//响应

	//客服
	CMD_REQ_IM_MESSAGE_CUSTON_TRANSFER_NTF = 10003,//客服移交
	CMD_REQ_IM_MESSAGE_CUSTON_TRANSFER_NTF_ACK = 10004,//客服移交响应

	CMD_REQ_PUSH_INTERFACE_PUSH = 7001,//外部实时消息推送接口
	CMD_RSQ_PUSH_INTERFACE_PUSH = 7002,//外部实时消息推送接口响应

	CMD_REQ_IM_OFFICIAL_RECV_MSG = 7003,     ///< 发送公从号消息到客户端
	CMD_REQ_IM_OFFICIAL_RECV_MSG_ACK = 7004,     ///< 响应
	CMD_REQ_PUSH_TOKEN                  = 7005,         ///< 推送token
	CMD_RSP_PUSH_TOKEN                  = 7006,         ///< 推送token响应

	//用户和群的地理信息等，号段：8001~9000
    CMD_REQ_IM_UPDATE_USER_GROUP_COORDINATE = 8001,     ///< 上传当前用户或者自己群的坐标
    CMD_RSQ_IM_UPDATE_USER_GROUP_COORDINATE_ACK = 8002,     ///< 上传当前用户或者自己群的坐标应答
    CMD_REQ_IM_UPDATE_USER_DISTANCE = 8003,     ///< 取得当前用户与群或者人的距离
    CMD_RSQ_IM_UPDATE_USER_DISTANCE_ACK = 8004,     ///< 取得当前用户与群或者人的距离应答
    CMD_REQ_IM_NEAR_USER = 8005,     ///< 查找附近的人
    CMD_RSQ_IM_NEAR_USER_ACK = 8006,     ///< 查找附近的人应答
    CMD_REQ_IM_NEAR_GROUP = 8007,     ///< 查找附近的群
    CMD_RSQ_IM_NEAR_GROUP_ACK = 8008,     ///< 查找附近的群应答
    CMD_REQ_IM_DELETE_GROUP_COORDINATE = 8009,     ///< 删除群坐标
    CMD_RSQ_IM_DELETE_GROUP_COORDINATE_ACK = 8010,     ///< 删除群坐标回应
    CMD_REQ_IM_DELETE_USER_COORDINATE = 8011,     ///< 删除用户坐标
    CMD_RSQ_IM_DELETE_USER_COORDINATE_ACK = 8012,     ///< 删除用户坐标回应

    //敏感词
    CMD_REQ_CHECK_SENSITIVE_WORDS = 10005,     ///< 检查敏感词
    CMD_RSQ_CHECK_SENSITIVE_WORDS_ACK = 10006,     ///< 检查敏感词响应

    CMD_REQ_UPDATE_SENSITIVE_WORDS = 10007,     ///< 更新敏感词
    CMD_RSQ_UPDATE_SENSITIVE_WORDS_ACK = 10008,     ///< 更新敏感词响应

    //S2S用户注册协议
    CMD_RSQ_IM_REGISTER_USER_INFO = 64201,//用户注册协议
    CMD_RSQ_IM_REGISTER_USER_INFO_ACK = 64202,//用户注册协议回应
    CMD_RSQ_IM_UPDATE_USER_INFO = 64203,//用户信息更新协议
    CMD_RSQ_IM_UPDATE_USER_INFO_ACK = 64204,//用户信息更新协议回应

};

}   // end of namespace im

#endif /* SRC_IMCW_H_ */
