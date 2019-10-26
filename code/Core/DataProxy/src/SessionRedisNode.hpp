/*******************************************************************************
 * Project:  DataProxyServer
 * @file     SessionRedisNode.hpp
 * @brief    Redis节点Session
 * @author   cjy
 * @date:    2016年3月19日
 * @note     存储Redis节点信息，提供Redis节点的添加、删除、修改操作，提供通过
 * hash字符串或hash值定位具体节点操作。
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDDATAPROXY_SESSIONREDISNODE_HPP_
#define SRC_CMDDATAPROXY_SESSIONREDISNODE_HPP_
#include <vector>
#include <map>
#include "session/Session.hpp"
#include "ProtoError.h"

#define ROT32(x, y) ((x << y) | (x >> (32 - y))) // avoid effort

namespace core
{

const unsigned long FNV_64_INIT = 0x100000001b3;
const unsigned long FNV_64_PRIME = 0xcbf29ce484222325;

enum E_HASH_ALGORITHM
{
    HASH_fnv1a_64           = 0,
    HASH_fnv1_64            = 1,
    HASH_murmur3_32         = 2,
};

struct tagRedisNodeAttr
{
    tagRedisNodeAttr():uiPingMasterNode(0),uiPingSlaveNode(0){}
    std::string strMasterNodeIdentify;
    std::string strSlaveNodeIdentify;
    std::vector<uint32> vecHash;
    uint32 uiPingMasterNode;
    uint32 uiPingSlaveNode;
};
/**
 * @brief Redis节点Session
 * @note Redis节点Session常驻内存，永不过期，构造函数中的dSessionTimeout传入0表示
 * 不做超时检查。Timeout()方法实现直接返回Running，即便dSessionTimeout设置了超时，
 * 此Session也只是成为了一个定时器，不会真正超时。
 */
class SessionRedisNode: public net::Session
{
public:
    /**
     * @note Redis节点管理Session构造函数
     * @param strSessionId 会话ID，在这里用作Redis节点类型
     * @param iVirtualNodeNum 每个实体节点对应的虚拟节点数量
     * @param dSessionTimeout 超时时间，0表示永不超时
     */
    SessionRedisNode(const std::string& strSessionId, int iHashAlgorithm = HASH_fnv1a_64,int iVirtualNodeNum = 200,const std::string &strGroupUse="ssdb_group",ev_tstamp dSessionTimeout = 3.0);
    virtual ~SessionRedisNode();
    virtual net::E_CMD_STATUS Timeout();
public:
    /**
     * @brief 获取Redis节点信息
     * @note 通过hash key获取一致性hash算法计算后对应的主备redis节点信息
     * @param strHashKey 数据操作的key值
     * @param strMasterNodeIdentify 主Redis节点信息
     * @param strSlaveNodeIdentify  备Redis节点信息
     * @return 是否成功获取
     */
    bool GetRedisNode(const std::string& strHashKey, std::string& strMasterNodeIdentify, std::string& strSlaveNodeIdentify);
    /**
     * @brief 添加Redis节点
     * @note 添加Redis节点信息，每个节点均有一个主节点一个被节点构成。
     * @param strNodeIdentify Redis节点标识
     * @param strMasterHost 主节点IP
     * @param iMasterPort   主节点端口
     * @param strSlaveHost  备节点IP
     * @param iSlavePort    备节点端口
     */
    void AddRedisNode(const std::string& strNodeIdentify,const std::string& strMasterHostPort,const std::string& strSlaveHostPort);
    /**
     * @brief 删除Redis节点
     * @note 删除Redis节点信息，每个节点均有一个主节点一个被节点构成。
     * @param strNodeIdentify Redis节点标识
     */
    void DelRedisNode(const std::string& strNodeIdentify);

protected:
    uint32 hash_fnv1_64(const char *key, size_t key_length);
    uint32 hash_fnv1a_64(const char *key, size_t key_length);
    uint32_t murmur3_32(const char *key, uint32_t len, uint32_t seed);

private:
    const int m_iHashAlgorithm;
    const int m_iVirtualNodeNum;

    const std::string m_strGroupUse;
    /* redis实体节点信息
     * key为形如PropertyRedis001的字符串
     * value为hash(PropertyRedis001#0) hash(PropertyRedis001#1) hash(PropertyRedis001#2) 组成的vector */
    typedef std::map<std::string, tagRedisNodeAttr* > RedisNodeMap;
    RedisNodeMap m_mapRedisNode;

    struct RedisVirtualNode
    {
        RedisVirtualNode(const std::string &MasterNodeIdentify,const std::string &SlaveNodeIdentify,
                        const std::string &VirtualNodeIdentify,const std::string &NodeIdentify):
            strMasterNodeIdentify(MasterNodeIdentify),strSlaveNodeIdentify(SlaveNodeIdentify),
            strVirtualNodeIdentify(VirtualNodeIdentify),strNodeIdentify(NodeIdentify){}
        std::string strMasterNodeIdentify;
        std::string strSlaveNodeIdentify;
        std::string strVirtualNodeIdentify;
        std::string strNodeIdentify;
    };
    // redis虚拟节点信息hash， key为m_mapRedisNode中vector的各个取值，value为一对（主备）形如192.168.16.22:16379的字符串
    typedef std::map<uint32, RedisVirtualNode > RedisVirtualNodeMap;
    RedisVirtualNodeMap m_mapRedisNodeHash;
};

} /* namespace core */

#endif /* SRC_CMDDATAPROXY_SESSIONREDISNODE_HPP_ */
