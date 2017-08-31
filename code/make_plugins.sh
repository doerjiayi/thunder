#!/bin/bash 
#编译目录
#MAKE_PATH=`dirname $0`
MAKE_PATH=`pwd`
cd ${MAKE_PATH}
RUN_PATH=${MAKE_PATH}/../deploy

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input param1:nodetype[all,clean,Gate,DbAgentRead,DbAgentWrite,Proxy,Log,Logic,Robot,Center,Crm,File]"
	exit 1; 
fi

while read src_path dest_path nodetype others
do
    test ! -d  ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path}
done < make_plugins_list.conf

if [ $1 == "all" ]
then
    cd ${MAKE_PATH}
    while read src_path dest_path nodetype others
    do
        test ! -d ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path} &&　echo "mkdir -p ${RUN_PATH}${dest_path}"
        echo "cd ${MAKE_PATH}${src_path}" &&\
        cd ${MAKE_PATH}${src_path} &&\
        make clean && make && find ${MAKE_PATH}${src_path} -type f -name "*.so" | xargs -i cp -v {} ${RUN_PATH}${dest_path}
        cd ${MAKE_PATH}
    done < make_plugins_list.conf
    #输出编译后文件
    while read src_path dest_path nodetype others
    do 
        echo "${nodetype}:"
        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
    done < make_plugins_list.conf 
elif [ $1 == "clean" ] 
then
    cd ${MAKE_PATH}
    while read src_path dest_path nodetype others
    do
        echo "find ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i unlink {}"
        find ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i unlink {}
    done < make_plugins_list.conf
else
    cd ${MAKE_PATH}
    while read src_path dest_path nodetype others
    do
        test ${nodetype} == $1 &&\
        echo "cd ${MAKE_PATH}${src_path}" &&\
        cd ${MAKE_PATH}${src_path} &&\
        make clean && make && find ${MAKE_PATH}${src_path} -type f -name "*.so" | xargs -i cp -v {} ${RUN_PATH}${dest_path} &&\
        echo "${nodetype}:" &&\
        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {} &&\
        echo "done" && exit 0
    done < make_plugins_list.conf
    echo "nothings to make"
    echo "USAGE: $0 param1" 
    echo "please input param1:nodetype[All,Clean,Gate,DbAgentRead,DbAgentWrite,Proxy,Log,Logic,Robot,Center,Crm,File]"
fi


