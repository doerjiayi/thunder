/*******************************************************************************
 * Project:  Net
 * @file     CmdBeat.cpp
 * @brief 
 * @author   chenjiayi
 * @date:    2016年11月5日
 * @note
 * Modify history:
 ******************************************************************************/
#include <iostream>
#include <istream>
#include <fstream>
#include "CmdUpdateConfig.hpp"

namespace net
{

CmdUpdateConfig::CmdUpdateConfig()
{
    m_ReqConfigType = 0;
}

CmdUpdateConfig::~CmdUpdateConfig()
{
}

bool CmdUpdateConfig::AnyMessage(
                const tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    const std::string& oInMsgBodyStr = oInMsgBody.body();
    LOG4_DEBUG("CmdUpdateConfig body(%s)",oInMsgBodyStr.c_str());
    //更新服务器配置文件
    util::CJsonObject reqConfigObj;
	if (!reqConfigObj.Parse(oInMsgBodyStr))
	{
		LOG4_WARN("oInMsgBodyStr is not json obj.oInMsgBodyStr: %s",oInMsgBodyStr.c_str());
		return (false);
	}
	if(!reqConfigObj.Get("config_file",m_ReqConfigFileName))
	{
		LOG4_WARN("config_file don't exist.oInMsgBodyStr: %s",oInMsgBodyStr.c_str());
		return (false);
	}
	if(!reqConfigObj.Get("config_content",m_ReqConfigContent))
	{
		LOG4_WARN("config_content don't exist.oInMsgBodyStr: %s",oInMsgBodyStr.c_str());
		return (false);
	}
	if(!reqConfigObj.Get("config_type",m_ReqConfigType))
	{
	    LOG4_WARN("config_type don't exist.oInMsgBodyStr: %s",oInMsgBodyStr.c_str());
        return (false);
	}
	return ReadConfig();
}

bool CmdUpdateConfig::ReadConfig()
{
    //配置文件路径查找
    std::string strConfFile = GetConfigPath() + m_ReqConfigFileName;
    LOG4_DEBUG("ReqConfigType(%d).CONF FILE = %s.ReqConfigContent(%s).",
                    m_ReqConfigType,strConfFile.c_str(),m_ReqConfigContent.ToString().c_str());
    util::CJsonObject oLocalConfJson;
	if (!net::GetConfig(oLocalConfJson,strConfFile))//读取配置文件
	{
		LOG4_ERROR("Open conf (%s) to read error!",strConfFile.c_str());
		return false;
	}
    std::string oConfJsonStr;
    if(0 == m_ReqConfigType)
    {//修改配置文件内容(这里是检查服务器配置)
        //需要更新的字段的内容
    	LOG4_DEBUG("before update CONF FILE CONTENT= %s.",oLocalConfJson.ToFormattedString().c_str());
    	{//检查so
    	    util::CJsonObject configUpdateContentSo;//更新so
            util::CJsonObject configLocalContentSo;//本地so
            if(!m_ReqConfigContent.Get("so",configUpdateContentSo))
            {
                LOG4_DEBUG("so don't exist.");
            }
            else
            {
                LOG4_DEBUG("configUpdateContentSo:%s",configUpdateContentSo.ToString().c_str());
                if(oLocalConfJson.Get("so",configLocalContentSo) && (configLocalContentSo.ToString() != configUpdateContentSo.ToString()))
                {
                    LOG4_DEBUG("configUpdateContentSo:%s",configUpdateContentSo.ToString().c_str());
                    oLocalConfJson.Replace("so",configUpdateContentSo);
                }
                else
                {
                    LOG4_DEBUG("don't need to update so(%s)",configUpdateContentSo.ToFormattedString().c_str());
                }
            }
    	}
    	{//检查log_level
    	    int iUpdateLogLevel(0);//更新日志等级
            int iLocalLogLevel(0);//本地日志等级
            if(!m_ReqConfigContent.Get("log_level",iUpdateLogLevel))
            {
                LOG4_DEBUG("log_level don't exist");
            }
            else
            {
                LOG4_DEBUG("log_level:%d",iLocalLogLevel);
                if(oLocalConfJson.Get("log_level",iLocalLogLevel) && (iLocalLogLevel != iUpdateLogLevel))
                {
                    LOG4_DEBUG("update log_level(%d)",iUpdateLogLevel);
                    oLocalConfJson.Replace("log_level",iUpdateLogLevel);
                }
                else
                {
                    LOG4_DEBUG("don't need to update log_level(%d)",iUpdateLogLevel);
                }
            }
    	}
    	{//检查module
    	    util::CJsonObject configUpdateContentModule;//更新module
            util::CJsonObject configLocalContentModule;//本地module
            if(!m_ReqConfigContent.Get("module",configUpdateContentModule))
            {
                LOG4_DEBUG("module don't exist.");
            }
            else
            {
                LOG4_DEBUG("module:%s",configUpdateContentModule.ToString().c_str());
                if(oLocalConfJson.Get("module",configLocalContentModule) && (configLocalContentModule.ToString() != configUpdateContentModule.ToString()))
                {
                    LOG4_DEBUG("module:%s",configUpdateContentModule.ToString().c_str());
                    oLocalConfJson.Replace("module",configUpdateContentModule);
                }
                else
                {
                    LOG4_DEBUG("don't need to update module(%s)",configUpdateContentModule.ToFormattedString().c_str());
                }
            }
    	}
		oConfJsonStr = oLocalConfJson.ToFormattedString();
		LOG4_DEBUG("after update CONF FILE CONTENT= %s.",oConfJsonStr.c_str());
    }
    else
    {
        LOG4_DEBUG("before update CONF FILE CONTENT= %s.",
                        oLocalConfJson.ToFormattedString().c_str());
        oConfJsonStr = m_ReqConfigContent.ToFormattedString();
        LOG4_DEBUG("after update CONF FILE CONTENT= %s.",oConfJsonStr.c_str());
    }
    if(!oConfJsonStr.empty())
    {//写入配置文件内容
    	std::ofstream fin(strConfFile.c_str(),std::ios::out | std::ios::trunc);//打开时会清空文件
    	if (fin.good())
		{
			//解析配置信息 JSON格式
			fin.write(oConfJsonStr.c_str(),oConfJsonStr.length());
			fin.flush();
			fin.close();
			LOG4_DEBUG("update strConfFile(%s) ok",strConfFile.c_str());
		}
		else
		{
			//配置信息流读取失败
			LOG4_ERROR("Open conf (%s) to write error!",
							strConfFile.c_str());
			return false;
		}
    }
    return true;
}



} /* namespace bolt */
