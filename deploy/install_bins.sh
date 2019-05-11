#!/bin/bash 
#bin文件安装
#RUN_PATH=`dirname $0`
RUN_PATH=`pwd`
cd ${RUN_PATH}
config_file="server_dir.conf"
src_path="/bin/"

function list_server()
{
	echo "USAGE: $0 param1(servername):" 
    while read server others
    do 
    	echo "${server}"
    done < server_list.conf
}

if [ $# -lt 1 ]; then 
    echo "please input param1[all,clean]"
    list_server
	exit 1; 
fi

function list_nodes()
{
    while read nodetype others
    do 
        echo "${nodetype}:"
        #输出安装前文件
        find  ${RUN_PATH}${src_path} -type f -name "${nodetype}" | xargs -i ls -l --color=tty {}
        #输出安装后文件    
        find  ${RUN_PATH}${dest_path} -type f -name "${nodetype}" | xargs -i ls -l --color=tty {}
    done < ${config_file}
}

if [ "$1"x == "all"x ];then
    #安装所有bin文件
    cd ${RUN_PATH}
    while read nodetype others
    do
        test ! -d ${RUN_PATH}/${nodetype}/bin && mkdir -p ${RUN_PATH}/${nodetype}/bin
        echo "install ${RUN_PATH}${src_path}${nodetype} to ${RUN_PATH}/${nodetype}/bin" &&\
        install ${RUN_PATH}${src_path}${nodetype}  ${RUN_PATH}/${nodetype}/bin
    done < ${config_file}
    list_nodes
elif [ "$1"x == "clean"x ] ;then
    #清理全部bin文件
    echo "cleaning all"
    cd ${RUN_PATH}
    while read nodetype others
    do
        echo "unlink ${RUN_PATH}/${nodetype}/bin/${nodetype}"
        unlink ${RUN_PATH}/${nodetype}/bin/${nodetype}
    done < ${config_file}
else #安装指定bin文件
    test ! -f ${RUN_PATH}/bin/$1 && echo "no file for ${RUN_PATH}/bin/$1" && exit 0
    cd ${RUN_PATH}
    while read nodetype others
    do
        test ${nodetype} == $1 &&\
        echo "install ${RUN_PATH}${src_path}$1 to ${RUN_PATH}/${nodetype}/bin" &&\
        install ${RUN_PATH}${src_path}$1     ${RUN_PATH}/${nodetype}/bin &&\
        find  ${RUN_PATH}${src_path} -type f -name "*" | xargs -i ls -l --color=tty {} &&\
        find  ${RUN_PATH}/${nodetype}/bin -type f -name "*" | xargs -i ls -l --color=tty {} &&\
        echo "done" && exit 0
    done < ${config_file}
    echo "nothings to install"
    echo "please input param1[all,clean]"
    list_server
fi
