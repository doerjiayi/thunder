#! /bin/bash 

SERVER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${SERVER_HOME}
SERVER_HOME=`pwd`

SERVER_BIN=${SERVER_HOME}/bin
SERVER_CONF=${SERVER_HOME}/conf
#SERVER_CONF2=${SERVER_HOME}/conf2
SERVER_LIB=${SERVER_HOME}/../lib  
SERVER_3LIB=${SERVER_HOME}/../3lib
SERVER_LOG=${SERVER_HOME}/log
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${SERVER_LIB}:${SERVER_3LIB}

LOG_FILE="${SERVER_HOME}/log/${SCRIPT_NAME}.log"

. ${SERVER_HOME}/scripts/analysis_script_func.sh

if [ $# == 0 ]; then 
	echo "do you want to stop robot server process ${SERVER_HOME}? [yes|no]"
	read stop_robot_server
	if [ "yes" != "$stop_robot_server" ]
	then
	    echo "cancel"
	    exit 0
	fi
fi

server_bin_files=`ls ${SERVER_BIN}/`
for server_bin in $server_bin_files
do
    if [ -f "${SERVER_CONF}/${server_bin}.json" ]
    then
        target_server=`awk -F'"server_name"' '/server_name/{print $2}'  ${SERVER_CONF}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $3}'`
        target_port=`awk -F'"inner_port"' '/inner_port/{print $2}'  ${SERVER_CONF}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $2}'`
        target_server_tag=`echo "$target_server" | awk '{print substr($0,0,10)}'`
        echo ${target_server_tag}
        #echo $target_port
        running_target_server_pid=`netstat -apn 2>>/dev/null | grep -w $target_port | grep $target_server_tag | awk -F/ '/^tcp/{print $1}' | awk '/LISTEN/{print $NF}'`
        #echo "running_target_server_pid:$running_target_server_pid"
        if [ -n "$running_target_server_pid" ]
        then
            echo "kill $running_target_server_pid    $target_server"
            kill $running_target_server_pid
        fi
    fi
    #热备份节点
    if [ -f "${SERVER_CONF2}/${server_bin}.json" ]
    then
    	target_server=`awk -F'"server_name"' '/server_name/{print $2}'  ${SERVER_CONF2}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $3}'`
        target_port=`awk -F'"inner_port"' '/inner_port/{print $2}'  ${SERVER_CONF2}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $2}'`
        target_server_tag=`echo "$target_server" | awk '{print substr($0,0,10)}'`
        echo ${target_server_tag}
        running_target_server_pid=`netstat -apn 2>>/dev/null | grep -w $target_port | grep $target_server_tag | awk -F/ '/^tcp/{print $1}' | awk '/LISTEN/{print $NF}'`
        if [ -n "$running_target_server_pid" ]
        then
            echo "kill $running_target_server_pid    $target_server"
            kill $running_target_server_pid
        fi
    fi
done

