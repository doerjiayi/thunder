# 近期总体功能更新简述 #

- 使用c++11特性，支持匿名函数等新功能，简化远程调用等接口使用，不需要写新step或者新函数。
- 编译选项增加优化选项，makefile加-Og，提升整体性能50%，单节点接收返回http请求12032.09 trans/sec，支持调试。
- json结构优化，优化支持配置文件数据等缓存内存，读取接口性能提高几百倍，可提高dataproxy数据检查性能。
- 新增协程函数，简化异步接口代码编写
- 新增mysql step及其异步处理,提高4倍左右读写性能，支持多连接，经过单进程代理abagent读操作6024.10 trans/sec，写操作mysql 6451.61 trans/sec。访问mysql使用mysql step支持失败重发，提高峰值情况下的发送成功率，目前使用在读操作。
- 改造center、dbagent大部分操作为异步访问mysql数据库，提高访问性能和接口可用性，也避免了dbagent同步访问mysql阻塞引发的子进程被重启
- 新增https发送支持，支持与第三方api（如极光推送）的接口互通。
- 使用新https接口支持域名解析缓存，提高访问第三方https接口2倍以上性能
- 使用ccache支持预编译代码头文件，提高几倍代码编译速度
- 优化dataproxy、dbagent数据合法性检查代码，dataproxy写本地数据文件代码
- 支持发送运维指令重新初始化逻辑动态库，可以重新读取节点服务配置。支持发送运维指令重新启动节点子进程。
- 参考os.conf配置优化linux系统tcp连接处理选项，可大幅提高http后台服务连接数处理能力，避免大量连接时连接失败问题
- 整理框架工作者io相关api封装，减少冗余代码。优化管理者到工作者文件描述符异步转移处理。优化socket属性选项，提高消息发送响应速度等。
- 简化运维节点服务配置文件，中心逻辑配置文件，优化运维脚本批量管理配置文件等。
- 框架新增redis cluster集群支持。支持redis服务高可用、主从切换功能、支持哈希槽动态移动和节点动态伸缩，客户端支持节点列表动态更新。100w请求异步测试 4580.305176 ms ，平均2180000qps。
- 框架新增MariaDB Galera Cluster 集群支持，支持多主集群。支持原来的mysql接口，支持高可用、多主读写功能。DbAgent客户端支持心跳保活检查。

可用于Starship框架下的所有后台服务项目

# 近期总体框架问题处理简述 #
- 处理加载动态库时失败引发宕机的问题，如初始化配置文件失败的宕机
- 处理发送http请求url解析可能失败的问题
- 修改dataproxy返回数据消息大小限制，避免数据消息过大而失败

# 暂未处理的拓展功能 #
- 暂时限制逻辑动态库的代码热更新功能，使用pb动态库等含静态变量第三方库若热更新代码可能引发错误。

# 目前依赖第三方库和框架依赖功能 #
libcryptopp.so 加密
libcurl.so https
libev.so 事件驱动
libhiredis.so redis异步访问
libjemalloc.so 内存分配缓存优化
liblog4cplus.so 日志
libmariadb.so mysql同步、异步访问，可同时支持mariadb
libprotobuf.so pb协议
gcc 4.8.2 ，glibc  GLIBCXX_3.4.19（开发环境）

#需要用到的运维工具
yum -y install bc  ccache

# 版本更新说明1.0 #
## 远程过程调用 ##
1. 编译使用c++11特性的选项-std=c++11（开发环境使用gcc 4.8.2），并且已重新编译liblog4cplus到目录3lib，包括liblog4cplus.so\liblog4cplus-1.1.so.9.1.3\liblog4cplus-1.1.so.9

在session中远程过程调用：
<pre><code>
auto Callback = [] (const DataMem::MemRsp &oRsp,net::Session* pSession)
{
    //返回的响应
};
char sRedisKey[64];
//写sRedisKey 
net::RedisOperator oRedisOperator(0, sRedisKey,"HMSET");
//写oRedisOperator
SendToProxyCallBack(oRedisOperator.MakeMemOperate(),Callback,true)；//发送
</code></pre>
其中Callback为匿名函数，其定义可在函数内部使用。

## 异步mysql访问 ##
1. 新增mysql step及其异步处理，需要更新第三方库文件liblog4cplus-1.1.so、libmariadb.so和重新编译更新框架库文件和插件库文件。
1. 异步mysql step客户端性能大致是同步mysql 4到5倍，并且不影响进程进行其他操作，如上报管理者进程等，支持回调中嵌套发送新mysql指令。支持mysql多连接发送。进行过性能测试和功能测试、数据完整性测试。
1. 改造center、dbagent大部分操作为异步访问数据库
1. 为了支持异步处理，第三方库修改为使用libmariadb.so（mariadb-connector-c），不用libmysqlclient.so，mariadb-connector-c地址 https://github.com/MariaDB/mariadb-connector-c。mariadb-connector-c支持对mysql和mariadb的访问。

异步访问mysql步骤接口如下：
<pre><code>
auto mysqlCallback = [](net::StepState* state)
{
	LOG4_TRACE_S(state,"LoadConfigFilesStateSendToMysqlCallback");
	STAGE_TEST_PARAM(LoadConfigSendToMysqlParam,state);
	net::MysqlStep* pMysqlState = (net::MysqlStep*)state;
	if (pMysqlState->m_pMysqlResSet)
	{
		util::T_vecResultSet vecRes;
		if (pMysqlState->m_pMysqlResSet->GetResultSet(vecRes) > 0)
		{
			pStageParam->pNodeSession->LoadConfigFiles(vecRes);
			LOG4_TRACE_S(state,"LoadConfigFilesStateSendToMysqlCallback ok vecRes size:%u",vecRes.size());
		}
	}
	return true;
};
net::MysqlStep* pstep = new net::MysqlStep(m_dbConnInfo);
pstep->SetTask(util::eSqlTaskOper_select,"select * from %s", NODE_CONFIG_FILES_TABLE);//第一个任务，读取配置
pstep->AddStateFunc(mysqlCallback);//stage 0
pstep->SetData(new LoadConfigSendToMysqlParam(this));
if (!net::MysqlStep::Launch(GetLabor(),pstep))
{
	LOG4_WARN("MysqlStep::Launch failed");
	return (false);
}
</code></pre>

# 版本更新说明1.1 #
1. mysql client编码修改
1. 框架网络处理连接接口修改
1. 增加步骤协程函数支持和封装
1. 编译选项增加优化选项

## 协程函数访问mysql ##
使用接口步骤如下，使用协程函数AddCoroutinueFunc：
<pre><code>
auto mysqlCallback_tb_nodetype = [](bolt::StepState* state)
{
	net::MysqlStep* pMysqlState = (net::MysqlStep*)state;
	if (pMysqlState->m_pMysqlResSet)//第一个任务响应结果集
	{
		llib::T_vecResultSet vecRes;
		if (pMysqlState->m_pMysqlResSet->GetResultSet(vecRes) > 0)
		{
			LOG4_TRACE_S(state,"LoadNodeTypes callback ok vecRes size:%u",vecRes.size());
			//pMysqlState->SetConf(pMysqlState->m_dbConnInfo);//这里可以修改db链接配置(如无需要可不修改)
			pMysqlState->AppendTask(llib::eSqlTaskOper_select,"select * from tb_config_files");//第2个任务，读取配置
			//load tb_config_files callback
			if (pMysqlState->m_pMysqlResSet)//第2个任务响应结果集
			{
				llib::T_vecResultSet vecRes;
				if (pMysqlState->m_pMysqlResSet->GetResultSet(vecRes) > 0)
				{
					LOG4_TRACE_S(state,"tb_config_files callback ok vecRes size:%u",vecRes.size());
					llib::CJsonObject oRsp;
					oRsp.Add("code", 0);
					oRsp.Add("msg", "ok");
					state->SendBack(oRsp.ToString());
				}
			}
		}
	}
};
net::MysqlStep* pstep = new net::MysqlStep(stMsgShell,oInHttpMsg,
		local_mysql_ip,local_mysql_port,local_mysql_dbname,local_mysql_user,local_mysql_pwd,local_mysql_charset);
pstep->SetTask(llib::eSqlTaskOper_select,"select * from tb_nodetype");//第一个任务，读取节点类型
pstep->AddCoroutinueFunc(mysqlCallback_tb_nodetype);//stage 0 ,第一个协程函数
if (!net::MysqlStep::Launch(GetLabor(),pstep))
{
	LOG4_WARN("MysqlStep::Launch failed");
	return (false);
}
</code></pre>

## 协程函数访问普通节点 ##
使用接口步骤如下，使用协程函数AddCoroutinueFunc：
<pre><code>
auto ReadAppTable = [](bolt::StepState* state)
{
	bolt::DbOperator oMemOper(0,"test1",DataMem::MemOperate::DbOperate::SELECT);
	oMemOper.AddDbField("col1");//0
	oMemOper.AddDbField("col2");//1
	if (!state->SendToProxy(oMemOper.MakeMemOperate()->SerializeAsString(),"DBAGENT_R"))return;
	DataMem::MemRsp oMemRsp;
	if (state->RecvFromProxy(oMemRsp))
	{
		LOG4_TRACE_S(state,"%s() uiSendCounter(%llu) uiRecvCounter(%llu) RecvFromProxy:%s!",
				__FUNCTION__,uiSendCounter,++uiRecvCounter,oMemRsp.DebugString().c_str());
		llib::CJsonObject oRsp;
		oRsp.Add("code", 0);
		oRsp.Add("msg", "ok");
		state->SendBack(oRsp.ToString());
	}
};
bolt::StepState* pstep = new bolt::StepState(stMsgShell,oInHttpMsg);
pstep->AddCoroutinueFunc(ReadAppTable);
return bolt::StepState::Launch(GetLabor(),pstep);
</code></pre>

# 版本更新说明1.2 #
框架新增redis cluster集群支持。支持redis高可用、主从切换功能。redis cluster集群管理脚本为cluster.sh。redis版本为3.2.11，目前暂时不能支持4.0以上版本的redis cluster集群客户端。

第三方库libhiredis.so 替换为libhiredis_vip.so libhiredis_vip.so.1.0

redis cluster集群在第一次启动后，需要执行cluster通知操作来搭建集群，指令为./cluster.sh clucter。已搭建过的则不需要重复执行。

CmdDataProxy.json配置redis cluster使用说明
<pre><code>
"redis_group_brief":"master为一个服务器列表则表示redis cluster集群,此时slave与master相同；master为单个服务器则表示主从，此时slave为master的从服务配置",
"redis_cluster_brief":"redis cluster集群一般至少6个实例以上，每个分片则可以是不同的集群；若为相同集群，则集群列表为同样的字符串",
"redis_group": {
    "robot_data_attribute_1": {
        "master": "192.168.18.78:6000,192.168.18.78:6001,192.168.18.78:6002,192.168.18.78:6003,192.168.18.78:6004,192.168.18.78:6005",
        "slave": "192.168.18.78:6000,192.168.18.78:6001,192.168.18.78:6002,192.168.18.78:6003,192.168.18.78:6004,192.168.18.78:6005"
    },
    "robot_data_attribute_2": {
        "master": "192.168.18.78:6000,192.168.18.78:6001,192.168.18.78:6002,192.168.18.78:6003,192.168.18.78:6004,192.168.18.78:6005",
        "slave": "192.168.18.78:6000,192.168.18.78:6001,192.168.18.78:6002,192.168.18.78:6003,192.168.18.78:6004,192.168.18.78:6005"
    },
    "robot_data_log_1": {
        "master": "192.168.18.78:6000,192.168.18.78:6001,192.168.18.78:6002,192.168.18.78:6003,192.168.18.78:6004,192.168.18.78:6005",
        "slave": "192.168.18.78:6000,192.168.18.78:6001,192.168.18.78:6002,192.168.18.78:6003,192.168.18.78:6004,192.168.18.78:6005"
    },
    "robot_data_log_2": {
        "master": "192.168.18.78:6000,192.168.18.78:6001,192.168.18.78:6002,192.168.18.78:6003,192.168.18.78:6004,192.168.18.78:6005",
        "slave": "192.168.18.78:6000,192.168.18.78:6001,192.168.18.78:6002,192.168.18.78:6003,192.168.18.78:6004,192.168.18.78:6005"
    }
}
</code></pre>
若不使用redis cluster集群，而使用主从集群，管理脚本为masterslave.sh

CmdDataProxy.json配置redis主从使用说明
<pre><code>
"redis_group": {
    "robot_data_attribute_1": {
        "master": "192.168.18.78:36379",
        "slave": "192.168.18.78:36379"
    },
    "robot_data_attribute_2": {
        "master": "192.168.18.78:36380",
        "slave": "192.168.18.78:36380"
    },
    "robot_data_log_1": {
        "master": "192.168.18.78:46379",
        "slave": "192.168.18.78:46379"
    },
    "robot_data_log_2": {
        "master": "192.168.18.78:46380",
        "slave": "192.168.18.78:46380"
    }
}
</code></pre>

# 版本更新说明1.3 #
框架新增MariaDB Galera Cluster 集群支持，支持多主集群。支持原来的mysql接口，支持高可用、多主读写功能。DbAgent客户端支持心跳保活检查。MariaDB服务使用版本10.2.11。
CmdDbOper.json配置mariadb多主使用说明
包含host_list则使用多主集群配置
<pre><code>
"db_group": {
    "robot_db_instance_1": {
    	"host_list": ["192.168.18.78:3307","192.168.18.78:3308","192.168.18.78:3309"],
        "user": "robot",
        "password": "123456",
        "charset": "utf8",
        "query_permit": 7,
        "timeout": 3
    },
    "robot_db_instance_2": {
    	"host_list": ["192.168.18.78:3307","192.168.18.78:3308","192.168.18.78:3309"],
        "user": "robot",
        "password": "123456",
        "charset": "utf8",
        "query_permit": 7,
        "timeout": 3
    }
}
</code></pre>
不包含host_list则使用主从配置
<pre><code>
"db_group": {
    "robot_db_instance_1": {
        "master_host": "192.168.18.78",
        "slave_host": "192.168.18.78",
        "port": 3307,
        "user": "robot",
        "password": "123456",
        "charset": "utf8",
        "query_permit": 7,
        "timeout": 3
    },
    "robot_db_instance_2": {
        "master_host": "192.168.18.78",
        "slave_host": "192.168.18.78",
        "port": 3307,
        "user": "robot",
        "password": "123456",
        "charset": "utf8",
        "query_permit": 7,
        "timeout": 3
    }
}
</code></pre>