/*******************************************************************************
 * Project:  DataProxyServer
 * @file     StorageOperator.hpp
 * @brief    存储协议操作者
 * @author   cjy
 * @date:    2019年11月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef CLIENT_STORAGEOPERATOR_HPP_
#define CLIENT_STORAGEOPERATOR_HPP_
#include <stdio.h>
#include <string>
#include "dataproxy.pb.h"
#include "NetDefine.hpp"

namespace net
{

class StorageOperator
{
public:
    StorageOperator();
    virtual ~StorageOperator();

    virtual DataMem::MemOperate* MakeMemOperate() = 0;
};

} /* namespace net */

#endif /* CLIENT_STORAGEOPERATOR_HPP_ */
