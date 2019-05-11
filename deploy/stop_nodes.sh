#!/bin/bash

SERVER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${SERVER_HOME}
SERVER_HOME=`pwd`

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1(servername)" 
    while read server others
    do 
    	echo "${server}"
    done < server_list.conf
    echo "All"
    exit 1; 
fi

server_name=_robot

if [ $1 == "all" ]
then
    while read server others
    do
        ${SERVER_HOME}/${server}/stop.sh "yes"
    done < server_list.conf
elif [ "$1"x == "kill"x ];then
	ps -ef |grep $server_name|grep -v grep|awk '{print $2}'|xargs kill 
	ps -ef |grep $server_name
else
    while read server others
    do
        test $1 == ${server} &&\
        ${SERVER_HOME}/$1/stop.sh "yes"&&\
        echo "stop ${server} ok" && exit 0
    done < server_list.conf
	echo "USAGE: $0 param1(servername)" 
    while read server others
    do 
    	echo "${server}"
    done < server_list.conf
fi



