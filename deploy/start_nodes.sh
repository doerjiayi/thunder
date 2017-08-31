#!/bin/bash

SERVER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${SERVER_HOME}
SERVER_HOME=`pwd`

if [ $# != 1 ]; then 
    echo "USAGE: $0 param1(servername)" 
    while read server others
    do 
    	echo "${server}"
    done < server_list.conf
    exit 1; 
fi

if [ $1 == "all" ]
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
	echo "USAGE: $0 param1(servername):" 
    while read server others
    do 
    	echo "${server}"
    done < server_list.conf
fi



