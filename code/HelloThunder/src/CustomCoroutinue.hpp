#ifndef SRC_CustomCoroutinue_HPP_
#define SRC_CustomCoroutinue_HPP_
#include <string>
#include "labor/NodeLabor.hpp"

namespace hello
{

class SessionHello;

////协程名需要是静态变量CoroutineName
////协程函数需要是静态变量CoroutineFunc
////协程固定工作者labor
#define CoroutineArgsFixedMembers \
static std::string CoroutineName;\
static void CoroutineFunc(void *ud);\
thunder::NodeLabor* labor;

//具体用户参数
struct CoroutineArgs {
	CoroutineArgsFixedMembers
	//自定义参数
	int n;
	SessionHello* pSession;
};
struct CoroutineArgs2 {
	CoroutineArgsFixedMembers
	//自定义参数
	std::string param;
	SessionHello* pSession;
};

#define  MakeCoroutine(ClassArgs,arg)  GetLabor()->CoroutineNew(ClassArgs::CoroutineName,ClassArgs::CoroutineFunc,&arg)

#define  RunCoroutine(ClassArgs)\
while (GetLabor()->CoroutineTaskSize(ClassArgs::CoroutineName) > 0)\
{\
	GetLabor()->CoroutineResume(ClassArgs::CoroutineName);\
}

#define SuspendCoroutine(arg) arg->labor->CoroutineYield(CoroutineName)

};

#endif
