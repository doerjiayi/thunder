#!/bin/bash 
#RUN_PATH=`dirname $0`
RUN_PATH=`pwd`
cd ${RUN_PATH}

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input param1:unlinklib linklib unlink3lib link3lib AllLibs"
    echo "USAGE: $0 param1 param2" 
    echo "please input param1:nodetype[All,clean,Gate,DbAgentRead,DbAgentWrite,Proxy,Log,Robot,Logic,Interface,Lbs,Center,File,ESAgent]"
    echo "please input param2:file[librobot_proto.so,libloss.so,libthunder.so]"
    exit 1;     
fi

if [ $# == 1 ]; then 
	if [ $1 == "unlinklib" ] ;then
	    echo "unlinklib"
	    cd ${RUN_PATH}
	    while read src_path dest_path nodetype others
	    do
	        test -d ${RUN_PATH}${dest_path} && rm -R ${RUN_PATH}${dest_path} &&\
	        echo "rm -R ${RUN_PATH}${dest_path}" 
	    done < install_libs.conf
	    exit 0
	elif [ $1 == "linklib" ] ;then
	    echo "linklib"
	    cd ${RUN_PATH}
	    while read src_path dest_path src_path third_dest_path nodetype others
	    do
	        test ! -d ${RUN_PATH}${dest_path} && ln -s ${RUN_PATH}${src_path} ${RUN_PATH}${dest_path} &&\
	        echo "ln -s ${RUN_PATH}/lib ${RUN_PATH}${dest_path}" 
	    done < install_libs.conf
	    exit 0
	elif [ $1 == "unlink3lib" ] ;then
	    echo "unlink3lib"
	    cd ${RUN_PATH}
	    while read src_path dest_path third_src_path third_dest_path nodetype others
	    do
	        test -d ${RUN_PATH}${third_dest_path} && rm -R ${RUN_PATH}${third_dest_path} &&\
	        echo "rm -R ${RUN_PATH}${third_dest_path}" 
	    done < install_libs.conf
	    exit 0
	elif [ $1 == "link3lib" ] ;then
	    echo "link3lib"
	    cd ${RUN_PATH}
	    while read src_path dest_path third_src_path third_dest_path nodetype others
	    do
	        test ! -d ${RUN_PATH}${third_dest_path} && ln -s ${RUN_PATH}${third_src_path} ${RUN_PATH}${third_dest_path} &&\
	        echo "ln -s ${RUN_PATH}${third_src_path} ${RUN_PATH}${third_dest_path}" 
	    done < install_libs.conf
	    exit 0
	elif [ $1 == "all" ];then 
		#安装librobot_proto.so
	    cd ${RUN_PATH}
	    while read src_path dest_path third_src_path third_dest_path nodetype others
	    do
	        test ! -d ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path}
	        echo "install ${RUN_PATH}${src_path}/librobot_proto.so to ${RUN_PATH}${dest_path}" &&\
	        install ${RUN_PATH}${src_path}/librobot_proto.so  ${RUN_PATH}${dest_path}
	    done < install_libs.conf
	    #安装libloss.so
	    while read src_path dest_path third_src_path third_dest_path nodetype others
	    do
	        test ! -d ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path}
	        echo "install ${RUN_PATH}${src_path}/libloss.so to ${RUN_PATH}${dest_path}" &&\
	        install ${RUN_PATH}${src_path}/libloss.so  ${RUN_PATH}${dest_path}
	    done < install_libs.conf
	    #安装libthunder.so
	    while read src_path dest_path third_src_path third_dest_path nodetype others
	    do
	        test ! -d ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path}
	        echo "install ${RUN_PATH}${src_path}/libthunder.so to ${RUN_PATH}${dest_path}" &&\
	        install ${RUN_PATH}${src_path}/libthunder.so  ${RUN_PATH}${dest_path}
	    done < install_libs.conf
	    #输出so
	    while read src_path dest_path third_src_path third_dest_path nodetype others
	    do 
	        echo "${nodetype}:"
	        #输出安装前文件
	        find  ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
	        #输出安装后文件
	        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
	    done < install_libs.conf 
	    echo "install all libs done"
	    exit 0;     
	fi
elif [ $# == 2 ]; then 
    if [ $1 == "clean" ] ;then
	    cd ${RUN_PATH}
	    while read src_path dest_path third_src_path third_dest_path nodetype others
	    do
	        echo "unlink ${RUN_PATH}${dest_path}/$2"
	        unlink ${RUN_PATH}${dest_path}/$2
	    done < install_libs.conf
	    exit 0
	elif [ $1 == "lib" ];then #安装指定库到所有节点 如 ./install_libs.sh CenterServer librobot_proto.so
		test ! -f ${RUN_PATH}/lib/$2 && echo "no file for ${RUN_PATH}/lib/$2" && exit 0
	    cd ${RUN_PATH}
	    while read src_path dest_path third_src_path third_dest_path nodetype others
	    do
	        test ! -d ${RUN_PATH}${dest_path} && mkdir -p ${RUN_PATH}${dest_path}
	        echo "install ${RUN_PATH}${src_path}/$2 to ${RUN_PATH}${dest_path}" &&\
	        install ${RUN_PATH}${src_path}/$2     ${RUN_PATH}${dest_path}
	    done < install_libs.conf
	    
	    while read src_path dest_path third_src_path third_dest_path nodetype others
	    do 
	        echo "${nodetype}:"
	        #输出安装前文件
	        find  ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
	        #输出安装后文件
	        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
	    done < install_libs.conf 
	    exit 0
	else #安装指定库到指定节点   如 ./install_libs.sh CenterServer librobot_proto.so
		test ! -f ${RUN_PATH}/lib/$2 && echo "no file for ${RUN_PATH}/lib/$2" && exit 0
	    cd ${RUN_PATH}
	    while read src_path dest_path third_src_path third_dest_path nodetype others
	    do
	        test ${nodetype} == $1 &&\
	        echo "install ${RUN_PATH}${src_path}/$2 to ${RUN_PATH}${dest_path}" &&\
	        install ${RUN_PATH}${src_path}/$2     ${RUN_PATH}${dest_path} &&\
	        echo "${nodetype}:" &&\
	        find  ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i ls -l --color=tty {} &&\
	        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {} &&\
	        echo "done" && exit 0
	    done < install_libs.conf
	fi
fi

echo "USAGE: $0 param1 param2" 
echo "please input param1:nodetype[All,clean,Gate,DbAgentRead,DbAgentWrite,Proxy,Log,Robot,Logic,Interface,Lbs,Center]"
echo "please input param2:file[librobot_proto.so,libloss.so,libthunder.so]"


