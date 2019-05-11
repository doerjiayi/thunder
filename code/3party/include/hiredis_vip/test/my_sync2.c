#include <stdio.h>

#include "../hircluster.h"

const char* constPtrAddr =
"192.168.18.68:6000,192.168.18.68:6001,192.168.18.68:6002,192.168.18.68:6003,192.168.18.68:6004,192.168.18.68:6005";

int main()
{
    char *key="key-a";
    char *field="field-1";
    char *key1="key1";
    char *value1="value-1";
    char *key2="key1";
    char *value2="value-1";
    redisClusterContext *cc;

    cc = redisClusterContextInit();
    redisClusterSetOptionAddNodes(cc, constPtrAddr);
    redisClusterConnect2(cc);
    if(cc == NULL || cc->err)
    {
        printf("connect error : %s\n", cc == NULL ? "NULL" : cc->errstr);
        return -1;
    }

    redisReply *reply = redisClusterCommand(cc, "hmget %s %s", key, field);
    if(reply == NULL)
    {
        printf("reply is null[%s]\n", cc->errstr);
        redisClusterFree(cc);
        return -1;
    }

    printf("reply->type:%d\n", reply->type);

    freeReplyObject(reply);

    reply = redisClusterCommand(cc, "mset %s %s %s %s", key1, value1, key2, value2);
    if(reply == NULL)
    {
        printf("reply is null[%s]\n", cc->errstr);
        redisClusterFree(cc);
        return -1;
    }

    printf("succ reply->str:%s\n", reply->str);

    freeReplyObject(reply);
    redisClusterFree(cc);
    return 0;
}
