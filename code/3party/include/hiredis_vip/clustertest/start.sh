#!/bin/bash
RUN_PATH=`pwd`
cd ${RUN_PATH}

export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/app/tools/redis-cluster/hiredis-vip-master:/app/3lib/lib

test -f valgrind.log && rm valgrind.log
valgrind --tool=memcheck --leak-check=full --leak-resolution=high --num-callers=20 --show-reachable=yes --log-file=valgrind.log ./my_async