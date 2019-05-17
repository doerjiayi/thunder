#!/bin/bash

SERVER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${SERVER_HOME}
SERVER_HOME=`pwd`

SERVER_BIN=${SERVER_HOME}/bin
SERVER_CONF=${SERVER_HOME}/conf
SERVER_CONF2=${SERVER_HOME}/conf2
SERVER_LIB=${SERVER_HOME}/../lib  
SERVER_3LIB=${SERVER_HOME}/../3lib
SERVER_LOG=${SERVER_HOME}/log
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${SERVER_LIB}:${SERVER_3LIB}

LOG_FILE="${SERVER_HOME}/log/${SCRIPT_NAME}.log"

. ${SERVER_HOME}/scripts/script_func.sh

server_bin_files=`ls ${SERVER_BIN}/`
for server_bin in $server_bin_files
do
    if [ -f "${SERVER_CONF}/${server_bin}.json" ]
    then
    	echo "centerserver no 1"
        target_server=`awk -F'"server_name"' '/server_name/{print $2}'  ${SERVER_CONF}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $3}'`
        #echo "target_server:$target_server"
        target_server_tag=`echo "$target_server" | awk '{print substr($0,0,20)}'`
        #echo "target_server_tag:$target_server_tag"
        target_port=`awk -F'"inner_port"' '/inner_port/{print $2}'  ${SERVER_CONF}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $2}'`
        #echo "target_port:$target_port"
        running_target_server_pid=`netstat -apn 2>>/dev/null | grep -w $target_port | grep $target_server_tag | awk -F/ '/^tcp/{print $1}' | awk '/LISTEN/{print $NF}'`
        #echo "running_target_server_pid:$running_target_server_pid"
        if [ -z "$running_target_server_pid" ]
        then
            ${SERVER_BIN}/$server_bin ${SERVER_CONF}/$server_bin.json
            if [ $? -eq 0 ]
            then
                info_log "${SERVER_HOME}/bin/$server_bin start successfully."
            else
                error_log "failed to start $server_bin"
            fi
            ps -ef | awk -vpname=$target_server '{idx=index($8,pname); if (idx == 1)print}'
        else
            info_log "the server process for $server_bin was exist!"
        fi
    fi
    
    #热备份节点
    if [ -f "${SERVER_CONF2}/${server_bin}.json" ]
    then
    	echo "centerserver no 2"
        target_server=`awk -F'"server_name"' '/server_name/{print $2}'  ${SERVER_CONF2}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $3}'`
        #echo "target_server:$target_server"
        target_server_tag=`echo "$target_server" | awk '{print substr($0,0,20)}'`
        #echo "target_server_tag:$target_server_tag"
        target_port=`awk -F'"inner_port"' '/inner_port/{print $2}'  ${SERVER_CONF2}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $2}'`
        #echo "target_port:$target_port"
        running_target_server_pid=`netstat -apn 2>>/dev/null | grep -w $target_port | grep $target_server_tag | awk -F/ '/^tcp/{print $1}' | awk '/LISTEN/{print $NF}'`
        #echo "running_target_server_pid:$running_target_server_pid"
        if [ -z "$running_target_server_pid" ]
        then
            ${SERVER_BIN}/$server_bin ${SERVER_CONF2}/$server_bin.json
            if [ $? -eq 0 ]
            then
                info_log "${SERVER_HOME}/bin/$server_bin start successfully."
            else
                error_log "failed to start $server_bin"
            fi
            ps -ef | awk -vpname=$target_server '{idx=index($8,pname); if (idx == 1)print}'
        else
            info_log "the server process for $server_bin was exist!"
        fi
    fi
done

