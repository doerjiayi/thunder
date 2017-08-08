/*******************************************************************************
 * Project:  DataProxyServer
 * @file     StorageOperator.hpp
 * @brief    存储协议操作者
 * @author   cjy
 * @date:    2015年11月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef CLIENT_STORAGEOPERATOR_HPP_
#define CLIENT_STORAGEOPERATOR_HPP_

#include <stdio.h>
#include <string>
#include "dataproxy.pb.h"

namespace oss
{

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int int64;
typedef unsigned long long int uint64;

class StorageOperator
{
public:
    StorageOperator();
    virtual ~StorageOperator();

    virtual DataMem::MemOperate* MakeMemOperate() = 0;
};

} /* namespace oss */

#endif /* CLIENT_STORAGEOPERATOR_HPP_ */
