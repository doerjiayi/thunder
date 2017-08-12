SCRIPT_NAME=`basename $0`
THUNDER_HOME=..

THUNDER_3LIB=${THUNDER_HOME}/3lib/lib
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${THUNDER_3LIB}

if [ -f testserver ];then
	#testserver $ip $port $uid_from $uid_to
	./testserver 192.168.18.68 10000 20000 30000 
else
	echo "testserver not exist"
fi