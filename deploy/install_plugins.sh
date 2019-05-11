#!/bin/bash 
#cmd so文件安装
#`dirname $0`
RUN_PATH=`pwd`
cd ${RUN_PATH}
config_file="server_dir.conf"

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input option[all,clean]"
	exit 1; 
fi

function print_so()
{
	while read nodetype dest_path src_path others
    do 
        echo "${nodetype}:"
        #输出安装前文件
        find  ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
        #输出安装后文件
        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
    done < ${config_file}
}

if [ "$1"x == "all"x ];then
    cd ${RUN_PATH}
    while read nodetype dest_path src_path others
    do
    	test ! -d ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path} &&　echo "mkdir -p ${RUN_PATH}${dest_path}"
        echo "find ${RUN_PATH}${src_path} -type f -name '*.so' | xargs -i install -vD {} ${RUN_PATH}${dest_path}"
        find ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i install -vD {} ${RUN_PATH}${dest_path}
    done < ${config_file}
    print_so
elif [ "$1"x == "clean"x ] ;then
    cd ${RUN_PATH}
    while read nodetype dest_path src_path others
    do
        echo "find ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i unlink {}"
        find ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i unlink {}
    done < ${config_file}
else
	#安装指定node的库文件
    cd ${RUN_PATH}
    echo "try to intall $1"
    while read nodetype dest_path src_path others
    do
    	test ! -d ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path} &&　echo "mkdir -p ${RUN_PATH}${dest_path}"
        echo "checking ${RUN_PATH}${dest_path}"
        test ${nodetype} == $1 &&\
        echo "install ${RUN_PATH}${src_path} to ${RUN_PATH}${dest_path}" &&\
        find ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i install -vD {} ${RUN_PATH}${dest_path} &&\
        echo "${nodetype}:" &&\
        find  ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i ls -l --color=tty {} &&\
        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {} &&\
        echo "done" && exit 0
    done < ${config_file}
    echo "nothings to install"
fi
