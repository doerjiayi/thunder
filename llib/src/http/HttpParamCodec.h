/*******************************************************************************
 * Project:  protocol
 * @file     HttpParamCodec.h
 * @brief    http post参数编码/解码
 * @author   JiangJianyu
 * @date:    2017年12月21日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef HTTPPARAMCODEC_H_
#define HTTPPARAMCODEC_H_

#include <string>
#include <map>

namespace thunder
{

class HttpParamCodec
{
public:
    HttpParamCodec();
    virtual ~HttpParamCodec();
    /**
     * @brief post参数解码，解码成功后可以通过getParam（）函数获取相应的参数值
     * @param strParam:需要解码的post参数字符串
     * @return 调用成功返回true，失败返回false
     */
    bool decode(const std::string &strParam);
    bool decodeCookie(const std::string &strCookie);
    bool decodeUrl(const std::string &strUrl);
    /**
     * @brief post参数编码，编码前需要先使用setParam()函数设置参数值
     * @param strParam:编码后的post参数字符串通过strParam返回
     * @return 调用成功返回true，失败返回false
     */
    bool encode(std::string &strParam);
    //cookie参数编码
    bool encodeCookie(std::string &strParam);
    /**
     * @brief 获取参数值
     * @param strKey：参数名称
     * @param strValue：参数返回值
     * @return 调用成功返回true，失败返回false
     */
    bool getParam(const std::string &strKey, std::string &strValue);
    //从cookie中获取参数
    bool getCookieParam(const std::string &strKey, std::string &strValue);
    /**
     * @brief 设置参数值
     * @param strKey：参数名称
     * @param strValue：参数值
     * @return 调用成功返回true，失败返回false
     */
    bool setParam(const std::string &strKey, const std::string &strValue);
    bool setParam(const std::string &strKey, int iValue);
    bool setParam(const std::string &strKey, long lValue);
    bool setParam(const std::string &strKey, float fValue);
    bool setParam(const std::string &strKey, double dValue);
    //设置cookie参数(如果不是键值对，strValue可以只传入一个空字符串)
    bool setCookieParam(const std::string &strKey, const std::string &strValue);
    bool setCookieParam(const std::string &strKey, int iValue);
    bool setCookieParam(const std::string &strKey, long lValue);
    bool setCookieParam(const std::string &strKey, float fValue);
    bool setCookieParam(const std::string &strKey, double dValue);
    /**
     * @brief 获取参数个数
     * @return 参数个数
     */
    int getCount(){return m_map.size();}

private:
    //字符串替换,strSrc中所有的strOld都会被strNew替换
    void ReplaceString(std::string &strSrc, const std::string &strOld, const std::string &strNew);

private:
    std::map<std::string, std::string> m_map;//kev --> value
    std::map<std::string, std::string> m_mapCookie;
};

} /* namespace analysis */

#endif /* HTTPPARAMCODEC_H_ */
