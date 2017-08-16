ASYNC_SERVER_PATH=`dirname $0`
cd ${ASYNC_SERVER_PATH}
ASYNC_SERVER_PATH=`pwd`
ASYNC_SERVER_PATH_LIB=/app/robot/robotServer/code/l3oss/lib
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${ASYNC_SERVER_PATH_LIB}
/app/robot/robotServer/tools/bin/protoc  --version 
/app/robot/robotServer/tools/bin/protoc -I=. --cpp_out=.  ./http.proto  ./msg.proto ./oss_sys.proto 
