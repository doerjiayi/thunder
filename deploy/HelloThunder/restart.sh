#!/bin/bash

SERVER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${SERVER_HOME}
SERVER_HOME=`pwd`

SERVER_BIN=${SERVER_HOME}/bin
SERVER_CONF=${SERVER_HOME}/conf
SERVER_LIB=${SERVER_HOME}/../lib  
SERVER_3LIB=${SERVER_HOME}/../3lib
SERVER_LOG=${SERVER_HOME}/log

if [ $# == 1 ]; then 
	if [ $1 == "reload" ];then
		server_bin_files=`ls ${SERVER_BIN}/`
		for server_bin in $server_bin_files
		do
		    if [ -f "${SERVER_CONF}/${server_bin}.json" ]
		    then
		        target_server=`awk -F'"server_name"' '/server_name/{print $2}'  ${SERVER_CONF}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $3}'`
		        target_port=`awk -F'"inner_port"' '/inner_port/{print $2}'  ${SERVER_CONF}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $2}'`
		        target_server_tag=`echo "$target_server" | awk '{print substr($0,0,9)}'`
		        echo "reload ${target_server_tag} so..."
		        running_target_server_pid=`netstat -apn 2>>/dev/null | grep -w $target_port | grep $target_server_tag | awk -F/ '/^tcp/{print $1}' | awk '/LISTEN/{print $NF}'`
		        if [ -n "$running_target_server_pid" ]
		        then
		            echo "reload so. kill -SIGUSR1 server：$target_server pid:$running_target_server_pid"
		            kill -SIGUSR1 $running_target_server_pid
		        fi
		    fi
		done
		echo "reload server done."
		exit 0
	fi
	echo "wrong command.$1"
	exit 0
fi

${SERVER_HOME}/stop.sh
${SERVER_HOME}/start.sh

