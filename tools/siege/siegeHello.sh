#!/bin/bash
ROBOT_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${ROBOT_HOME}
ROBOT_HOME=`pwd`
IP="192.168.18.68"

SIEGE_BIN=`command -v /usr/local/bin/siege >/dev/null 2>&1  && echo "/usr/local/bin/siege" || echo "/usr/bin/siege"`
SIEGE_COMMAND="http://${IP}:21000/hello"

FILE_DATA=`cat ./postfile`
echo "length:"
echo "${FILE_DATA}" |wc -L

if [ $# -ge 1 ]; then 
	if [ "$1"x == "one"x ]; then 
		${SIEGE_BIN} -c 1 -r 1 "${SIEGE_COMMAND} POST ${FILE_DATA}"
	elif [[ $1 != *[!0-9]* ]]; then
		${SIEGE_BIN} -c 1 -r $1 "${SIEGE_COMMAND} POST ${FILE_DATA}"
	else
		echo "do nothings"
	fi
else
	${SIEGE_BIN} -c 300 -r 300 "${SIEGE_COMMAND} POST ${FILE_DATA}"
fi


