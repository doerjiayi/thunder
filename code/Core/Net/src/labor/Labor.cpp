/*******************************************************************************
 * Project:  Net
 * @file     Labor.cpp
 * @brief 
 * @author   cjy
 * @date:    2017年9月6日
 * @note
 * Modify history:
 ******************************************************************************/
#include "Labor.hpp"

//每个进程只有一个labor，使用单例模式
net::Labor* g_pLabor = NULL;
net::Labor* GetLabor() {return g_pLabor;}
const net::Labor* GetCLabor() {return g_pLabor;}

namespace net
{

Labor::Labor()
{
	g_pLabor = this;
}

Labor::~Labor()
{
}

const std::string& Labor::GetWorkerIdentify()
{
	if (m_strWorkerIdentify.size() < 5) // IP + port + worker_index长度一定会大于这个数即可，不在乎数值是什么
	{
		char szWorkerIdentify[64] = {0};
		snprintf(szWorkerIdentify, 64, "%s:%d.%d", GetHostForServer().c_str(),GetPortForServer(), GetWorkerIndex());
		m_strWorkerIdentify = szWorkerIdentify;
	}
	return(m_strWorkerIdentify);
}

const std::string& Labor::GetNodeIdentify()
{
	if (m_strNodeIdentify.size() < 5) // IP + port长度一定会大于这个数即可，不在乎数值是什么
	{
		char szNodeIdentify[64] = {0};
		snprintf(szNodeIdentify, 64, "%s:%d", GetHostForServer().c_str(),GetPortForServer());
		m_strNodeIdentify = szNodeIdentify;
	}
	return(m_strNodeIdentify);
}


} /* namespace net */
