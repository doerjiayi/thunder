ASYNC_SERVER_PATH=`dirname $0`
cd ${ASYNC_SERVER_PATH}
ASYNC_SERVER_PATH=`pwd`
ASYNC_SERVER_PATH_LIB=/app/thunder/deploy/3lib
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${ASYNC_SERVER_PATH_LIB}
/app/thunder/deploy/3lib/protoc  --version 
/app/thunder/deploy/3lib/protoc -I=.. --cpp_out=. ../common.proto  ../enum.proto  ../user.proto ../user_basic.proto  \
       ../robot_session.proto  ../test_proto3.proto ../robot_knowledge.proto  
