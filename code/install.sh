#!/bin/bash

THUNDER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${THUNDER_HOME}
THUNDER_HOME=`pwd`
THUNDER_DEPOLY=${THUNDER_HOME}/../deploy

cd ${THUNDER_DEPOLY}
ln -s  ${THUNDER_HOME}/l3lib/lib  ./3lib

cd ${THUNDER_HOME}
cd ./llib/src && make install

cd ${THUNDER_HOME}
cd ./thunder/src && make install

cd ${THUNDER_HOME}
cd ./HelloThunder/src && make install