/*******************************************************************************
* Project:  proto
* @file     RedisData.hpp
* @brief    Redis数据结构定义
* @author   cjy
* @date:    2015年11月11日
* @note
* Modify history:
******************************************************************************/
#ifndef REDISDATA_HPP_
#define REDISDATA_HPP_

enum E_REDIS_TYPE
{
    REDIS_T_HASH                = 1,    ///< redis hash
    REDIS_T_SET                 = 2,    ///< redis set
    REDIS_T_KEYS                = 3,    ///< redis keys
    REDIS_T_STRING              = 4,    ///< redis string
    REDIS_T_LIST                = 5,    ///< redis list
    REDIS_T_SORT_SET            = 6,    ///< redis sort set
};

enum E_REDIS_ROBOT_DATA
{
    /*属性型数据 */
    IM_DATA_DOMAIN               = 1,    ///< app域名
    IM_DATA_ACCOUNT              = 2,    ///< account数据
    IM_DATA_CREATE_USERID        = 3,    ///< 创建账号，userid生成
    IM_DATA_USER_INFO            = 4,    ///< 用户基础属性数据，对应数据库中db_userinfo.tb_userinfo_xx
    IM_DATA_USER_ONLINE          = 5,    ///< 用户在线状态
    IM_DATA_MOBILE_USERID_INFO	 = 6,	 ///< 用户手机号与userid映射信息表
    IM_DATA_NICKNAME_USERID_INFO = 7,	 ///< 用户昵称与userid映射信息表
    IM_DATA_MAIL_USERID_INFO     = 8,    ///< 用户邮件与userid映射信息表
    IM_DATA_RECEPTION_COUNT      = 12,   ///< 在线客服当前正在接待客户数量统计
    IM_DATA_QUEUE_LEN            = 13,   ///< 在线客服当前排队客户数量统计
    IM_DATA_SESSION_QUEUE        = 14,   ///< 请求进入人工会话排队队列
    IM_DATA_CUSTOMER_SERVICE_STAGE = 15, ///< 客服状态
    IM_DATA_AI_PRE_QUESTION      = 16,   ///< 智能前置问题
    IM_DATA_AI_PRE_QUESTION_LIST = 17,   ///< 智能前置问题列表
    IM_DATA_CUSTOMER_SERVICE_SESSION_LIST = 18,///< 客服会话列表
    IM_DATA_AI_ROBOT_QUESTION_LIST = 19,///< 智能机器人引擎问题列表
    IM_DATA_SENTENCE_LIST        = 20,  ///< 人工词库列表
    IM_DATA_CUSTOMERSERVICE_APPLICATION = 21, ///< 应用客服
    IM_DATA_CUSTOMERSERVICE_APPLICATION_TEAM       = 22,///< 客服组
    IM_DATA_CUSTOMERSERVICE_TEAM       = 23,///< 客服组
    IM_DATA_CUSTOMERSERVICE_TEAM_MEMBER = 24, ///< 客服组成员
    IM_DATA_CUSTOMERSERVICE_BELONG_TEAM = 25, ///< 客服所在组
    IM_DATA_SESSION_QUALITY_OPTIONS_LIST = 26,  ///<会话质检选项列表
    IM_DATA_APP_INFO               = 27,    ///< app应用信息

    /*日志型数据*/
    IM_LOG_SINGLE_MSG_LIST      = 1002,  ///< 用户单聊消息列表
    IM_LOG_SENDER_SINGLE_MSG    = 1003,  ///< 发送者单聊消息结构(有失效时间)
    IM_LOG_SINGLE_MSG           = 1100,  ///< 单聊消息
    IM_LOG_SESSION_INFO         = 1101,  ///< 会话详情
    IM_LOG_USER_SESSION         = 1102,  ///< 用户会话
    IM_LOG_SESSION_MESSAGES_LIST= 1103,  ///< 会话消息列表
    IM_LOG_SESSION_QUALITY_INFO = 1104,  ///<质检会话详情

    IM_LOG_TODO_LEAVEMSG_LIST   = 1204,  ///< 待处理留言列表
    IM_LOG_DEAL_LEAVEMSG_LIST   = 1205,  ///< 处理过的留言列表

    IM_LOG_APP_CRM_TOKEN        = 1301,  ///<app crm token
    IM_LOG_APP_FILE_UNIQUE_ID   = 1401,  ///<唯一文件id映射文件url
};



#endif /* REDISDATA_HPP_ */
