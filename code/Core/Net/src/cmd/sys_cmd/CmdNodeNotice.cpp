/*******************************************************************************
 * Project:  Net
 * @file     CmdNodeNotice.cpp
 * @brief 
 * @author   cjy
 * @date:    2019年8月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include <cmd/sys_cmd/CmdNodeNotice.hpp>
#include "util/json/CJsonObject.hpp"
#include <step/sys_step/StepNodeNotice.hpp>
#include <iostream>
using namespace std;
namespace net
{

CmdNodeNotice::CmdNodeNotice()
{
}

CmdNodeNotice::~CmdNodeNotice()
{

}

bool CmdNodeNotice::AnyMessage(
                const tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
	LOG4_DEBUG("CmdNodeNotice seq[%llu] jsonbuf[%s] Parse is ok",oInMsgHead.seq(),oInMsgBody.body().c_str());
	if (m_jsonData.Parse(oInMsgBody.body()))
	{
		string node_type = "";
		string node_ip = "";
		int    node_port = 0;
		int    worker_num = 0;
		char   strIdentify[50] = {0};
		for (int i = 0;i<m_jsonData["node_arry_reg"].GetArraySize();i++)
		{
			if (
				m_jsonData["node_arry_reg"][i].Get("node_type",node_type)
				&&m_jsonData["node_arry_reg"][i].Get("node_ip",node_ip)
				&&m_jsonData["node_arry_reg"][i].Get("node_port",node_port)
				&&m_jsonData["node_arry_reg"][i].Get("worker_num",worker_num)
				)
			{
				for(int j = 0;j<worker_num;j++)
				{
					sprintf(strIdentify,"%s:%d.%d",node_ip.c_str(),node_port,j);
					GetLabor()->AddNodeIdentify(node_type,string(strIdentify));
					LOG4_DEBUG("Step::AddNodeIdentify(%s,%s)",node_type.c_str(),strIdentify);
				}
			}
		}

		for (int i = 0;i<m_jsonData["node_arry_exit"].GetArraySize();i++)
		{
			if (
				m_jsonData["node_arry_exit"][i].Get("node_type",node_type)
				&&m_jsonData["node_arry_exit"][i].Get("node_ip",node_ip)
				&&m_jsonData["node_arry_exit"][i].Get("node_port",node_port)
				&&m_jsonData["node_arry_exit"][i].Get("worker_num",worker_num)
				)
			{
				for(int j = 0;j<worker_num;j++)
				{
					sprintf(strIdentify,"%s:%d.%d",node_ip.c_str(),node_port,j);
					GetLabor()->DelNodeIdentify(node_type,string(strIdentify));
					LOG4_DEBUG("Step::DelNodeIdentify(%s,%s)",node_type.c_str(),strIdentify);
				}
			}
		}
	}
	GetLabor()->SendToClient(stMsgShell,oInMsgHead,"ok");
    return(true);
}

} /* namespace net */
