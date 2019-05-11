#!/bin/bash
RUN_PATH=`pwd`
cd ${RUN_PATH}
SERVER_IP=192.168.18.78
SERVER_PORT=6000

./hiredis-test -h ${SERVER_IP} -p ${SERVER_PORT}

#ln -s libhiredis_vip.so libhiredis_vip.so.1.0
#export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/app/tools/redis-cluster/hiredis-vip-master
#gcc -L/app/tools/redis-cluster/hiredis-vip-master -lhiredis_vip my_sync.c -o my_sync
#./my_sync