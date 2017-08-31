#!/bin/bash

ROBOT_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${ROBOT_HOME}
ROBOT_HOME=`pwd`

SIEGE_BIN=/usr/local/bin/siege
SIEGE_COMMAND="http://192.168.18.68:21000/hello"

FILE_DATA=`cat ./postfile`
echo "length:"
echo "${FILE_DATA}" |wc -L

if [ $# == 1 ]; then 
	if [ $1 == "one" ];then
		${SIEGE_BIN} -c 1 -r 1 "${SIEGE_COMMAND} POST ${FILE_DATA}"
		exit 0
	fi
fi

${SIEGE_BIN} -c 300 -r 300 "${SIEGE_COMMAND} POST ${FILE_DATA}"
