#!/bin/bash

THUNDER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${THUNDER_HOME}
THUNDER_HOME=`pwd`

THUNDER_BIN=${THUNDER_HOME}/bin
THUNDER_CONF=${THUNDER_HOME}/conf
THUNDER_LIB=${THUNDER_HOME}/../lib  THUNDER_3LIB=${THUNDER_HOME}/../3lib
THUNDER_LOG=${THUNDER_HOME}/log
THUNDER_TEMP=${THUNDER_HOME}/temp
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${THUNDER_LIB}:${THUNDER_3LIB}

LOG_FILE="${THUNDER_HOME}/log/${SCRIPT_NAME}.log"

. ${THUNDER_HOME}/scripts/server_script_func.sh

server_list=""
if [ $# -ge 1 ]
then
    for server_bin in $@
    do
        if [ ! -f "${THUNDER_BIN}/$server_bin" ]
        then
            echo "error: ${THUNDER_BIN}/$server_bin not exist!"
        elif [ ! -f "${THUNDER_CONF}/$server_bin.json" ]
        then
            echo "error: ${THUNDER_BIN}/$server_bin.json not exist!"
        else
        	echo "try to exec ${THUNDER_BIN}/$server_bin  ${THUNDER_CONF}/${server_bin}.json"
            robot_server=`awk -F'"server_name"' '/server_name/{print $2}'  ${THUNDER_CONF}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $3}'`
            robot_server_tag=`echo "$robot_server" | awk '{print substr($0,0,9)}'`
            robot_port=`awk -F'"inner_port"' '/inner_port/{print $2}'  ${THUNDER_CONF}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $2}'`
            running_robot_pid=`netstat -apn 2>>/dev/null | grep -w $robot_port | grep $robot_server_tag | awk -F/ '/^tcp/{print $1}' | awk '/LISTEN/{print $NF}'`
            #echo "running_robot_pid:$running_robot_pid"
            if [ -z "$running_robot_pid" ]
            then
                ${THUNDER_BIN}/$server_bin ${THUNDER_CONF}/$server_bin.json
                if [ $? -eq 0 ]
                then
                    info_log "${THUNDER_HOME}/bin/$server_bin start successfully."
                else
                    error_log "failed to start $server_bin"
                fi
                ps -ef | awk -vpname=$robot_server '{idx=index($8,pname); if (idx == 1)print}'
            else
                info_log "the thunder process for $server_bin was exist!"
            fi
        fi
    done
    sleep 1
    for s in $server_list
    do
        ps -ef | awk -vpname=$s '{idx=index($8,pname); if (idx == 1)print}'
    done
    exit 0
fi


im_servre_bin_files=`ls ${THUNDER_BIN}/`
for server_bin in $im_servre_bin_files
do
    if [ -f "${THUNDER_CONF}/${server_bin}.json" ]
    then
    	echo "try to exec ${THUNDER_BIN}/$server_bin  ${THUNDER_CONF}/${server_bin}.json"
        robot_server=`awk -F'"server_name"' '/server_name/{print $2}'  ${THUNDER_CONF}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $3}'`
        robot_server_tag=`echo "$robot_server" | awk '{print substr($0,0,9)}'`
        robot_port=`awk -F'"inner_port"' '/inner_port/{print $2}'  ${THUNDER_CONF}/${server_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $2}'`
        running_robot_pid=`netstat -apn 2>>/dev/null | grep -w $robot_port | grep $robot_server_tag | awk -F/ '/^tcp/{print $1}' | awk '/LISTEN/{print $NF}'`
        #echo "running_robot_pid:$running_robot_pid"
        if [ -z "$running_robot_pid" ]
        then
            ${THUNDER_BIN}/$server_bin ${THUNDER_CONF}/$server_bin.json
            if [ $? -eq 0 ]
            then
                info_log "${THUNDER_HOME}/bin/$server_bin start successfully."
            else
                error_log "failed to start $server_bin"
            fi
            ps -ef | awk -vpname=$robot_server '{idx=index($8,pname); if (idx == 1)print}'
        else
            info_log "the thunder process for $server_bin was exist!"
        fi
    else 
    	echo "${THUNDER_CONF}/${server_bin}.json not exist.pass it"
    fi
done
sleep 1
for s in $server_list
do
    ps -ef | awk -vpname=$s '{idx=index($8,pname); if (idx == 1)print}'
done

