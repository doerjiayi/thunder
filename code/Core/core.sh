#!/bin/bash
#编译库文件
MAKE_PATH=`pwd`
cd ${MAKE_PATH}
RUN_PATH=${MAKE_PATH}/../../deploy

lib_so=libUtil.so
core_so=libNet.so

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input param1[all,Util,Net,clean]"
    exit 1; 
fi

function make_Util()
{
	if [ -d ${MAKE_PATH}/Util ];then
		cd ${MAKE_PATH}/Proto/src && chmod +x ./gen_proto.sh && ./gen_proto.sh && cd ${MAKE_PATH}
		test ! -d ${RUN_PATH}/lib && mkdir ${RUN_PATH}/lib
   		echo "making ${lib_so}"
	    #libUtil.so
	    cd ${MAKE_PATH}/Util
	    make clean
	    make 
	    echo "${RUN_PATH}/lib"
	    ls -l  ${RUN_PATH}/lib
   	else
   		echo "make no lib"     
    fi
}

function make_plugins()
{
	if [ -f ${MAKE_PATH}/plugins_core.sh ];then
   		echo "making plugins"
   		cd ${MAKE_PATH}
	    ${MAKE_PATH}/plugins_core.sh all
   	else
   		echo "make no plugins"     
    fi
    
}

function make_bins_libNet()
{
	if [ -d ${MAKE_PATH}/Net/src ];then
		echo "making servers's bin   ${core_so}"
	    echo "cd ${MAKE_PATH}/Net/src"
	    cd ${MAKE_PATH}/Net/src
	    make clean 
	    make # && find  ${MAKE_PATH}/Net/src -type f -name "*Server" | xargs -i cp -v {} ${RUN_PATH}/bin && \
	    cp -v ${MAKE_PATH}/Net/src/${core_so} ${RUN_PATH}/lib 
	    echo "${RUN_PATH}/bin"
	    ls -l ${RUN_PATH}/bin 
	    echo "${RUN_PATH}/lib"
	    ls -l ${RUN_PATH}/lib 
    else
		echo "make no Net"    
	fi
}

function clear_Util()
{
	if [ -d ${MAKE_PATH}/Net/src ];then
		cd ${MAKE_PATH}/Util
	    make clean
	    test -f ${RUN_PATH}/lib/libUtil.so && unlink  ${RUN_PATH}/lib/libUtil.so    
    else
	    echo "clear no lib"
	fi
}

function clear_Net()
{
	if [ -d ${MAKE_PATH}/Net/src ];then
		cd ${MAKE_PATH}/Net/src
	    make clean 
	    test -f ${RUN_PATH}/lib/${core_so} && unlink  ${RUN_PATH}/lib/${core_so}
    else
    	echo "clear no Net"
	fi
}

test ! -d ${MAKE_PATH}/../3party/lib && ln -s ${RUN_PATH}/3lib ${MAKE_PATH}/../3party/lib 

cd ${MAKE_PATH}

if [ $1 == "all" ];then
    make_Util 
    make_bins_libNet
    make_plugins
elif [ $1 == "Util" ];then
    make_Util
elif [ $1 == "Plugin" ];then
    make_plugins
elif [ $1 == "Net" ];then
    make_bins_libNet
elif [ $1 == "Clean" ];then
    clear_Util
    clear_Net
else
    echo "please input param1[all,Util,Net,Plugin,Clean]"
fi


