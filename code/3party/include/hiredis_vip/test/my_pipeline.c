#include<stdio.h>

#include "../hircluster.h"

const char* constPtrAddr =
"192.168.18.68:6000,192.168.18.68:6001,192.168.18.68:6002,192.168.18.68:6003,192.168.18.68:6004,192.168.18.68:6005";

int main()
{
	redisClusterContext *cc;
	redisReply *reply;

	cc = redisClusterContextInit();
	redisClusterSetOptionAddNodes(cc, constPtrAddr);
	redisClusterConnect2(cc);
	if(cc == NULL || cc->err)
	{
		printf("connect error : %s\n", cc == NULL ? "NULL" : cc->errstr);
		return -1;
	}

	redisClusterAppendCommand(cc, "set key1 value1");
	redisClusterAppendCommand(cc, "get key1");
	redisClusterAppendCommand(cc, "mset key2 value2 key3 value3");
	redisClusterAppendCommand(cc, "mget key2 key3");

	redisClusterGetReply(cc, &reply);  //for "set key1 value1"
	if(reply)printf("reply:%s\n",reply->str);
	freeReplyObject(reply);
	redisClusterGetReply(cc, &reply);  //for "get key1"
	if(reply)printf("reply:%s\n",reply->str);
	freeReplyObject(reply);
	redisClusterGetReply(cc, &reply);  //for "mset key2 value2 key3 value3"
	if(reply)printf("reply:%s\n",reply->str);
	freeReplyObject(reply);
	redisClusterGetReply(cc, &reply);  //for "mget key2 key3"
	if(reply)printf("reply:%s\n",reply->str);
	freeReplyObject(reply);

//	redisCLusterReset(cc);
	redisClusterFree(cc);
	return 0;
}
