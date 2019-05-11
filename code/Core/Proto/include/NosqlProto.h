/*******************************************************************************
* Project:  proto
* @file     NosqlProto.hpp
* @brief    数据结构定义
* @author   cjy
* @date:    2015年11月11日
* @note
* Modify history:
******************************************************************************/
#ifndef NOSQLPROTO_HPP_
#define NOSQLPROTO_HPP_

enum E_NOSQL_TYPE
{
    NOSQL_T_HASH                = 1,    ///< nosql hash
    NOSQL_T_SET                 = 2,    ///< nosql set
    NOSQL_T_KEYS                = 3,    ///< nosql keys
    NOSQL_T_STRING              = 4,    ///< nosql string
    NOSQL_T_LIST                = 5,    ///< nosql list
    NOSQL_T_SORT_SET            = 6,    ///< nosql sort set
};

enum E_REDIS_DATA
{
    /*属性型数据 */
	DATA_GLOBAL_MSG_BACKUP = 100,
};



#endif /* NOSQLDATA_HPP_ */
