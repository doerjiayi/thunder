#include <stdio.h>
#include <ev.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "../adapters/libev.h"
#include "../hircluster.h"

struct timeval  m_cBeginClock;
struct timeval  m_cEndClock;

void StartClock()
{
	gettimeofday(&m_cBeginClock,NULL);
}
float EndClock()
{
	gettimeofday(&m_cEndClock,NULL);
	float useTime=1000000*(m_cEndClock.tv_sec-m_cBeginClock.tv_sec)+
					m_cEndClock.tv_usec-m_cBeginClock.tv_usec;
	useTime/=1000;
	return useTime;
}

int all_count=0;
int succ_count=0;
int fail_count=0;
const char* constPtrAddr ="192.168.18.68:6000,192.168.18.68:6001,192.168.18.68:6002,192.168.18.68:6003,192.168.18.68:6004,192.168.18.68:6005";
//const char* constPtrAddr ="192.168.18.68:6000";

void getCallback(redisClusterAsyncContext *acc, void *r, void *privdata)
{
	int count =  *(int*)privdata;
	all_count ++;
	redisReply *reply = r;
	if (0 == acc->err)
	{
		++succ_count;
		if (reply)printf("succ reply:%s succ_count:%u fail_count:%u count:%u all_count:%u data:%p %d\n", reply->str,succ_count,fail_count,count,all_count,acc->data,*((int*)acc->data));
	}
	else
	{
		++fail_count;
		printf("failed err:%d %s succ_count:%u fail_count:%u count:%u all_count:%u\n", acc->err,acc->errstr,succ_count,fail_count,count,all_count);
	}
	if(all_count >= count)
	{
		float useTime=EndClock();
		printf("redisClusterAsyncDisconnect useTime:%f ms succ_count:%u fail_count:%u count:%u all_count:%u",
				useTime,succ_count,fail_count,count,all_count);
		//redisClusterAsyncDisconnect useTime:4348.041016 ms succ_count:1000000 fail_count:0 count:1000000 all_count:1000000
		//5 redisClusterAsyncDisconnect useTime:4580.305176 ms succ_count:1000000 fail_count:0 count:1000000 all_count:1000000
		redisClusterAsyncDisconnect(acc);
	}
}

void connectCallback(const redisAsyncContext *c, int status)
{
	if (status != REDIS_OK) {
		printf("connectCallback Error: %s\n", c->errstr);
		return;
	}
	printf("Connected...data:%d host:%s port:%d\n",*((int*)c->userData),c->c.tcp.host,c->c.tcp.port);
}

void disconnectCallback(const redisAsyncContext *c, int status)
{
	if (status != REDIS_OK) {
		printf("disconnectCallback Error: %s\n", c->errstr);
		return;
	}
	printf("disconnected...data:%d host:%s port:%d\n",*((int*)c->userData),c->c.tcp.host,c->c.tcp.port);
}

int count = 1;
redisClusterAsyncContext *g_acc = NULL;
void redis_cluster_async(struct ev_loop *loop)
{
	signal(SIGPIPE, SIG_IGN);
	int status, i;
	int nSucc = 0;
	if (NULL == g_acc)
	{
		g_acc = redisClusterAsyncConnect(constPtrAddr,HIRCLUSTER_FLAG_NULL);
	}
	if (g_acc->err)
	{
		printf("redisClusterAsyncConnect Error: %s\n", g_acc->errstr);
		return;
	}
	g_acc->data = &count;
	redisClusterLibevAttach(g_acc,loop);
	redisClusterAsyncSetConnectCallback(g_acc,connectCallback);
	redisClusterAsyncSetDisconnectCallback(g_acc,disconnectCallback);
	printf("redisClusterAsyncCommand\n");
	for(i = 0; i < count; i ++)
	{
//		status = redisClusterAsyncCommand(g_acc, getCallback, &count, "set %d %d", i, i + count);
		status = redisClusterAsyncCommand(g_acc, getCallback, &count, "get %d", i);
		if(status != REDIS_OK)
		{
			printf("error: %d %s\n", g_acc->err, g_acc->errstr);
		}
		else ++nSucc;
	}
	printf("succ: %d\n", nSucc);
}


int main(int argc, char **argv)
{
	struct ev_loop *loop = EV_DEFAULT;
	StartClock();
	redis_cluster_async(loop);
	ev_run (loop, 0);
	StartClock();
	return 0;
}

