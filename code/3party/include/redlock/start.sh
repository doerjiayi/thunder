#!/bin/bash

SERVER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${SERVER_HOME}
SERVER_HOME=`pwd`
 
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/app/thunder/code/3party/include/redlock/hiredis
./bin/CLockExample
