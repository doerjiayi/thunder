# 数据恢复 #
## 数据目录 ##
/app/thunder/analysisServer/deploy/LogQueue/data/file
## 数据文件格式 ##
包含数据头、数据体，分别是占一行的json格式数据，如下：
{"index":{"_index":"db_page_trace","_type":"tb_page_trace","_id":"408577775029026449633516908916"}}
{"time":"2017-09-11 00:00:59","page":"/h5/project/speed-loan/index.html#/login?phoneNum=","session_id":"1505059247816","device_id":"790a545a-43bf-4f25-8cfc-46442c061283","channel":"H5","event_id":"SpeedLoanLogin"}
## 恢复时间指令 ##
> http://192.168.18.78:17039/analysis/logrecover/recover?appkey=12345678910
## 恢复全部 ##
{
    "all": 1
}
## 恢复 指定时间戳范围内 ##
{
    "begin_t":1503896522,
	"end_t":1503996522
}
## 恢复 指定时间字符串范围内 ##
{
    "begin_d":"2017-07-02 21:39:43",
	"end_d":"2017-08-29 21:39:43"
}

{
    "begin_d":"2017-07-02",
	"end_d":"2017-08-30"
}
# 数据清空 #
清除指定数据库中的数据
> db_analysis_daily_statistics
> db_analysis_event_statistics

执行脚本(需要连接配置数据库)

LogRecover/dbflushall.sh
# 数据查看 #
统计已生成的日志数据
> LogQueue/check_all_file.sh

查看正在写入中的日志文件末尾
> LogQueue/check_new_file.sh

# 服务器列表 #
> CenterServer 192.168.18.78 17000
> 
> DataProxyServer 192.168.18.78 17004
> 
> DbAgentWrite 192.168.18.78 17005
> 
> DbAgentRead 192.168.18.78 17006
> 
> CollectServer 192.168.18.78 17037(外) 17038(内)
> 
> LogQueue 192.168.18.78 17009(内)
> 
> LogRecover 192.168.18.78 17039(外) 17039(内)
> 
> Statistics 192.168.18.78 17011(内)
> 
> StatisticsRecover 192.168.18.78 17012(内)
# 安装#
安装所有文件
deploy/install.sh all

安装所有bin
deploy/install_bins.sh all

安装所有plugin
deploy/install_plugins.sh all

安装所有系统动态库
deploy/install_libs.sh all

# 编译 #
编译 proto、Starship、loss
code/make_libs.sh all

编译 所有plugins
code/make_plugins.sh all

编译 所有库
code/make.sh all

# 编译脚本配置 #
脚本时间通知配置

Starship：makefile.access makefile.center makefile.other

中心节点：Makefile

节点子进程上报管理者、节点上报中心时间间隔配置
NODE_BEAT=10.0

子进程超时被重启时间配置
WORKER_OVERDUE=120.0

# 修改服务器配置 #
deploy/install.sh config

# 目录结构 #
server_dir.conf 服务节点插件路径

server_list.conf 启动、关闭服务节点配置

clear.sh清理服务运行文件脚本

install.sh安装服务程序文件脚本

restart_nodes.sh、start_nodes.sh、stop_nodes.sh重启、启动、停止服务程序文件脚本

# 代码统计 #
脚本code.sh 

统计所有代码 code.sh all

统计框架代码 code.sh frame

统计指定目录代码 code.sh ./

# mysql数据库 #

mysql实例管理脚本mysqlmgr.sh

mysql 数据建表脚本loadsql.sh

mysql配置及其说明my3306.cnf

# https 相关接口增加 #
新增加使用库libcurl，及其库文件libcurl.so libcurl.so.4 libcurl.so.4.5.0。新增接口HttpsGet、HttpsPost

# 重新加载逻辑so文件 #
restart_nodes.sh reload 重新加载所有正在运行的本机节点的插件so。目前只支持重新加载节点的服务配置，不支持重新加载so的逻辑代码。

在节点目录下
restart.sh reload 重新加载该节点正在运行的插件so。目前只支持重新加载节点的服务配置，不支持重新加载so的逻辑代码。

数据库集群配置（DbAgent的配置文件 CmdDbOper.json）
支持集群包括副本集:cluster 和 主从集群:masterslave. 
副本集 版本 为 mariadb-10.2.11
