#include "coroutine.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#if __APPLE__ && __MACH__
	#include <sys/ucontext.h>
#else 
	#include <ucontext.h>
#endif 
//https://github.com/cloudwu/coroutine

namespace util
{

#define STACK_SIZE (1024*1024)
#define DEFAULT_COROUTINE 16

struct coroutine;
// 协程调度器
struct schedule {
	char stack[STACK_SIZE];
	ucontext_t main;// 正在running的协程在执行完后需切换到的上下文，由于是非对称协程，所以该上下文用来接管协程结束后的程序控制权
	int nco; // 调度器中已保存的协程数量
	int cap; // 调度器中协程的最大容量
	int running;// 调度器中正在running的协程id
	struct coroutine **co;// 连续内存空间，用于存储所有协程任务
};

struct coroutine {
	coroutine_func func; // 协程函数
	void *ud;// 协程函数的参数(用户数据)
	ucontext_t ctx;// 协程上下文
	struct schedule * sch;// 协程所属的调度器
	// ptrdiff_t定义在stddef.h(cstddef)中，通常被定义为long int类型，通常用来保存两个指针减法操作的结果.
	ptrdiff_t cap;// 协程栈的最大容量
	ptrdiff_t size;// 协程栈的当前容量
	int status;// 协程状态(COROUTINE_DEAD/COROUTINE_READY/COROUTINE_RUNNING/COROUTINE_SUSPEND)
	char *stack;// 协程栈
};
// 创建协程任务(分配内存空间)并初始化
struct coroutine * 
_co_new(struct schedule *S , coroutine_func func, void *ud) {
	struct coroutine * co = (struct coroutine *)malloc(sizeof(*co));
	co->func = func;// 初始化协程函数
	co->ud = ud;// 初始化用户数据
	co->sch = S;// 初始化协程所属的调度器
	co->cap = 0;// 初始化协程栈的最大容量
	co->size = 0;// 初始化协程栈的当前容量
	co->status = COROUTINE_READY;// 初始化协程状态
	co->stack = NULL;// 初始化协程栈
	return co;
}
// 销毁协程任务(释放内存空间)
void
_co_delete(struct coroutine *co) {
	free(co->stack);
	free(co);
}

struct schedule * 
coroutine_open(void) {
	struct schedule *S = (schedule *)malloc(sizeof(*S));// 从堆上为调度器分配内存空间
	S->nco = 0;// 初始化调度器的当前协程数量
	S->cap = DEFAULT_COROUTINE;// 初始化调度器的最大协程数量
	S->running = -1;
	S->co = (coroutine **)malloc(sizeof(struct coroutine *) * S->cap);// 为调度器中的协程分配存储空间
	memset(S->co, 0, sizeof(struct coroutine *) * S->cap);
	return S;
}
// 销毁协程调度器schedule
void 
coroutine_close(struct schedule *S) {
	int i;
	for (i=0;i<S->cap;i++) {
		struct coroutine * co = S->co[i];
		if (co) {
			_co_delete(co);
		}
	}
	free(S->co);
	S->co = NULL;
	free(S);
}
// 创建协程任务、并将其加入调度器中
int 
coroutine_new(struct schedule *S, coroutine_func func, void *ud) {
	// 创建协程任务(分配内存空间)并初始化
	struct coroutine *co = _co_new(S, func , ud);
	// 将协程任务co加入调度器S,并返回该协程任务的id
	if (S->nco >= S->cap) {
		// 调整调度器S中协程的最大容量,然后将协程任务co加入调度器S,并返回该协程任务的id
		int id = S->cap;
		S->co = (coroutine **)realloc(S->co, S->cap * 2 * sizeof(struct coroutine *));
		memset(S->co + S->cap , 0 , sizeof(struct coroutine *) * S->cap);
		S->co[S->cap] = co;
		S->cap *= 2;
		++S->nco;
		return id;
	} else {
		// 将协程任务co加入调度器S,并返回该协程任务的id
		int i;
		for (i=0;i<S->cap;i++) {
			int id = (i+S->nco) % S->cap;
			if (S->co[id] == NULL) {
				S->co[id] = co;
				++S->nco;
				return id;
			}
		}
	}
	assert(0);
	return -1;
}
// 所有新协程第一次执行时的入口函数(其中执行协程,并处理善后工作等)
static void
mainfunc(uint32_t low32, uint32_t hi32) {
	uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
	struct schedule *S = (struct schedule *)ptr;
	int id = S->running;
	struct coroutine *C = S->co[id];
	C->func(C->ud);
	_co_delete(C);
	S->co[id] = NULL;
	--S->nco;
	S->running = -1;
}
// 恢复协程号为id的协程任务
void 
coroutine_resume(struct schedule * S, int id) {
	assert(S->running == -1);
	assert(id >=0 && id < S->cap);
	struct coroutine *C = S->co[id];
	if (C == NULL)
		return;
	int status = C->status;
	switch(status) {
	case COROUTINE_READY:
		{
			getcontext(&C->ctx);// 获取程序当前上下文
			C->ctx.uc_stack.ss_sp = S->stack;// 设置上下文C->ctx的栈
			C->ctx.uc_stack.ss_size = STACK_SIZE;// 设置上下文C->ctx的栈容量
			C->ctx.uc_link = &S->main;// 设置上下文C->ctx执行完后恢复到S->main上下文, 否则当前线程因没有上下文可执行而退出
			S->running = id;
			C->status = COROUTINE_RUNNING;
			uintptr_t ptr = (uintptr_t)S;
			// 修改上下文C->ctx, 新的上下文中执行函数mainfunc
			makecontext(&C->ctx, (void (*)(void)) mainfunc, 2, (uint32_t)ptr, (uint32_t)(ptr>>32));
			// 保持当前上下文到S->main, 切换当前上下文为C->ctx
			swapcontext(&S->main, &C->ctx);
		}
		break;
	case COROUTINE_SUSPEND:
		{
			// 拷贝协程栈C->stack到S->stack
			memcpy(S->stack + STACK_SIZE - C->size, C->stack, C->size);
			S->running = id;// 设置当前运行的协程id
			C->status = COROUTINE_RUNNING;// 修改协程C的状态
			swapcontext(&S->main, &C->ctx);// 保存当前上下文到S->main, 切换当前上下文为C->ctx
		}break;
	default:
		{
			assert(0);
		}
		break;
	}
}
// 保存协程栈
static void
_save_stack(struct coroutine *C, char *top) {
	char dummy = 0;
	assert(top - &dummy <= STACK_SIZE);
	if (C->cap < top - &dummy) {
		free(C->stack);
		C->cap = (ptrdiff_t)(top-&dummy);
		C->stack = (char *)malloc(C->cap);
	}
	C->size = top - &dummy;
	memcpy(C->stack, &dummy, C->size);//保存协程栈 to check
}
// 保存上下文后中断当前协程的执行,然后由调度器中的main上下文接管程序执行权
void
coroutine_yield(struct schedule * S) {
	int id = S->running;
	assert(id >= 0);
	struct coroutine * C = S->co[id];
	assert((char *)&C > S->stack);
	_save_stack(C,S->stack + STACK_SIZE);// 保存协程栈
	C->status = COROUTINE_SUSPEND;// 修改协程状态
	S->running = -1;// 修改当前执行的协程id为-1
	swapcontext(&C->ctx , &S->main);// 保存当前协程的上下文到C->ctx, 切换当前上下文到S->main
}
// 根据协程任务id返回协程的当前状态
int 
coroutine_status(struct schedule * S, int id) {
	assert(id>=0 && id < S->cap);
	if (S->co[id] == NULL) {
		return COROUTINE_DEAD;
	}
	return S->co[id]->status;
}
// 返回调度器S中正在running的协程任务id
int 
coroutine_running(struct schedule * S) {
	return S->running;
}


};
