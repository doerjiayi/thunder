#!/bin/bash 
#清除文件
#`dirname $0`
RUN_PATH=`pwd`
cd ${RUN_PATH}

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input option[all,plugin,log,bin,lib]"
	exit 1; 
fi

if [ $1 == "all" ];then
	#log
	while read plugins log bin lib others
    do
        test ! -d ${RUN_PATH}${log} && mkdir -p ${RUN_PATH}${log} && echo "mkdir -p ${RUN_PATH}${log}"
        echo "find ${RUN_PATH}${log} -type f -name "*.log*" | xargs -i unlink {}"
        find ${RUN_PATH}${bin} -type f -name "*.log*" | xargs -i unlink {}
    done < clear_files_list.conf
    echo "succ to clear log"
	#plugin
	while read plugin log bin lib others
    do
        test ! -d ${RUN_PATH}${plugin} && mkdir -p ${RUN_PATH}${log} && echo "mkdir -p ${RUN_PATH}${log}"
        echo "find ${RUN_PATH}${plugin} -type f -name "*.so" | xargs -i unlink {}"
        find ${RUN_PATH}${plugin} -type f -name "*.so" | xargs -i unlink {}
    done < clear_files_list.conf
    echo "succ to clear plugin"
	#bin
	while read plugins log bin lib others
    do
        test ! -d ${RUN_PATH}${bin} && mkdir -p ${RUN_PATH}${bin} && echo "mkdir -p ${RUN_PATH}${bin}"
        echo "find ${RUN_PATH}${bin} -type f -name "*Server" | xargs -i unlink {}"
        find ${RUN_PATH}${bin} -type f -name "*Server" | xargs -i unlink {}
    done < clear_files_list.conf
    echo "succ to clear bin"
elif [ $1 == "plugin" ];then
    while read plugin log bin lib others
    do
        test ! -d ${RUN_PATH}${plugin} && mkdir -p ${RUN_PATH}${log} && echo "mkdir -p ${RUN_PATH}${log}"
        echo "find ${RUN_PATH}${plugin} -type f -name "*.so" | xargs -i unlink {}"
        find ${RUN_PATH}${plugin} -type f -name "*.so" | xargs -i unlink {}
    done < clear_files_list.conf
    echo "succ to clear plugin"
elif [ $1 == "log" ] ;then
    while read plugins log bin lib others
    do
        test ! -d ${RUN_PATH}${log} && mkdir -p ${RUN_PATH}${log} && echo "mkdir -p ${RUN_PATH}${log}"
        echo "find ${RUN_PATH}${log} -type f -name "*.log*" | xargs -i unlink {}"
        find ${RUN_PATH}${bin} -type f -name "*.log*" | xargs -i unlink {}
    done < clear_files_list.conf
    echo "succ to clear log"
elif [ $1 == "lib" ] ;then
    cd ${RUN_PATH}
    while read plugins log bin lib others
    do
        #librobot_proto.so,libloss.so,libthunder.so
        test ! -d ${RUN_PATH}${lib} && mkdir -p ${RUN_PATH}${lib} && echo "mkdir -p ${RUN_PATH}${lib}"
        test -f ${RUN_PATH}${lib}/librobot_proto.so && echo "unlink ${RUN_PATH}${lib}/librobot_proto.so" && unlink ${RUN_PATH}${lib}/librobot_proto.so
        test -f ${RUN_PATH}${lib}/libloss.so && echo "unlink ${RUN_PATH}${lib}/libloss.so" && unlink ${RUN_PATH}${lib}/libloss.so
        test -f ${RUN_PATH}${lib}/libthunder.so && echo "unlink ${RUN_PATH}${lib}/libthunder.so" && unlink ${RUN_PATH}${lib}/libthunder.so
    done < clear_files_list.conf
    echo "succ to clear lib"
elif [ $1 == "bin" ] ;then
    while read plugins log bin lib others
    do
        test ! -d ${RUN_PATH}${bin} && mkdir -p ${RUN_PATH}${bin} && echo "mkdir -p ${RUN_PATH}${bin}"
        echo "find ${RUN_PATH}${bin} -type f -name "*Server" | xargs -i unlink {}"
        find ${RUN_PATH}${bin} -type f -name "*Server" | xargs -i unlink {}
    done < clear_files_list.conf
    echo "succ to clear bin"
else
    echo "nothings to done"
fi
