#!/bin/bash 
#清除文件
#`dirname $0`
RUN_PATH=`pwd`
cd ${RUN_PATH}
config_file="server_dir.conf"
log="/log"

if [ $# -ne 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input option[all,plugin,log,bin]"
	exit 1; 
fi

if [ "$1"x == "all"x ];then
	#log
	while read nodetype plugin others
    do
        set -x `find ${RUN_PATH}${bin} -maxdepth 3 -type f -name "*.log*" | xargs -i unlink {}`
        
    done < ${config_file}
    echo "succ to clear log"
	#plugin
	while read nodetype plugin others
    do
        set -x `find ${RUN_PATH}${plugin} -maxdepth 3  -type f -name "*.so" | xargs -i unlink {}`
    done < ${config_file}
    set -x `find ./plugins -maxdepth 3  -type f -name "*.so" | xargs -i unlink {}`
    echo "succ to clear plugin"
	#bin
	while read nodetype plugin  others
    do
        set -x `find ${RUN_PATH}/${nodetype}/bin -maxdepth 3  -type f -name "*" | xargs -i unlink {}`
    done < ${config_file}
    echo "succ to clear bin"
elif [ "$1"x == "plugin"x ];then
    while read nodetype plugin  others
    do
        set -x `find ${RUN_PATH}${plugin} -type f -name "*.so" | xargs -i unlink {}`
    done < ${config_file}
    echo "succ to clear plugin"
elif [ "$1"x == "log"x ] ;then
    while read nodetype plugin  others
    do
        set -x `find ${RUN_PATH}/${nodetype}${log} -type f -name '*.log*'| xargs -i unlink {}` 
    done < ${config_file}
    echo "succ to clear log"
elif [ "$1"x == "bin"x ] ;then
    while read nodetype plugin  others
    do
        set -x `find ${RUN_PATH}/${nodetype}/bin -type f -name "*" | xargs -i unlink {}`
    done < ${config_file}
    echo "succ to clear bin"
elif [ "$1"x == "core"x ];then
	find ./ -maxdepth 3 -type f -name "core*"  |xargs -i rm {}
else
    echo "nothings to done"
fi
