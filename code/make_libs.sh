#!/bin/bash
#编译库文件
MAKE_PATH=`pwd`
cd ${MAKE_PATH}
RUN_PATH=${MAKE_PATH}/../deploy

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input param1[all,llib,proto,thunder,clean]"
    exit 1; 
fi

function make_llib()
{
	if [ -d ${MAKE_PATH}/llib/src ];then
   		echo "making llib.so"
	    #libloss.so
	    cd ${MAKE_PATH}/llib/src
	    make clean
	    make 
	    echo "${RUN_PATH}/lib"
	    ls -l  ${RUN_PATH}/lib
   	else
   		echo "make no lib"     
    fi
    
}

function make_bins_libthunder()
{
	if [ -d ${MAKE_PATH}/thunder/src ];then
		echo "making servers's bin   libthunder.so"
	    echo "cd ${MAKE_PATH}/thunder/src"
	    cd ${MAKE_PATH}/thunder/src
	    make clean 
	    make && find  ${MAKE_PATH}/thunder/src -type f -name "*Server" | xargs -i cp -v {} ${RUN_PATH}/bin && \
	    cp -v ${MAKE_PATH}/thunder/src/libthunder.so ${RUN_PATH}/lib 
	    echo "${RUN_PATH}/bin"
	    ls -l ${RUN_PATH}/bin 
	    echo "${RUN_PATH}/lib"
	    ls -l ${RUN_PATH}/lib 
    else
		echo "make no thunder"    
	fi
}

function make_librobot_proto()
{
	if [ -d ${MAKE_PATH}/proto ];then
		echo "making librobot_proto.so"
	    #librobot_proto.so
	    cd ${MAKE_PATH}/proto
	    rm ${MAKE_PATH}/proto/src/*.o ${MAKE_PATH}/proto/src/*.pb.h ${MAKE_PATH}/proto/src/*.pb.cc
	    cd ${MAKE_PATH}/proto/src/
	    make clean
	    ./gen_proto.sh
	    make && cp -v ${MAKE_PATH}/proto/src/librobot_proto.so ${RUN_PATH}/lib 
	    echo "${RUN_PATH}/lib"
	    ls -l ${RUN_PATH}/lib 
    else
	    echo "make no proto" 
	fi
}

function clear_lib()
{
	if [ -d ${MAKE_PATH}/thunder/src ];then
		cd ${MAKE_PATH}/llib
	    make clean
	    test -f ${RUN_PATH}/lib/libllib.so && unlink  ${RUN_PATH}/lib/libllib.so    
    else
	    echo "clear no lib"
	fi
}

function clear_thunder()
{
	if [ -d ${MAKE_PATH}/thunder/src ];then
		cd ${MAKE_PATH}/thunder/src
	    make clean 
	    test -f ${RUN_PATH}/lib/libthunder.so && unlink  ${RUN_PATH}/lib/libthunder.so
    else
    	echo "clear no thunder"
	fi
}

function clear_librobot_proto()
{
	if [ -d ${MAKE_PATH}/proto/src ];then
		cd ${MAKE_PATH}/proto/src
	    make clean 
	    test -f ${RUN_PATH}/lib/librobot_proto.so && unlink ${RUN_PATH}/lib/librobot_proto.so 
	else
		echo "clear no proto"
	fi
}

if [ $1 == "all" ];then
    make_librobot_proto
    make_llib 
    make_bins_libthunder
elif [ $1 == "llib" ];then
    make_llib
elif [ $1 == "thunder" ];then
    make_bins_libthunder
elif [ $1 == "proto" ];then
    make_librobot_proto
elif [ $1 == "clean" ];then
    clear_lib
    clear_thunder
    clear_librobot_proto
else
    echo "please input param1[all,llib,proto,thunder,clean]"
fi







