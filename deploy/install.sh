#!/bin/bash 
#RUN_PATH=`dirname $0`
RUN_PATH=`pwd`
cd ${RUN_PATH}
lib3_path=/app/robot/robotServer/deploy/3lib

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input option[all,plugin,lib]"
	exit 1; 
fi

if [ $1 == "all" ];then
	./install_bins.sh all && ./install_libs.sh all && ./install_plugins.sh all
elif [ $1 == "plugin" ];then
	./install_plugins.sh all
elif [ $1 == "lib" ];then
	./install_libs.sh all
elif [ $1 == "pre" ];then
	find ./ -maxdepth 3 -type f -name "*.sh"  |xargs -i chmod +x {}
	test ! -d ./3lib  && test -d ${lib3_path} && ln -s ${lib3_path} ${RUN_PATH}/3lib 
else
	echo "do nothings"
fi