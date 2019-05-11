#!/bin/bash
SERVER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${SERVER_HOME}
SERVER_HOME=`pwd`

HOST=
#FreeBSD/OpenBSD
test -z "${HOST}" && HOST=`ifconfig|grep -E 'inet.[0-9]'|grep -v '127.'|awk '{print $2}'|awk 'NR==1{print}'`
#linux
test -z "${HOST}" && HOST=`ifconfig |grep "inet addr" |grep "192."|grep -o "addr:[0-9.]\{1,\}"|cut -d: -f2|awk 'NR==1{print}'`

#清空pg数据
function clear_pg()
{
	echo "try to clear_pg"
	USER=postgres
	PASS=postgres
	PORT=5432
	PSQL=/usr/local/pgsql/bin/psql
	
	DBS="db_analysis3_offline db_analysis3_online"
	
	#被调用的函数_truncate_all_tables（在函数初始化时加载）
	dbs=($DBS)
	echo "dbs $dbs"
	for dbname in ${dbs[@]} 
	do
		echo "dbname:${dbname}"
		$PSQL -h $HOST -p $PORT  -U $USER ${dbname} -c "select _truncate_all_tables()"
		if [ $? -eq 0 ];then
			echo "succ to truncate tables for ${dbname}"	
		else
			echo "failed to truncate tables for ${dbname}"	
		fi
	done
}

#清空pg某一天统计数据
function clear_pg_one_day_statistics()
{
	echo "try to clear_pg"
	USER=postgres
	PASS=postgres
	PORT=5432
	PSQL=/usr/local/pgsql/bin/psql
	
	#被调用的函数_clear_data_offline _clear_data_online（在函数初始化时加载）
	dbname=db_analysis3_offline
	echo "dbname:${dbname}"
	$PSQL -h $HOST -p $PORT  -U $USER ${dbname} -c "select _clear_data_offline(current_date)"
	if [ $? -eq 0 ];then
		echo "succ to truncate tables for ${dbname}"	
	else
		echo "failed to truncate tables for ${dbname}"	
	fi
	
	dbname=db_analysis3_online
	echo "dbname:${dbname}"
	$PSQL -h $HOST -p $PORT  -U $USER ${dbname} -c "select _clear_data_online(current_date)"
	if [ $? -eq 0 ];then
		echo "succ to truncate tables for ${dbname}"	
	else
		echo "failed to truncate tables for ${dbname}"	
	fi
}

function clear_pg_statistics()
{
	echo "try to clear_pg"
	USER=postgres
	PASS=postgres
	PORT=5432
	PSQL=/usr/local/pgsql/bin/psql
	
	#被调用的函数_clear_data_offline _clear_data_online（在函数初始化时加载）
	dbname=db_analysis3_offline
	echo "dbname:${dbname}"
	$PSQL -h $HOST -p $PORT  -U $USER ${dbname} -c "select _clear_data_offline(null)"
	if [ $? -eq 0 ];then
		echo "succ to truncate tables for ${dbname}"	
	else
		echo "failed to truncate tables for ${dbname}"	
	fi
	
	dbname=db_analysis3_online
	echo "dbname:${dbname}"
	$PSQL -h $HOST -p $PORT  -U $USER ${dbname} -c "select _clear_data_online(null)"
	if [ $? -eq 0 ];then
		echo "succ to truncate tables for ${dbname}"	
	else
		echo "failed to truncate tables for ${dbname}"	
	fi
}

function flush_ssdb()
{
	#使用双主模式的不能只用此操作
	PORT=7000
	echo "input:flushdb "
	/app/thunder/db/ssdb/ssdb/tools/ssdb-cli -h $HOST -p $PORT
}

#清空ssdb数据
function clear_ssdb()
{
	echo "try to clear_ssdb"
	CLIENT=/app/thunder/db/redis/bin/redis-cli
	PORT=7000
	SIZE=1000000
	
	chmod +x $CLIENT
	echo "$CLIENT  -h $HOST -p $PORT"
	#$CLIENT  -h $HOST -p $PORT  hlist "" "" $SIZE
	$CLIENT  -h $HOST -p $PORT  hlist "" "" $SIZE  | cut -d ' ' -f 2| xargs -i $CLIENT  -h $HOST -p $PORT  hclear {} 1>/dev/null 
	
	if [ $? -eq 0 ];then
		echo "succ to clear hlist"	
	else
		echo "failed to clear hlist"	
	fi
	
	#$CLIENT  -h $HOST -p $PORT  scan "" "" $SIZE 
	$CLIENT  -h $HOST -p $PORT  scan "" "" $SIZE | cut -d ' ' -f 2| xargs -i $CLIENT  -h $HOST -p $PORT  del {} 1>/dev/null 
	
	if [ $? -eq 0 ];then
		echo "succ to clear scan"	
	else
		echo "failed to clear scan"	
	fi
}


function check_ssdb_hlist()
{
	echo "try to check_ssdb hlist"
	CLIENT=/app/analysis3/db/redis/bin/redis-cli
	PORT=7000
	SIZE=1000000
	
	chmod +x $CLIENT
	echo "$CLIENT  -h $HOST -p $PORT"
	$CLIENT  -h $HOST -p $PORT  hlist "" "" $SIZE
	if [ $? -eq 0 ];then
		echo "succ to hlist"	
	else
		echo "failed to hlist"	
	fi
}

function check_ssdb_scan()
{
	echo "try to check_ssdb hlist"
	CLIENT=/app/analysis3/db/redis/bin/redis-cli
	PORT=7000
	SIZE=1000000
	
	chmod +x $CLIENT
	echo "$CLIENT  -h $HOST -p $PORT"
	
	$CLIENT  -h $HOST -p $PORT  scan "" "" $SIZE 
	if [ $? -eq 0 ];then
		echo "succ to scan"	
	else
		echo "failed to scan"	
	fi
}

if [ "$1"x == "clearall"x ];then
	clear_pg
	clear_ssdb
elif [ "$1"x == "clearpg"x ];then
	clear_pg
elif [ "$1"x == "clearssdb"x ];then
	clear_ssdb
elif [ "$1"x == "checkssdb"x ];then
	check_ssdb_hlist
	check_ssdb_scan
elif [ "$1"x == "checkssdb_hlist"x ];then
	check_ssdb_hlist
elif [ "$1"x == "checkssdb_scan"x ];then
	check_ssdb_scan
elif [ "$1"x == "clearpg_statistics"x ];then
	clear_pg_statistics
elif [ "$1"x == "clear_pg_one_day_statistics"x ];then
	clear_pg_one_day_statistics
else
	echo "no such cmd.try (checkssdb\checkssdb_hlist\checkssdb_scan\clearssdb\clearpg\clearall\clearpg_statistics\clear_pg_one_day_statistics)"
fi
	
	