/*******************************************************************************
 * Project:  Net
 * @file     HttpStep.hpp
 * @brief    Http服务的异步步骤基类
 * @author   cjy
 * @date:    2019年10月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STEP_HTTPSTEP_HPP_
#define SRC_STEP_HTTPSTEP_HPP_

#include "Step.hpp"
#include "protocol/http.pb.h"
#include "codec/HttpCodec.hpp"

namespace net
{

class HttpStep: public Step
{
public://子类通过构造函数给父类传参后，使用父类的成员,包括m_oInHttpMsg,m_stReqMsgShell,m_oReqMsgHead,m_oReqMsgBody
    HttpStep(Step* pNextStep = NULL):net::Step(pNextStep){}
    HttpStep(const HttpMsg& oInHttpMsg,Step* pNextStep = NULL):net::Step(pNextStep){m_oInHttpMsg = oInHttpMsg;}
    HttpStep(const tagMsgShell& stInMsgShell, const MsgHead& oInMsgHead,Step* pNextStep = NULL):net::Step(stInMsgShell,oInMsgHead,pNextStep){}
    HttpStep(const tagMsgShell& stInMsgShell, const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,Step* pNextStep = NULL):net::Step(stInMsgShell,oInMsgHead,oInMsgBody,pNextStep){}
    HttpStep(const tagMsgShell& stInMsgShell, const HttpMsg& oInHttpMsg,Step* pNextStep = NULL):net::Step(stInMsgShell,pNextStep){m_oInHttpMsg = oInHttpMsg;}
    virtual ~HttpStep(){}
    //使用回调函数接口SendToCallback则不需要处理本step的Callback，否则就继承Callback
    virtual E_CMD_STATUS Callback(const tagMsgShell& stMsgShell,const HttpMsg& oHttpMsg,void* data = NULL){return net::STATUS_CMD_COMPLETED;}
    //Step基类的方法，HttpStep中无须关注
    virtual E_CMD_STATUS Callback(const tagMsgShell& stMsgShell,const MsgHead& oInMsgHead,const MsgBody& oInMsgBody,void* data = NULL){return(STATUS_CMD_COMPLETED);}
    //@brief 步骤超时回调
    virtual E_CMD_STATUS Timeout() = 0;
    bool HttpPost(const std::string& strUrl,const std::string& strBody, const std::unordered_map<std::string, std::string>& mapHeaders);
    bool HttpPost(const std::string& strUrl,const std::string& strBody);
    bool HttpGet(const std::string& strUrl);
    bool HttpRequest(const HttpMsg& oHttpMsg);
    bool SendTo(const tagMsgShell& stMsgShell, const HttpMsg& oHttpMsg);
};

} /* namespace net */

#endif /* SRC_STEP_HTTPSTEP_HPP_ */
