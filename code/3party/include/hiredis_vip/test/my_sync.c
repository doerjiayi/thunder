#include <stdio.h>

#include "../hircluster.h"

/*
https://github.com/vipshop/hiredis-vip
Cluster connecting
 * */

const char* constPtrAddr =
"192.168.18.68:6000,192.168.18.68:6001,192.168.18.68:6002,192.168.18.68:6003,192.168.18.68:6004,192.168.18.68:6005";

int main()
{
    redisClusterContext *cc = redisClusterConnect(constPtrAddr,HIRCLUSTER_FLAG_NULL);
    if(cc == NULL || cc->err)
	{
		printf("connect error : %s\n", cc == NULL ? "NULL" : cc->errstr);
		return -1;
	}
    print_cluster_node_list(cc);
	int i;
	redisReply* reply = NULL;

	for(i=0; i<10000; i++)
	{
	  //set
	  reply = redisClusterCommand(cc, "set key%d value%d", i, i);
	  if(reply == NULL)
	  {
		printf("set key%d, reply is NULL, error info: %s\n", i, cc->errstr);
		redisClusterFree(cc);
		return -1;
	  }
	  printf("set key%d, reply:%s\n", i, reply->str);
	  freeReplyObject(reply);

	  //get
	  reply = redisClusterCommand(cc, "get key%d", i);
	  if(reply == NULL)
	  {
		printf("get key%d, reply is NULL, error info: %s\n", i, cc->errstr);
		redisClusterFree(cc);
		return -1;
	  }
	  printf("get key%d, reply:%s\n", i, reply->str);
	  freeReplyObject(reply);
	}

	redisClusterFree(cc);
     return 0;
 }

