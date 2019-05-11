#!/bin/bash 
#RUN_PATH=`dirname $0`
RUN_PATH=`pwd`
cd ${RUN_PATH}
src_path="/lib"
dest_path="/lib"
config_file="server_dir.conf"

proto_so=libanalysis_proto.so
lib_so=libloss.so
core_so=libstarship.so

function list_server()
{
	echo "USAGE: $0 param1(servername):" 
    while read server others
    do 
    	echo "${server}"
    done < server_list.conf
}

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input param1:all"
    echo "USAGE: $0 param1 param2" 
    echo "please input param1:nodetype[all,clean]"
    list_server
    echo "please input param2:file[${proto_so},${lib_so},${core_so}]"
    exit 1;     
fi

function print_so()
{
    while read nodetype others
    do 
        echo "${nodetype}:"
        echo "print so src"
        find  ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
        echo "print so dest"
        find  ${RUN_PATH}/${nodetype}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
    done < ${config_file}
}

function r_libs_conf_p1()
{
	cd ${RUN_PATH}
    while read nodetype others
    do
    	if [ "$1"x == "all"x ];then 
			echo "install librobot_proto.so"
			test -d ${RUN_PATH}/${nodetype}${dest_path} && install -vD ${RUN_PATH}${src_path}/${proto_so}  ${RUN_PATH}/${nodetype}${dest_path}
		    echo "install libllib.so"
		    test -d ${RUN_PATH}/${nodetype}${dest_path} && install -vD ${RUN_PATH}${src_path}/${lib_so}  ${RUN_PATH}/${nodetype}${dest_path}
		    echo "install libthunder.so"
		    test -d ${RUN_PATH}/${nodetype}${dest_path} && install -vD ${RUN_PATH}${src_path}/${core_so}  ${RUN_PATH}/${nodetype}${dest_path}
	    elif [ "$1"x == "clean"x ];then
	    	echo "unlink librobot_proto.so"
	    	unlink ${RUN_PATH}/${nodetype}${dest_path}/${proto_so}
	    	echo "unlink libllib.so"
	    	unlink ${RUN_PATH}/${nodetype}${dest_path}/${lib_so}
	    	echo "unlink libthunder.so"
	    	unlink ${RUN_PATH}/${nodetype}${dest_path}/${core_so}
		fi
    done < ${config_file}
	print_so
}

function r_libs_conf_p2()
{
	cd ${RUN_PATH}
    while read nodetype others
    do
    	if [ "$1"x == "clean"x ] ;then
    		unlink ${RUN_PATH}${dest_path}/$2 
    	elif [ "$1"x == "all"x ];then #安装指定库到所有节点 如 ./install_libs.sh all ${core_so}
	        install -vD ${RUN_PATH}${src_path}/$2 ${RUN_PATH}${dest_path}
	    else #安装指定库到所有节点 如 ./install_libs.sh node ${core_so}
	    	test "${nodetype}"x == "$1"x &&\
	        install -vD ${RUN_PATH}${src_path}/$2     ${RUN_PATH}/${nodetype}${dest_path} &&\
	        echo "${nodetype}:" &&\
	        find  ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i ls -l --color=tty {} &&\
	        find  ${RUN_PATH}/${nodetype}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {} &&\
	        echo "done" && exit 0
    	fi
    done < ${config_file}
	print_so
}

if [ $# == 1 ]; then 
	r_libs_conf_p1 $1
elif [ $# == 2 ]; then 
    if [ "$1"x == "clean"x ] ;then #清理指定文件
 		r_libs_conf_p2 $1 $2
	elif [ "$1"x == "lib"x ];then  #安装指定库到所有节点   如 ./install_libs.sh lib ${core_so}
		test ! -f ${RUN_PATH}/lib/$2 && echo "no file for ${RUN_PATH}/lib/$2" && exit 0
	    r_libs_conf_p2 $1 $2
	else #安装指定库到指定节点   如 ./install_libs.sh node ${core_so}
		test ! -f ${RUN_PATH}/lib/$2 && echo "no file for ${RUN_PATH}/lib/$2" && exit 0
	    r_libs_conf_p2 $1 $2
	fi
fi




