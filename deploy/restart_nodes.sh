#!/bin/bash

SERVER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${SERVER_HOME}
SERVER_HOME=`pwd`

if [ $# -ne 1 ]; then 
	#参数说明
    echo "USAGE: $0 param1(servername)" 
    echo "All"
    while read server others
    do 
    	echo "${server}"
    done < server_list.conf
	exit 1; 
fi

#准备工作
find ./ -maxdepth 3 -type f -name "*.sh"  |xargs -i chmod +x {}

if [ "$1"x == "reload"x ]
then
    while read server others
    do
        ${SERVER_HOME}/${server}/restart.sh "reload"
    done < server_list.conf
elif [ "$1"x == "all"x ]
then
    while read server others
    do
        ${SERVER_HOME}/${server}/stop.sh "yes"
        ${SERVER_HOME}/${server}/start.sh
    done < server_list.conf
else
    while read server others
    do
        test $1 == ${server} &&\
        ${SERVER_HOME}/$1/stop.sh "yes" &&\
        ${SERVER_HOME}/$1/start.sh &&\
        echo "restart ${server} ok" && exit 0
    done < server_list.conf
    echo "USAGE: $0 param1(servername)" 
    echo "All"
    while read server others
    do 
    	echo "${server}"
    done < server_list.conf
fi


