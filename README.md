# 应用场景
分布式异步集群服务

#  引擎使用说明 #
在文件thunder引擎说明.pptx中
#  引擎使用说明 #
目录结构说明：

**引擎核心thunder**

cmd 系统指令\codec 系统指令\labor 工作者与管理者\protocol 服务器内部与客户端通用协议\session 会话对象\step 异步访问对象\storage 存储接口对象

**第三方库**
3party


# 空载压测 #
压测工具siege
测试为单个物理机6核cpu，16G内存，200.0 GB机械硬盘，1000Mb网卡和路由。压测客户端、计算集群、存储集群在一个物理机。
单进程23255.81qps网络收发（http Web服务器）返回默认响应  300连接 分别300消息 单进程23255.81qps

# 路由 #
支持中心路由，中心节点支持主从热切换（可一主多从，用到redis集群分布式锁）

# rpc #
支持状态机、协程(目前支持共享栈空间，支持的是内部通信；后续增加独立堆空间和钩子函数，和外部通信)、远程过程调用（使用匿名函数）

# io事件 #
支持自定义信号处理等


# 详细配置说明 #
参考config.md

# 分布式缓存 启动redis实例 #
提供分布式锁、缓存等

#目录说明
3party 第三方库
Core  核心逻辑 ：服务发现 、网络、常用库

# 编译
用ccache 优化编译 

#配置热加载
增加了脚本支持发送信号热加载配置

#mysql代理DbAgent
多进程支持同步和异步

#postgresql代理PgAgent
多进程支持同步

#DataProxy 缓存代理
支持分组redis 主从
支持分组redis cluster
支持与持久化存储的数据同步
支持网络分区和持久化存储异常情况下的数据补偿

#测试实例
Interface  节点为接入层
Logic  节点为逻辑层
Hello  功能测试

Interface 与 Logic的测试 说明参考node_test.md

#环境安装
参考install.md


#支持绑定cpu
绑定性能测试（不同环境不同），参考 bindcpu_test.md


#架构说明
参考 architecture.pptx

#todo list
自动构建 
docker化 核心服务（https://blog.csdn.net/q610376681/article/details/90483576）
增加kafka组件支持（https://github.com/mfontanini/cppkafka）
增加线程池，并使用libco实现对外部阻塞api的线程复用


