#!/bin/bash

SERVER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${SERVER_HOME}
SERVER_HOME=`pwd`

test ! -d ${SERVER_HOME}/../deploy/3lib && ln -s  ${SERVER_HOME}/l3lib/lib  ${SERVER_HOME}/../deploy/3lib

compile_nodes=""
if [ $# -ge 1 ];then
	if [ $1 == "node" ];then
		compile_nodes="node" 
	fi
fi

echo "compiling:${compile_nodes}"
if [ "${compile_nodes}" == "node" ];then
	echo "compile node..."
	cd ${SERVER_HOME}/HelloThunder/src && make clean && make  
else
	echo "compile all..."
	cd ${SERVER_HOME}/llib/src && make clean && make 
	cd ${SERVER_HOME}/thunder/src && make clean && make  
	cd ${SERVER_HOME}/HelloThunder/src && make clean && make  
fi

