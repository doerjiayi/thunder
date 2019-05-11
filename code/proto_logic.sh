#!/bin/bash
#编译库文件
MAKE_PATH=`pwd`
cd ${MAKE_PATH}
RUN_PATH=${MAKE_PATH}/../deploy

proto_so=libProto.so

function make_libProto()
{
	if [ -d ${MAKE_PATH}/Proto ];then
		echo "making ${proto_so}"
	    #${proto_so}
	    cd ${MAKE_PATH}/Proto
	    rm ${MAKE_PATH}/Proto/src/*.o ${MAKE_PATH}/Proto/src/*.pb.h ${MAKE_PATH}/Proto/src/*.pb.cc
	    cd ${MAKE_PATH}/Proto/src/
	    make clean
	    ./gen_proto.sh
	    make && cp -v ${MAKE_PATH}/Proto/src/${proto_so} ${RUN_PATH}/lib 
	    echo "${RUN_PATH}/lib"
	    ls -l ${RUN_PATH}/lib 
    else
	    echo "make no proto" 
	fi
}

function clear_libProto()
{
	if [ -d ${MAKE_PATH}/Proto/src ];then
		cd ${MAKE_PATH}/Proto/src
	    make clean 
	    test -f ${RUN_PATH}/lib/${proto_so} && unlink ${RUN_PATH}/lib/${proto_so} 
	else
		echo "clear no proto"
	fi
}

test ! -d ${MAKE_PATH}/3party/lib && ln -s ${RUN_PATH}/3lib ${MAKE_PATH}/3party/lib

if [ "$1"x == "clean"x ];then
    clear_libProto
else
    clear_libProto
    make_libProto
fi


