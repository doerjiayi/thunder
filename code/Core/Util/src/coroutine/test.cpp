#include "coroutine.h"
#include <stdio.h>

namespace util
{

struct args {
	int n;
	struct schedule *S;
};

static void
foo(void *ud) {
	struct args * arg = (args *)ud;
	int start = arg->n;
	int i;
	for (i=0;i<5;i++) {
		printf("coroutine %d : %d\n",coroutine_running(arg->S) , start + i);
		coroutine_yield(arg->S);
	}
}

void
test(struct schedule *S) {
	struct args arg1 = { 0 ,S};
	struct args arg2 = { 100 ,S};

	int co1 = coroutine_new(S, foo, &arg1);
	int co2 = coroutine_new(S, foo, &arg2);
	printf("main start\n");
	while (coroutine_status(S,co1) && coroutine_status(S,co2)) {
		coroutine_resume(S,co1);
		coroutine_resume(S,co2);
	} 
	printf("main end\n");
}
/*
main start
coroutine 0 : 0
coroutine 1 : 100
coroutine 0 : 1
coroutine 1 : 101
coroutine 0 : 2
coroutine 1 : 102
coroutine 0 : 3
coroutine 1 : 103
coroutine 0 : 4
coroutine 1 : 104
main end
 * */

};

//int
//main() {
//	struct util::schedule * S = util::coroutine_open();
//	util::test(S);
//	util::coroutine_close(S);
//	return 0;
//}
