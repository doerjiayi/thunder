#!/bin/bash

SERVER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${SERVER_HOME}
SERVER_HOME=`pwd`

function list_server()
{
	echo "USAGE: $0 param1(servername):" 
    while read server others
    do 
    	echo "${server}"
    done < server_list.conf
}

if [ $# != 1 ]; then 
    list_server
    exit 1; 
fi

#准备工作
find ./ -maxdepth 3 -type f -name "*.sh"  |xargs -i chmod +x {}

if [ "$1"x == "all"x ]
then
    while read server others
    do
        ${SERVER_HOME}/${server}/start.sh
    done < server_list.conf
else
    while read server others
    do
        test $1 == ${server} &&\
        ${SERVER_HOME}/$1/start.sh &&\
        echo "start ${server} ok" && exit 0
    done < server_list.conf
	list_server
fi



