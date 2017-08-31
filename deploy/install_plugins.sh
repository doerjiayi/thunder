#!/bin/bash 
#cmd so文件安装
#`dirname $0`
RUN_PATH=`pwd`
cd ${RUN_PATH}

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input option[all,clean]"
	exit 1; 
fi

if [ $1 == "all" ];then
    cd ${RUN_PATH}
    while read src_path dest_path nodetype others
    do
        test ! -d ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path} &&　echo "mkdir -p ${RUN_PATH}${dest_path}"
        echo "find ${RUN_PATH}${src_path} -type f -name '*.so' | xargs -i install -v {} ${RUN_PATH}${dest_path}"
        find ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i install -v {} ${RUN_PATH}${dest_path}
    done < install_plugins_list.conf
    
    while read src_path dest_path nodetype others
    do 
        echo "${nodetype}:"
        #输出安装前文件
        find  ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
        #输出安装后文件
        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
    done < install_plugins_list.conf  
elif [ $1 == "clean" ] ;then
    cd ${RUN_PATH}
    while read src_path dest_path nodetype others
    do
        echo "find ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i unlink {}"
        find ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i unlink {}
    done < install_plugins_list.conf
else#安装指定node的库文件
    cd ${RUN_PATH}
    while read src_path dest_path nodetype others
    do
        echo "checking ${RUN_PATH}${dest_path}"
        test ! -d ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path} &&　echo "mkdir -p ${RUN_PATH}${dest_path}"
        test ${nodetype} == $1 &&\
        echo "install ${RUN_PATH}${src_path} to ${RUN_PATH}${dest_path}" &&\
        find ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i install -v {} ${RUN_PATH}${dest_path} &&\
        echo "${nodetype}:" &&\
        find  ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i ls -l --color=tty {} &&\
        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {} &&\
        echo "done" && exit 0
    done < install_plugins_list.conf
    echo "nothings to install"
fi
