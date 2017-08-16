#!/bin/bash

THUNDER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${THUNDER_HOME}
THUNDER_HOME=`pwd`

test ! -d ${THUNDER_HOME}/../deploy/3lib && ln -s  ${THUNDER_HOME}/l3lib/lib  ${THUNDER_HOME}/../deploy/3lib

if [ $# -ge 1 ];then
	if [ $1 == "node" ];then
		echo "compile node..."
		cd ${THUNDER_HOME}
		cd ./HelloThunder/src && make clean && make  
	else
		echo "compile all..."
		cd ${THUNDER_HOME}
		cd ./llib/src && make clean && make 
		
		cd ${THUNDER_HOME}
		cd ./thunder/src && make clean && make  
		
		cd ${THUNDER_HOME}
		cd ./HelloThunder/src && make clean && make  
	fi
else
	echo "compile all..."
	cd ${THUNDER_HOME}
	cd ./llib/src && make clean && make 
	
	cd ${THUNDER_HOME}
	cd ./thunder/src && make clean && make  
	
	cd ${THUNDER_HOME}
	cd ./HelloThunder/src && make clean && make  
fi



