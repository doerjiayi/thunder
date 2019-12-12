/*******************************************************************************
 * Project:  Net
 * @file     RedisStep.hpp
 * @brief    带Redis的异步步骤基类
 * @author   cjy
 * @date:    2019年8月15日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_REDISSTEP_HPP_
#define SRC_STEP_REDISSTEP_HPP_
#include <set>
#include <list>
#include "dbi/redis/RedisCmd.hpp"
#include "Step.hpp"

namespace net
{

/**
 * @brief StepRedis在回调后一定会被删除
 */
class RedisStep: public Step
{
public:
    RedisStep(Step* pNextStep = NULL);
    RedisStep(const tagMsgShell& stReqMsgShell, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, Step* pNextStep = NULL);
    virtual ~RedisStep();
    /**
     * @brief redis步骤回调
     * @param c redis连接上下文
     * @param status 回调状态
     * @param pReply 执行结果集
     */
    virtual E_CMD_STATUS Callback(const redisAsyncContext *c,int status,redisReply* pReply) = 0;
public:
    /**
     * @brief 从Step派生的回调函数
     * @deprecated RedisStep的Callback与普通Step的Callback不一样
     * @note 从Step派生的回调函数在reids回调中不需要，所以直接返回
     */
    virtual E_CMD_STATUS Callback(const tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data = NULL)
    {
        return(STATUS_CMD_COMPLETED);
    }
    /**
     * @brief 超时回调
     * @note redis step 暂时不启用超时机制
     * @return 回调状态
     */
    virtual E_CMD_STATUS Timeout(){return(STATUS_CMD_FAULT);}
public:
    util::RedisCmd* RedisCmd(){return(m_pRedisCmd);}
public:
    const util::RedisCmd* GetRedisCmd(){return(m_pRedisCmd);}
private:
    util::RedisCmd* m_pRedisCmd;
};


/**
 * @brief Redis连接属性
 * @note  Redis连接属性，因内部带有许多指针，并且没有必要提供深拷贝构造，所以不可以拷贝，也无需拷贝
 */
struct tagRedisAttr
{
    uint32 ulSeq;                           ///< redis连接序列号
    redisReply* pReply;                     ///< redis命令执行结果
    bool bIsReady;                          ///< redis连接是否准备就绪
    std::list<RedisStep*> listData;         ///< redis连接回调数据
    std::list<RedisStep*> listWaitData;     ///< redis等待连接成功需执行命令的数据

    tagRedisAttr() : ulSeq(0), pReply(NULL), bIsReady(false)
    {
    }

    ~tagRedisAttr()
    {
        //freeReplyObject(pReply);  redisProcessCallbacks()函数中有自动回收
		for (auto& iter:listData)
        {
			SAFE_DELETE(iter);
        }
        listData.clear();
        for (auto& iter:listWaitData)
        {
        	SAFE_DELETE(iter);
        }
        listWaitData.clear();
    }
};

} /* namespace net */

#endif /* SRC_STEP_REDISSTEP_HPP_ */
