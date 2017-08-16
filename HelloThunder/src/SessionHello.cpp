/*******************************************************************************
 * Project:  HelloThunder
 * @file     SessionHello.cpp
 * @brief 
 * @author   chenjiayi
 * @date:    2017年10月23日
 * @note
 * Modify history:
 ******************************************************************************/
#include "SessionHello.hpp"

namespace hello
{

SessionHello::SessionHello(unsigned int ulSessionId, ev_tstamp dSessionTimeout)
    : thunder::Session(ulSessionId, dSessionTimeout), m_iSayHelloNum(0)
{
}

SessionHello::~SessionHello()
{
}

} /* namespace hello */
