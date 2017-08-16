#! /bin/bash 

THUNDER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${THUNDER_HOME}
THUNDER_HOME=`pwd`

THUNDER_BIN=${THUNDER_HOME}/bin
THUNDER_CONF=${THUNDER_HOME}/conf
THUNDER_LIB=${THUNDER_HOME}/lib  THUNDER_3LIB=${THUNDER_HOME}/3lib
THUNDER_LOG=${THUNDER_HOME}/log
THUNDER_TEMP=${THUNDER_HOME}/temp
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${THUNDER_LIB}:${THUNDER_3LIB}

LOG_FILE="${THUNDER_HOME}/log/${SCRIPT_NAME}.log"

. ${THUNDER_HOME}/scripts/analysis_script_func.sh

if [ $# -ge 1 ]
then
    for anslysis_bin in $@
    do
        if [ ! -f "${THUNDER_BIN}/$anslysis_bin" ]
        then
            echo "error: ${THUNDER_BIN}/$anslysis_bin not exist!"
        elif [ ! -f "${THUNDER_CONF}/$anslysis_bin.json" ]
        then
            echo "error: ${THUNDER_BIN}/$anslysis_bin.json not exist!"
        else
            robot_server=`awk -F'"server_name"' '/server_name/{print $2}'  ${THUNDER_CONF}/${anslysis_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $3}'`
            robot_port=`awk -F'"inner_port"' '/inner_port/{print $2}'  ${THUNDER_CONF}/${anslysis_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $2}'`
            robot_server_tag=`echo "$robot_server" | awk '{print substr($0,0,9)}'`
            echo ${robot_server_tag}
            echo ${robot_port}   $robot_server_tag 
            echo `netstat -apn 2>>/dev/null | grep  $robot_port | grep $robot_server_tag| awk -F/ '/^tcp/{print $1}' | awk '/LISTEN/{print $NF}' `
                running_robot_pid=`netstat -apn 2>>/dev/null  | grep -w $robot_port | grep $robot_server_tag | awk -F/ '/^tcp/{print $1}' | awk '/LISTEN/{print $NF}'`
            if [ -n "$running_robot_pid" ]
            then
                echo "kill $running_robot_pid    $robot_server" 
                kill $running_robot_pid
            fi
        fi
    done
    exit 0
fi


echo "do you want to stop robot server process ${THUNDER_HOME}? [yes|no]"
read stop_robot_server
if [ "yes" != "$stop_robot_server" ]
then
    echo "cancel"
    exit 0
fi
im_servre_bin_files=`ls ${THUNDER_BIN}/`
for anslysis_bin in $im_servre_bin_files
do
    if [ -f "${THUNDER_CONF}/${anslysis_bin}.json" ]
    then
        robot_server=`awk -F'"server_name"' '/server_name/{print $2}'  ${THUNDER_CONF}/${anslysis_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $3}'`
        robot_port=`awk -F'"inner_port"' '/inner_port/{print $2}'  ${THUNDER_CONF}/${anslysis_bin}.json | sed 's/ //g' | awk -F'[:",]' '{print $2}'`
        robot_server_tag=`echo "$robot_server" | awk '{print substr($0,0,9)}'`
        echo ${robot_server_tag}
        running_robot_pid=`netstat -apn 2>>/dev/null | grep -w $robot_port | grep $robot_server_tag | awk -F/ '/^tcp/{print $1}' | awk '/LISTEN/{print $NF}'`
        if [ -n "$running_robot_pid" ]
        then
            echo "kill $running_robot_pid    $robot_server"
            kill $running_robot_pid
        fi
    fi
done

