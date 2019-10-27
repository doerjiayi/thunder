#!/bin/bash

#需要系统centos 

DEPLOY_PATH="."
cd $DEPLOY_PATH
DEPLOY_PATH=`pwd`
BUILD_PATH="${DEPLOY_PATH}/build"
3lib_PATH="${DEPLOY_PATH}/3lib"
INCLUDE_PATH="${DEPLOY_PATH}/../code/3party/include"

CPU_NUM=`lscpu | awk '/^CPU\(s\)/{print $2}'`
chmod u+x *.sh

mkdir -p ${BUILD_PATH} >> /dev/null 2>&1

DEPLOY_LOCAL=false 

#
#log4cplus-2.0.2
cd ${BUILD_PATH}
# install protobuf
if ! $DEPLOY_LOCAL
then
    if [ -f jemalloc-5.2.1.tar.bz2 ]
    then
        echo "jemalloc-5.2.1.tar.bz2 exist, skip download."
    else
        wget -O https://github.com/jemalloc/jemalloc/releases/download/5.2.1/jemalloc-5.2.1.tar.bz2
        if [ $? -ne 0 ]
        then
            echo "failed to download log4cplus!" >&2
            exit 2
        fi
    fi

    tar xvf  jemalloc-5.2.1.tar.bz2
    cd jemalloc-5.2.1
    ./configure --prefix=${BUILD_PATH}/deps
    make -j$CPU_NUM
    make install
    if [ $? -ne 0 ]
    then
        echo "failed, teminated!" >&2
        exit 2
    fi
fi


#log4cplus-2.0.2
cd ${BUILD_PATH}
# install protobuf
if ! $DEPLOY_LOCAL
then
    if [ -f log4cplus-2.0.4.tar.gz ]
    then
        echo "log4cplus-2.0.4.tar.gz exist, skip download."
    else
        wget -O https://github.com/log4cplus/log4cplus/releases/download/REL_2_0_4/log4cplus-2.0.4.tar.gz
        if [ $? -ne 0 ]
        then
            echo "failed to download log4cplus!" >&2
            exit 2
        fi
    fi

    tar xvf log4cplus-2.0.4.tar.gz
    cd log4cplus-2.0.4
    ./configure --prefix=${BUILD_PATH}/deps
    make -j$CPU_NUM
    make install
    if [ $? -ne 0 ]
    then
        echo "failed, teminated!" >&2
        exit 2
    fi
fi
 
cd ${BUILD_PATH}
# install protobuf
if ! $DEPLOY_LOCAL
then
    if [ -f protobuf-3.10.0.zip ]
    then
        echo "protobuf-3.10.0 exist, skip download."
    else
        wget -O protobuf-3.10.0.zip https://github.com/google/protobuf/archive/v3.10.0.zip
        if [ $? -ne 0 ]
        then
            echo "failed to download protobuf!" >&2
            exit 2
        fi
    fi

    unzip protobuf-3.10.0.zip
    cd protobuf-3.10.0
    chmod u+x autogen.sh
    ./autogen.sh
    ./configure --prefix=${BUILD_PATH}/deps
    make -j$CPU_NUM
    make install
    if [ $? -ne 0 ]
    then
        echo "failed, teminated!" >&2
        exit 2
    fi
fi

# install libev
cd ${BUILD_PATH}
if ! $DEPLOY_LOCAL
then
    if [ -f libev.zip ]
    then
        echo "libev exist, skip download."
    else
        wget -O libev.zip https://github.com/kindy/libev/archive/master.zip
        if [ $? -ne 0 ]
        then
            echo "failed to download libev!" >&2
            exit 2
        fi
        mv master.zip libev.zip
    fi

    unzip libev.zip
    mv libev-master libev
    cd libev/src
    chmod u+x autogen.sh
    ./autogen.sh
    ./configure --prefix=${BUILD_PATH}/deps
    make -j$CPU_NUM
    make install
    if [ $? -ne 0 ]
    then
        echo "failed, teminated!" >&2
        exit 2
    fi
fi

# install hiredis_vip
cd ${BUILD_PATH}
if ! $DEPLOY_LOCAL
then
    if [ -f hiredis_vip.zip ]
    then
        echo "directory hiredis exist, skip download."
    else
        wget -O hiredis_vip.zip https://codeload.github.com/vipshop/hiredis-vip/zip/master
        if [ $? -ne 0 ]
        then
            echo "failed to download hiredis!" >&2
            exit 2
        fi
    fi

    unzip hiredis_vip.zip
    cd hiredis
    make -j$CPU_NUM
    
    mkdir -p ${INCLUDE_PATH}/hiredis_vip
    cp *.h ${INCLUDE_PATH}/hiredis_vip
    
    cp libhiredis_vip.so.1.0 ${3lib_PATH}/libhiredis_vip.so.1.0
    cd ${3lib_PATH}
    ln -s libhiredis_vip.so.1.0 libhiredis_vip.so
    cd -
    if [ $? -ne 0 ]
    then
        echo "failed, teminated!" >&2
        exit 2
    fi
fi

# install crypto++
cd ${BUILD_PATH}
if ! $DEPLOY_LOCAL
then
    if [ -f CRYPTOPP_8_0_0.zip ]
    then
        echo "cryptopp-CRYPTOPP_8_0_0 exist, skip download."
    else
        wget https://github.com/weidai11/cryptopp/archive/CRYPTOPP_8_0_0.zip
        if [ $? -ne 0 ]
        then
            echo "failed to download crypto++!" >&2
            exit 2
        fi
    fi

    unzip CRYPTOPP_8_0_0.zip
    cd cryptopp-CRYPTOPP_8_0_0
    make -j$CPU_NUM libcryptopp.so
    
    mkdir -p ${INCLUDE_PATH}/cryptopp
    cp *.h ${INCLUDE_PATH}/cryptopp
    
    cp libcryptopp.so ${3lib_PATH}/libcryptopp.so.8
    cd ${3lib_PATH}
    
    ln -s libcryptopp.so.8 libcryptopp.so
    cd -
    if [ $? -ne 0 ]
    then
        echo "failed, teminated!" >&2
        exit 2
    fi
fi


 


