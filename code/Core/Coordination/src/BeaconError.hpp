/*******************************************************************************
* Project:  NebulaBeacon
* @file     BeaconError.hpp
* @brief 
* @author   Bwar
* @date:    2018年12月8日
* @note
* Modify history:
******************************************************************************/
#ifndef BEACONERROR_HPP_
#define BEACONERROR_HPP_

namespace coor
{

/**
 * @brief 错误码定义
 */
enum E_ERROR_NO
{
    ERR_OK                              = 0,        ///< 正确

    /* 集群管理错误码段  61000~61999 */
    ERR_SERVICE                         = 61000,    ///< 服务内部错误
    ERR_INVALID_CMD                     = 61001,    ///< 命令错误
    ERR_INVALID_ARGV                    = 61002,    ///< 参数错误
    ERR_INVALID_ARGC                    = 61003,    ///< 参数个数错误
    ERR_NODE_IDENTIFY                   = 61004,    ///< 节点标识错误
    ERR_NODE_TYPE                       = 61005,    ///< 节点类型错误
};

}

#endif /* BEACONERROR_HPP_ */
