/*******************************************************************************
 * Project:  protocol
 * @file     HttpParamCodec.cpp
 * @brief 
 * @author   JiangJianyu
 * @date:    2015年12月21日
 * @note
 * Modify history:
 ******************************************************************************/
#include <list>
#include <stdio.h>
#include "http_parser.h"
#include "HttpParamCodec.h"
#include "HttpUrlCoder.h"

namespace thunder
{

HttpParamCodec::HttpParamCodec()
{
    
}

HttpParamCodec::~HttpParamCodec()
{
}

bool HttpParamCodec::decode(const std::string &strParam)
{
    std::string strSrc = strParam;
    //url code解码
    url_decode_original(strSrc);
    //去除空格
    ReplaceString(strSrc," ","");
    if(strSrc.size() <= 0)
    {
        return false;
    }
    int iIndex = -1;
    std::list<std::string> list;

    while((iIndex = strSrc.find('&')) >= 0)
    {
        list.push_back(strSrc.substr(0,iIndex));
        strSrc.replace(0,iIndex + 1,"");
    }
    if(strSrc.size() > 0)
    {
        list.push_back(strSrc);
    }

    m_map.clear();
    for(std::list<std::string>::iterator iter = list.begin(); iter != list.end(); iter++)
    {
        std::string str = *iter;
        iIndex = str.find('=');
        if(iIndex <= 0 || iIndex == (int)(str.size() - 1))
        {
            return false;
        }
        m_map.insert(std::pair<std::string,std::string>(str.substr(0,iIndex),str.substr(iIndex + 1,str.size() - 1)));
    }

    return true;
}

bool HttpParamCodec::decodeCookie(const std::string &strCookie)
{
    std::string strSrc = strCookie;
    //去除空格
    ReplaceString(strSrc," ","");
    if(strSrc.size() <= 0)
    {
        return false;
    }
    int iIndex = -1;
    std::list<std::string> list;

    while((iIndex = strSrc.find(';')) >= 0)
    {
        list.push_back(strSrc.substr(0,iIndex));
        strSrc.replace(0,iIndex + 1,"");
    }
    if(strSrc.size() > 0)
    {
        list.push_back(strSrc);
    }
    m_mapCookie.clear();    
    for(std::list<std::string>::iterator iter = list.begin(); iter != list.end(); iter++)
    {
        std::string str = *iter;
        iIndex = str.find('=');
        if(iIndex <= 0 || iIndex == (int)(str.size() - 1))
        {
            return false;
        }
        m_mapCookie.insert(std::pair<std::string,std::string>(str.substr(0,iIndex),str.substr(iIndex + 1,str.size() - 1)));
    }
    return true;
}

bool HttpParamCodec::decodeUrl(const std::string &strUrl)
{
    struct http_parser_url stUrl;
    if(0 == http_parser_parse_url(strUrl.c_str(), strUrl.size(), 0, &stUrl))
    {
        if(!(stUrl.field_set & (1 << UF_QUERY)))
        {
            return false;
        }
        std::string strSrc;
        std::list<std::string> list;

        strSrc.assign(strUrl.data()+stUrl.field_data[UF_QUERY].off, stUrl.field_data[UF_QUERY].len);
        if(strSrc.size() <= 0)
        {
            return true;
        }
        int iIndex = -1;
        while((iIndex = strSrc.find('&')) >= 0)
        {
            list.push_back(strSrc.substr(0,iIndex));
            strSrc.replace(0,iIndex + 1,"");
        }
        if(strSrc.size() > 0)
        {
            list.push_back(strSrc);
        }
        m_map.clear();
        for(std::list<std::string>::iterator iter = list.begin(); iter != list.end(); iter++)
        {
            std::string str = *iter;
            iIndex = str.find('=');
            if(iIndex <= 0 || iIndex == (int)(str.size() - 1))
            {
                return false;
            }
            m_map.insert(std::pair<std::string,std::string>(str.substr(0,iIndex),str.substr(iIndex + 1,str.size() - 1)));
        }
        return true;
    }
    return false;
}

bool HttpParamCodec::encode(std::string &strParam)
{
    std::string strDes;
    if(m_map.size() <= 0)
    {
        return false;
    }
    for(std::map<std::string, std::string>::iterator iter = m_map.begin(); iter != m_map.end(); iter ++)
    {
        if(strDes.size() > 0)
        {
            strDes += "&";
        }
        strDes += iter->first + "=" + iter->second;
    }
    strParam = strDes;
    return true;
}

bool HttpParamCodec::encodeCookie(std::string &strParam)
{
    std::string strDes;
    if(m_mapCookie.size() <= 0)
    {
        return false;
    }
    for(std::map<std::string, std::string>::iterator iter = m_mapCookie.begin(); iter != m_mapCookie.end(); iter ++)
    {
        if(strDes.size() > 0)
        {
            strDes += ";";
        }
        if(iter->first.size() > 0 && iter->second.size() >0)
        {
            strDes += iter->first + "=" + iter->second;
        }
        else if(iter->first.size() > 0)
        {
            strDes += iter->first;
        }
        else
        {
            strDes += iter->second;
        }
    }
    strParam = strDes;
    return true;
}

bool HttpParamCodec::setParam(const std::string &strKey, const std::string &strValue)
{
    if(strKey.size() <= 0 || strValue.size() <= 0)
    {
        return false;
    }
    std::map<std::string, std::string>::iterator iter = m_map.find(strKey);
    if(iter == m_map.end())
    {
        m_map.insert(std::pair<std::string,std::string>(strKey,strValue));
    }
    else
    {
        iter->second = strValue;
    }
    return true;
}

bool HttpParamCodec::setParam(const std::string &strKey, int iValue)
{
    char szValue[100] = {0};
    snprintf(szValue,sizeof(szValue),"%d",iValue);
    return setParam(strKey,szValue);
}

bool HttpParamCodec::setParam(const std::string &strKey, long lValue)
{
    char szValue[100] = {0};
    snprintf(szValue,sizeof(szValue),"%ld",lValue);
    return setParam(strKey,szValue);
}

bool HttpParamCodec::setParam(const std::string &strKey, float fValue)
{
    char szValue[100] = {0};
    snprintf(szValue,sizeof(szValue),"%f",fValue);
    return setParam(strKey,szValue);
}

bool HttpParamCodec::setParam(const std::string &strKey, double dValue)
{
    char szValue[100] = {0};
    snprintf(szValue,sizeof(szValue),"%f",dValue);
    return setParam(strKey,szValue);
}


bool HttpParamCodec::setCookieParam(const std::string &strKey, const std::string &strValue)
{
    if(strKey.size() <= 0 || strValue.size() <= 0)
    {
        return false;
    }
    std::map<std::string, std::string>::iterator iter = m_map.find(strKey);
    if(iter == m_mapCookie.end())
    {
        m_mapCookie.insert(std::pair<std::string,std::string>(strKey,strValue));
    }
    else
    {
        iter->second = strValue;
    }

    return true;
}

bool HttpParamCodec::setCookieParam(const std::string &strKey, int iValue)
{
    char szValue[100] = {0};
    snprintf(szValue,sizeof(szValue),"%d",iValue);
    return setCookieParam(strKey,szValue);
}

bool HttpParamCodec::setCookieParam(const std::string &strKey, long lValue)
{
    char szValue[100] = {0};
    snprintf(szValue,sizeof(szValue),"%ld",lValue);
    return setCookieParam(strKey,szValue);
}

bool HttpParamCodec::setCookieParam(const std::string &strKey, float fValue)
{
    char szValue[100] = {0};
    snprintf(szValue,sizeof(szValue),"%f",fValue);
    return setCookieParam(strKey,szValue);
}

bool HttpParamCodec::setCookieParam(const std::string &strKey, double dValue)
{
    char szValue[100] = {0};
    snprintf(szValue,sizeof(szValue),"%f",dValue);
    return setCookieParam(strKey,szValue);
}

bool HttpParamCodec::getParam(const std::string &strKey, std::string &strValue)
{
    std::map<std::string, std::string>::iterator iter = m_map.find(strKey);
    if(iter == m_map.end())
    {
        return false;
    }
    strValue = iter->second;
    return true;
}

bool HttpParamCodec::getCookieParam(const std::string &strKey, std::string &strValue)
{
    std::map<std::string, std::string>::iterator iter = m_mapCookie.find(strKey);
    if(iter == m_mapCookie.end())
    {
        return false;
    }
    strValue = iter->second;
    return true;
}

void HttpParamCodec::ReplaceString(std::string &strSrc, const std::string &strOld, const std::string &strNew)
{
    while(true)
    {
        std::string::size_type pos(0);
        if((pos = strSrc.find(strOld)) != std::string::npos)
        {
            strSrc.replace(pos,strOld.size(),strNew);
        }
        else
        {
            break;
        }
    }
}

} /* namespace analysis */
