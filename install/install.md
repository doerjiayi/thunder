一、软件安装
添加开发用户
groupadd dev 
useradd -g dev dev  
passwd dev 
vim /etc/sudoers
dev	ALL=(ALL) 	ALL

sudo chown dev:dev  /app/

CentOS7系统配置国内yum源和epel源(先查看系统版本 cat /etc/redhat-release 为Centos-7)
cd /etc/yum.repos.d/
sudo mkdir repo_bak
sudo mv *.repo repo_bak/
sudo wget http://mirrors.aliyun.com/repo/Centos-7.repo
sudo wget http://mirrors.163.com/.help/CentOS7-Base-163.repo
sudo yum clean all && sudo yum makecache

可选(上一步做了的不用做)
sudo yum list | grep epel-release && sudo yum install -y epel-release
sudo wget -O /etc/yum.repos.d/epel-7.repo http://mirrors.aliyun.com/repo/epel-7.repo  
sudo yum clean all && sudo yum makecache
sudo yum repolist enabled && sudo yum repolist all

安装
sudo yum -y install tar bzip2 readline readline-devel zlib zlib-devel unzip wget openssl openssl-devel bc git mlocate cmake python python-devel telnet lsof
sudo yum -y update nss curl 

安装gcc4.8.5(centos7以上 cat /etc/redhat-release)
sudo yum install -y gcc gcc-c++

可选安装ccache：
wget https://www.samba.org/ftp/ccache/ccache-3.5.tar.gz
tar -zxf ccache-3.5.tar.gz && cd ccache-3.5
./configure -prefix=/usr/local/ccache && make -j8 && sudo make install 
sudo ln -s /usr/local/ccache/bin/ccache /usr/bin/ccache

可选安装siege：
wget http://download.joedog.org/siege/siege-latest.tar.gz
cd siege-4.0.4/    
./configure --with-ssl && make && sudo make install
修改配置/home/dev/.siege/siege.conf
verbose = false
quiet = false
limit = 1023
concurrent = 10000
benchmark = true

二、系统调优
（1）Linux内核调优(使用于centos6.9\7.6,cat /etc/redhat-release)
sudo mv /etc/sysctl.conf /etc/sysctl.bak
sudo cp sysctl.conf /etc/
运行 sysctl -p即可生效。

（2）文件数调优
设置系统打开文件数设置，解决高并发下 too many open files 问题。此选项直接影响单个进程容纳的客户端连接数。
增大这个设置以便服务能够维持更多的TCP连接。
修改配置文件：/etc/security/limits.conf :
* soft nproc 1024000
* hard nproc 1024000
* soft nofile 1024000
* hard nofile 1024000
* soft core unlimited
* hard core unlimited

三、安装postgresql
1、下载源代码并解压
tar jxvf postgresql-10.5.tar.bz2 
cd postgresql-10.5
./configure --with-python && make && sudo make install

2、创建用户组和用户
sudo groupadd postgres  && sudo useradd -g postgres postgres  && sudo passwd postgres 
vim /etc/sudoers
postgres	ALL=(ALL) 	ALL

3、创建数据目录
sudo mkdir -p /data/pg/data && sudo chown -R postgres:postgres /data/pg && sudo chmod -R 0700 /data/pg

4、安装数据库
su postgres  
初始化数据库
/usr/local/pgsql/bin/initdb -D/data/pg/data   

5、修改postgresql.conf
vim /data/pg/data/postgresql.conf
 listen_addresses = '*'
 port= 5432
#支持从库（可选）
max_wal_senders = 10   #最多有10个流复制连接
wal_sender_timeout = 60s    #流复制超时时间
max_connections = 500   #最大连接时间，必须要小于从库的配置
wal_level = hot_standby  
wal_keep_segments = 256

logging_collector = on
log_min_duration_statement = 3000
log_lock_waits = on			 
log_timezone = 'Asia/Shanghai'  

设置服务器时区
cp -rf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime 

或者拷贝文件 
mv /data/pg/data/postgresql.conf /data/pg/data/postgresql.bak && cp ../db/postgresql/conf/postgresql.conf /data/pg/data/postgresql.conf

6、修改pg_hba.conf 
vim /data/pg/data/pg_hba.conf
# IPv4 local connections:
host   all             all             0.0.0.0/0            trust
#支持从库（可选）
host   replication     all             0.0.0.0/0            trust

或者拷贝文件 
mv /data/pg/data/pg_hba.conf /data/pg/data/pg_hba.bak && cp ../db/postgresql/conf/pg_hba.conf /data/pg/data/pg_hba.conf


7、操作数据库
启动(cd /data/pg/data)
su - postgres -c "/usr/local/pgsql/bin/pg_ctl -D/data/pg/data/  	start -l /data/pg/data/logfile"
停止
su - postgres -c "/usr/local/pgsql/bin/pg_ctl -D/data/pg/data/ 	stop"
重启数据库
su - postgres -c "/usr/local/pgsql/bin/pg_ctl -D/data/pg/data/    restart -l /data/pg/data/logfile"
su - postgres -c "/usr/local/pgsql/bin/pg_ctl -D/data/pg/data/    restart -m fast -l /data/pg/data/logfile"
重加载
su - postgres -c "/usr/local/pgsql/bin/pg_ctl reload   -D/data/pg/data/"
创建测试数据库
su - postgres -c "/usr/local/pgsql/bin/createdb test"

7、客户端连接
使用客户端Navicat for PostgreSQL 用户 postgres 密码 postgres可连接

8、安装插件(安装后需要重新启动数据库服务)
sudo ln -s /usr/local/pgsql/bin/pg_config /usr/bin/pg_config

安装默认控件
cd /app/thunder/db/postgresql/postgresql-10.5/contrib
make && sudo make install

需要安装的内置插件如下(如果执行默认安装插件指令则会直接都安装)
安装plpythonu （安装pg 时加选项  ./configure --with-python）
客户端连接pg后使用时安装
create extension IF NOT EXISTS plpythonu;

可选安装插件pg_trgm
cd /app/thunder/db/postgresql/postgresql-10.5/contrib/pg_trgm
make && sudo make install

可选安装插件pg_stat_statements
cd /app/thunder/db/postgresql/postgresql-10.5/contrib/pg_stat_statements
make && sudo make install
vim /data/pg/data/postgresql.conf
shared_preload_libraries = 'pg_stat_statements'
pg_stat_statements.max = 1000
pg_stat_statements.track = all
重加载 su - postgres -c "/usr/local/pgsql/bin/pg_ctl reload   -D/data/pg/data/"
客户端连接pg后使用时安装 
create extension IF NOT EXISTS pg_stat_statements; 

需要安装的第三方插件如下：
安装插件hll
git clone https://github.com/citusdata/postgresql-hll.git
cd postgresql-hll  
make && sudo make install  
客户端连接pg后使用时安装
CREATE EXTENSION IF NOT EXISTS hll;

安装插件pldebugger
git clone git://git.postgresql.org/git/pldebugger.git  
cd pldebugger  
export PATH=/usr/local/pgsql/bin:$PATH  
USE_PGXS=1 make clean  && USE_PGXS=1 make  && sudo USE_PGXS=1 make install  
修改配置(若已有则追加，以逗号,分隔so)
sudo vim /usr/local/pgsql/data/postgresql.conf  
shared_preload_libraries = 'plugin_debugger.so'  
客户端连接pg后使用时安装 
create extension IF NOT EXISTS pldbgapi;  

可选安装插件rum
git clone https://github.com/postgrespro/rum
cd rum && make USE_PGXS=1 && sudo make USE_PGXS=1 install && cd ..
重加载 
su - postgres -c "/usr/local/pgsql/bin/pg_ctl reload   -D/data/pg/data/"
客户端连接pg后使用时安装  
create extension IF NOT EXISTS rum;

可选安装插件jieba
wget  https://codeload.github.com/jaiminpan/pg_jieba/tar.gz/v1.0.1
tar xvf v1.0.1 && cd pg_jieba-1.0.1/ && USE_PGXS=1 make && sudo USE_PGXS=1 make install && cd ..
修改配置(若已有则追加，以逗号,分隔so)
sudo vim /data/pg/data/postgresql.conf 末尾添加
shared_preload_libraries = 'pg_jieba.so'
重加载 su - postgres -c "/usr/local/pgsql/bin/pg_ctl reload   -D/data/pg/data/"
客户端连接pg后使用时安装 
create extension IF NOT EXISTS pg_jieba;

postgresql.conf类似如下：
shared_preload_libraries = 'pg_jieba.so,plugin_debugger.so,pg_stat_statements.so,hll.so'
pg_stat_statements.max = 1000
pg_stat_statements.track = all

9、安装数据表
sh /app/thunder/db/postgresql/tables/loadsql.sh cre
sh /app/thunder/db/postgresql/func/loadfunc/loadfunc.sh

四、安装postgresql从库
1、创建数据目录
sudo mkdir -p /data/pg/data_backup   && sudo chown -R postgres:postgres /data/pg/data_backup && sudo chmod -R 0700 /data/pg/data_backup

2、安装数据库
su postgres  
拷贝数据(192.168.18.78改为源地址)
/usr/local/pgsql/bin/pg_basebackup -F p --progress -R -D /data/pg/data_backup -h 192.168.10.46 -p 5432 -U postgres --password 

3、修改postgresql.conf
vim /data/pg/data_backup/postgresql.conf
 listen_addresses = '*'
 port= 5433
 wal_level = replica #热备模式
 #max_wal_senders = 10 #从库不使用
 max_connections = 1000   #最大连接时间，主库必须要小于从库的配置
 wal_keep_segments = 256
 hot_standby = on #说明这台机器不仅用于数据归档，还可以用于数据查询
 max_standby_streaming_delay = 30s #流备份的最大延迟时间
 wal_receiver_status_interval = 10s  #向主机汇报本机状态的间隔时间
 hot_standby_feedback = on #r出现错误复制，向主机反馈
 
4、修改recovery.conf
vim /data/pg/data_backup/recovery.conf
standby_mode = 'on'
primary_conninfo = 'host=192.168.18.78 port=5432 user=postgres password=postgres sslmode=disable sslcompression=1 target_session_attrs=any'
recovery_target_timeline = 'latest'     #同步到最新数据

6、启动从库
启动(cd /usr/local/pgsql/data/)
su - postgres -c "/usr/local/pgsql/bin/pg_ctl -D/data/pg/data_backup/  	start -l /data/pg/data_backup/logfile"
停止
su - postgres -c "/usr/local/pgsql/bin/pg_ctl -D/data/pg/data_backup/ 	stop"
重启数据库
su - postgres -c "/usr/local/pgsql/bin/pg_ctl -D/data/pg/data_backup/    restart -l /data/pg/data_backup/logfile"
su - postgres -c "/usr/local/pgsql/bin/pg_ctl -D/data/pg/data_backup/    restart -m fast -l /data/pg/data_backup/logfile "

7.查看主从状态
主库
/usr/local/pgsql/bin/pg_controldata /data/pg/data |grep state
Database cluster state:               in production
从库
/usr/local/pgsql/bin/pg_controldata /data/pg/data_backup |grep state
Database cluster state:               in archive recovery
主库可读写，从库只读。在主库创建表和写表会同步从库。
ps -ef |grep "sender process"
postgres  5190  5175  0 17:18 ?        00:00:02 postgres: wal sender process postgres 192.168.18.78(51506) streaming 8D/928524C0
ps -ef |grep "receiver process"
postgres  5189  2543  0 17:18 ?        00:00:05 postgres: wal receiver process   streaming 8D/87E6E000

8.主从切换（一般不需要）
(1)主库上执行：
su - postgres
/usr/local/pgsql/bin/pg_ctl             -D/data/pg/data/     stop -m fast        	#停主库日志
/usr/local/pgsql/bin/pg_controldata     /data/pg/data/       #此时Database cluster state: shutdown
(2)从库切换为主库：
su - postgres
/usr/local/pgsql/bin/pg_ctl promote     -D/data/pg/data/
/usr/local/pgsql/bin/pg_controldata     /data/pg/data/       #此时Database cluster state: Inproduction

五、安装ssdb（如果是使用ssdb）
sudo mkdir -p /data/ssdb /data/ssdb/log /data/ssdb/var1 /data/ssdb/var2

cd /app/thunder/db/ssdb
unzip ssdb.zip

修改ssdb1.conf
vim /app/thunder/db/ssdb/ssdb/ssdb1.conf
host: 192.168.18.78
work_dir = /data/ssdb/var1
pidfile = /data/ssdb/var1/ssdb1.pid
output: /data/ssdb/log/log1.txt

修改ssdb2.conf
vim /app/thunder/db/ssdb/ssdb/ssdb2.conf
host: 192.168.18.78
work_dir = /data/ssdb/var2
pidfile = /data/ssdb/var2/ssdb2.pid
output: /data/ssdb/log/log2.txt

启动服务(root用户)
chmod +x /app/thunder/db/ssdb/ssdb/ssdb-server
sudo /app/thunder/db/ssdb/ssdb/ssdb-server -d /app/thunder/db/ssdb/ssdb/ssdb1.conf 
sudo /app/thunder/db/ssdb/ssdb/ssdb-server -d /app/thunder/db/ssdb/ssdb/ssdb2.conf 

重启
sudo /app/thunder/db/ssdb/ssdb/ssdb-server -d /app/thunder/db/ssdb/ssdb/ssdb1.conf -s restart
sudo /app/thunder/db/ssdb/ssdb/ssdb-server -d /app/thunder/db/ssdb/ssdb/ssdb2.conf -s restart

关闭
sudo /app/thunder/db/ssdb/ssdb/ssdb-server -d /app/thunder/db/ssdb/ssdb/ssdb1.conf -s stop
sudo /app/thunder/db/ssdb/ssdb/ssdb-server -d /app/thunder/db/ssdb/ssdb/ssdb2.conf -s stop

六、修改服务器配置
根据文档os.conf修改服务器配置

七、安装代码
拷贝thunder到/app目录

解压
cd /app/thunder/deploy
unzip 3lib.zip

编译
cd /app/thunder/code
sh ./make.sh pre && ./make.sh all

八、运行程序
创建数据目录（修改QueueCmd.json配置 "datalog_path":	"/data/thunder/file/"）
sudo mkdir -p /data/thunder/file && sudo chown -R dev:dev /data/thunder

cd /app/thunder/deploy

修改服务配置
sh install.sh pre
安装程序
./install.sh all
启动程序
./start_nodes.sh all
关闭程序
./stop_nodes.sh all
重启程序
./restart_nodes.sh all

