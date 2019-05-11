1）服务器时间
主从中心节点所在的物理机的服务器时间需要是一致的。
查看时间服务器的时间:
# rdate time-b.nist.gov
设置时间和时间服务器同步:（需要root权限）
# rdate -s time-b.nist.gov
在crontab表中设置每天同步一次时间，加入一行如下：
执行 crontab -e
* 0 * * * rdate -s time-b.nist.gov
2）上报时间
中心服务器的cmd的动态库的makefile的心跳时间定义宏 -DNODE_BEAT=10.0 ，必须跟框架Net的makefile的一致
3）发布前需要先安装数据表 
执行db_im3_center_10_17.sql
