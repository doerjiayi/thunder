#!/bin/bash 
#bin文件安装
#RUN_PATH=`dirname $0`
RUN_PATH=`pwd`
cd ${RUN_PATH}

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input param1:nodetype[All,Clean,DbAgentRead,DbAgentWrite,DataProxyServer,LogServer,RobotServer,Center,CenterServer,LogicServer]"
	exit 1; 
fi

if [ $1 == "all" ];then
    #安装所有bin文件
    cd ${RUN_PATH}
    while read src_path dest_path server others
    do
        test ! -d ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path}
        echo "install ${RUN_PATH}${src_path}/${server} to ${RUN_PATH}${dest_path}" &&\
        install ${RUN_PATH}${src_path}/${server}  ${RUN_PATH}${dest_path}
    done < install_bins.conf
    
    while read src_path dest_path server others
    do 
        echo "${server}:"
        #输出安装前文件
        find  ${RUN_PATH}${src_path} -type f -name "${server}" | xargs -i ls -l --color=tty {}
        #输出安装后文件    
        find  ${RUN_PATH}${dest_path} -type f -name "${server}" | xargs -i ls -l --color=tty {}
    done < install_bins.conf 
elif [ $1 == "clean" ] ;then
    #清理全部bin文件
    echo "cleaning all"
    cd ${RUN_PATH}
    while read src_path dest_path server others
    do
        echo "unlink ${RUN_PATH}${dest_path}/${server}"
        unlink ${RUN_PATH}${dest_path}/${server}
    done < install_bins.conf
else #安装指定bin文件
    test ! -f ${RUN_PATH}/bin/$1 && echo "no file for ${RUN_PATH}/bin/$1" && exit 0
    cd ${RUN_PATH}
    while read src_path dest_path server others
    do
        test ${server} == $1 &&\
        echo "install ${RUN_PATH}${src_path}/$1 to ${RUN_PATH}${dest_path}" &&\
        install ${RUN_PATH}${src_path}/$1     ${RUN_PATH}${dest_path} &&\
        find  ${RUN_PATH}${src_path} -type f -name "*" | xargs -i ls -l --color=tty {} &&\
        find  ${RUN_PATH}${dest_path} -type f -name "*" | xargs -i ls -l --color=tty {} &&\
        echo "done" && exit 0
    done < install_bins.conf
    echo "nothings to install"
    echo "USAGE: $0 param1" 
    echo "please input param1:nodetype[All,Clean,DbAgentRead,DbAgentWrite,DataProxyServer,LogServer,RobotServer,Center,CenterServer,LogicServer]"
fi
