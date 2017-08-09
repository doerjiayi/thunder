/*******************************************************************************
* Project:  AsyncServer
* @file     main.cpp
* @brief 
* @author   cjy
* @date:    2015年8月6日
* @note
* Modify history:
******************************************************************************/

#include "labor/process/NodeManager.hpp"
#include "unix_utility/proctitle_helper.h"

int main(int argc, char* argv[])
{
    signal(SIGPIPE, SIG_IGN);
    if (argc != 2)
    {
        std::cerr << "para num error!" << std::endl;
        exit(-1);
    }
	ngx_init_setproctitle(argc, argv);
    thunder::NodeManager oManager(argv[1]);
    oManager.Run();
	return(0);
}
