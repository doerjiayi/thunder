#!/bin/bash 
#RUN_PATH=`dirname $0`
RUN_PATH=`pwd`
cd ${RUN_PATH}

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input param1:all"
    echo "USAGE: $0 param1 param2" 
    echo "please input param1:nodetype[all,clean,HelloThunder,...]"
    echo "please input param2:file[librobot_proto.so,libloss.so,libthunder.so]"
    exit 1;     
fi

function print_so()
{
    while read src_path dest_path nodetype others
    do 
        echo "${nodetype}:"
        echo "print so src"
        find  ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
        echo "print so dest"
        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {}
    done < install_libs.conf 
}

function r_libs_conf_p1()
{
	cd ${RUN_PATH}
    while read src_path dest_path nodetype others
    do
    	if [ "$1"x == "all"x ];then 
			echo "install librobot_proto.so"
			install -vD ${RUN_PATH}${src_path}/librobot_proto.so  ${RUN_PATH}${dest_path}
		    echo "install libllib.so"
		    install -vD ${RUN_PATH}${src_path}/libllib.so  ${RUN_PATH}${dest_path}
		    echo "install libthunder.so"
		    install -vD ${RUN_PATH}${src_path}/libthunder.so  ${RUN_PATH}${dest_path}
	    elif [ "$1"x == "clean"x ];then
	    	echo "unlink librobot_proto.so"
	    	unlink ${RUN_PATH}${dest_path}/librobot_proto.so
	    	echo "unlink libllib.so"
	    	unlink ${RUN_PATH}${dest_path}/libllib.so
	    	echo "unlink libthunder.so"
	    	unlink ${RUN_PATH}${dest_path}/libthunder.so
		fi
    done < install_libs.conf
	print_so
}

function r_libs_conf_p2()
{
	cd ${RUN_PATH}
    while read src_path dest_path nodetype others
    do
    	if [ "$1"x == "clean"x ] ;then
    		unlink ${RUN_PATH}${dest_path}/$2 
    	elif [ "$1"x == "all"x ];then #安装指定库到所有节点 如 ./install_libs.sh all libthunder.so
	        install -vD ${RUN_PATH}${src_path}/$2 ${RUN_PATH}${dest_path}
	    else #安装指定库到所有节点 如 ./install_libs.sh HelloThunder libthunder.so
	    	test "${nodetype}"x == "$1"x &&\
	        install -vD ${RUN_PATH}${src_path}/$2     ${RUN_PATH}${dest_path} &&\
	        echo "${nodetype}:" &&\
	        find  ${RUN_PATH}${src_path} -type f -name "*.so" | xargs -i ls -l --color=tty {} &&\
	        find  ${RUN_PATH}${dest_path} -type f -name "*.so" | xargs -i ls -l --color=tty {} &&\
	        echo "done" && exit 0
    	fi
    done < install_libs.conf
	print_so
}

if [ $# == 1 ]; then 
	r_libs_conf_p1 $1
elif [ $# == 2 ]; then 
    if [ "$1"x == "clean"x ] ;then #清理指定文件
 		r_libs_conf_p2 $1 $2
	elif [ "$1"x == "lib"x ];then  #安装指定库到所有节点   如 ./install_libs.sh lib libthunder.so
		test ! -f ${RUN_PATH}/lib/$2 && echo "no file for ${RUN_PATH}/lib/$2" && exit 0
	    r_libs_conf_p2 $1 $2
	else #安装指定库到指定节点   如 ./install_libs.sh HelloThunder libthunder.so
		test ! -f ${RUN_PATH}/lib/$2 && echo "no file for ${RUN_PATH}/lib/$2" && exit 0
	    r_libs_conf_p2 $1 $2
	fi
fi




