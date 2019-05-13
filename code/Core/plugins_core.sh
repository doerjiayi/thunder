#!/bin/bash 
#编译目录
#MAKE_PATH=`dirname $0`
MAKE_PATH=`pwd`
cd ${MAKE_PATH}
RUN_PATH=${MAKE_PATH}/../../deploy

command="all,clean,Access,DbAgentRead,DbAgentWrite,Center"

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input param1:nodetype[${command}]"
	exit 1; 
fi

while read nodetype src_path dest_path  others
do
    test ! -d  ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path}
done < plugins_core.conf

function print_so()
{
	#输出编译后文件
    while read nodetype src_path dest_path others
    do 
        echo "${nodetype}:"
        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
    done < plugins_core.conf 
}

if [ $1 == "all" ];then
    cd ${MAKE_PATH}
    while read nodetype src_path dest_path others
    do
        test ! -d ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path} &&　echo "mkdir -p ${RUN_PATH}${dest_path}"
        echo "cd ${MAKE_PATH}${src_path}" &&\
        cd ${MAKE_PATH}${src_path} &&\
        make clean && make && find ${MAKE_PATH}${src_path} -type f -name "*.so" | xargs -i cp -v {} ${RUN_PATH}${dest_path}
        cd ${MAKE_PATH}
    done < plugins_core.conf
    print_so
elif [ $1 == "clean" ] ;then
    cd ${MAKE_PATH}
    while read nodetype src_path dest_path others
    do
        echo "find ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i unlink {}"
        find ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i unlink {}
    done < plugins_core.conf
else
    cd ${MAKE_PATH}
    while read nodetype src_path dest_path others
    do
        test ${nodetype} == $1 &&\
        echo "cd ${MAKE_PATH}${src_path}" &&\
        cd ${MAKE_PATH}${src_path} &&\
        make clean && make && find ${MAKE_PATH}${src_path} -type f -name "*.so" | xargs -i cp -v {} ${RUN_PATH}${dest_path} &&\
        echo "${nodetype}:" &&\
        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {} &&\
        echo "done" && exit 0
    done < plugins_core.conf
    echo "nothings to make"
    echo "USAGE: $0 param1" 
    echo "please input param1:nodetype[${command}]"
fi
