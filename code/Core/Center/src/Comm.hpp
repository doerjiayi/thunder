/*******************************************************************************
 * Project:  CenterServer
 * @file    Comm.hpp
 * @brief    
 * @author   cjy
 * @date:    2015年9月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_NODE_COMM_HPP_
#define SRC_NODE_COMM_HPP_
#include "ProtoError.h"
#include "util/json/CJsonObject.hpp"
#include "UnixTime.hpp"
#include "dbi/Dbi.hpp"
#include "log4cplus/loggingmacros.h"
#include "NetError.hpp"
#include "NetDefine.hpp"
#include "labor/Labor.hpp"

/*
 1：管理员2：超级管理员
 * */
enum eUserType
{
    eUserType_unknown = 0,
    eUserType_manager = 1,
    eUserType_supermanager = 2,
};

//是否在线已加载配置，0：不是，1：是
enum eReloadConfig
{
    eReloadConfig_no = 0,
    eReloadConfig_yes = 1,
};

//是否自动下发0：不是，1：是
enum eAutoSendConfig
{
    eAutoSendConfig_no = 0,
    eAutoSendConfig_yes = 1,
};

/*
message node_config
{
    //查询和更新发送
    string node_type = 1;//节点类型 ,”LOGIC”
    uint32 config_type = 2;//配置类型，0:服务器配置，其他类型为逻辑配置
    string config_content = 3;//配置内容（目前更新内容的字段名为"so"、"module"、"log_level",可更新其中之一，或者一起更新）
    uint32 auto_send = 4;//是否自动下发0：不是，1：是
    uint32 reload_config = 5;//是否在线已加载配置，0：不是，1：是
    //查询时Server发送
    string config_file  = 6;//配置文件名，如LogicServer.json
    uint32 update_time = 7;//更新时间
}
 * */

DEFINE_INT

namespace core
{

//节点状态信息
typedef struct NodeStatusInfo
{
    int nodeId; //节点ID
    std::string nodeType; //节点类型
    int nodeInnerPort; //节点内网端口
    std::string nodeInnerIp; //节点内网IP
    int nodeAccessPort; //节点外部访问端口
    std::string nodeAccessIp; //节点外部访问IP
    int workerNum; //工作进程数量
    net::uint64 activeTime; //活跃时间
    int load;//服务器负载
    int connect;//服务器连接数
    int client;//客户端连接数
    int recvNum;//接收数据包
    int recvByte;//接收字节
    int sendNum;//发送数据包
    int sendByte;//发送字节
    //负载描述
    //如：[{"load":8,"connect":8,"recv_num":1,"recv_byte":14,"send_num":1,"send_byte":14,"client":5}
    std::string worker;
    short suspend;//是否挂起状态，0：否，1：是, 关闭网关服务器（INTREFACE、ACCESS、OSSI）到指定更新节点路由信息
    NodeStatusInfo()
    {
        clean();
    }
    NodeStatusInfo(const NodeStatusInfo &info)
    {
        nodeId = info.nodeId;
        nodeType = info.nodeType;
        nodeInnerPort = info.nodeInnerPort;
        nodeInnerIp = info.nodeInnerIp;
        nodeAccessPort = info.nodeAccessPort;
        nodeAccessIp = info.nodeAccessIp;
        activeTime = info.activeTime;
        workerNum = info.workerNum;
        load = info.load;
        connect = info.connect;
        client = info.client;
        recvNum = info.recvNum;
        recvByte = info.recvByte;
        sendNum = info.sendNum;
        sendByte = info.sendByte;
        worker = info.worker;
        suspend = info.suspend;
    }
    NodeStatusInfo &operator =(const NodeStatusInfo &info)
    {
        nodeId = info.nodeId;
        nodeType = info.nodeType;
        nodeInnerPort = info.nodeInnerPort;
        nodeInnerIp = info.nodeInnerIp;
        nodeAccessPort = info.nodeAccessPort;
        nodeAccessIp = info.nodeAccessIp;
        activeTime = info.activeTime;
        workerNum = info.workerNum;
        load = info.load;
        connect = info.connect;
        client = info.client;
        recvNum = info.recvNum;
        recvByte = info.recvByte;
        sendNum = info.sendNum;
        sendByte = info.sendByte;
        worker = info.worker;
        suspend = info.suspend;
        return *this;
    }
    void clean()
    {
        nodeId = 0;
        nodeType.clear();
        nodeInnerPort = 0;
        nodeInnerIp.clear();
        nodeAccessPort = 0;
        nodeAccessIp.clear();
        workerNum = 0;
        activeTime = 0; //激活时间
        load = 0;
        connect = 0;
        client = 0;
        recvNum = 0;
        recvByte = 0;
        sendNum = 0;
        sendByte = 0;
        worker.clear();
        suspend = 0;
    }
    //解析json数据
    bool pareJsonData(util::CJsonObject &jObj)
    {
        util::CJsonObject nodeobj, workerobj;
        if (jObj.Get("node_type", nodeType) && jObj.Get("node_ip", nodeInnerIp)
                        && jObj.Get("node_port", nodeInnerPort)
                        && jObj.Get("worker_num", workerNum)
                        && jObj.Get("node_id", nodeId)
                        && jObj.Get("node", nodeobj)
                        && jObj.Get("worker", workerobj))
        {
            //数据作检查
            if (nodeType == "" || nodeInnerPort == 0 || workerNum == 0
                            || nodeInnerIp == "")
            {
                return false;
            }
            activeTime = ::time(NULL); //使用中心服务器的本地时间（发送来的时间精度有问题，且不合适统一管理）
            worker = workerobj.ToString();
            jObj.Get("access_port", nodeAccessPort);
            jObj.Get("access_ip", nodeAccessIp);
            nodeobj.Get("load", load);
            nodeobj.Get("connect", connect);
            nodeobj.Get("client", client);
            nodeobj.Get("recv_num", recvNum);
            nodeobj.Get("recv_byte", recvByte);
            nodeobj.Get("send_num", sendNum);
            nodeobj.Get("send_byte", sendByte);
            return true;
        }
        return false;
    }
    //获取节点的key,为innerIp:innerPort组成
    const std::string getNodeKey() const
    {
        char strNodeKey[100] = { 0 };
        sprintf(strNodeKey, "%s:%d", nodeInnerIp.c_str(), nodeInnerPort);
        return std::string(strNodeKey);
    }
    bool update(const NodeStatusInfo &info) //更新只更新负载部分
    {
        if ((nodeInnerIp == info.nodeInnerIp) && (nodeInnerPort == info.nodeInnerPort))
        {
            activeTime = info.activeTime;
            workerNum = info.workerNum;
            load = info.load;
            connect = info.connect;
            client = info.client;
            recvNum = info.recvNum;
            recvByte = info.recvByte;
            sendNum = info.sendNum;
            sendByte = info.sendByte;
            worker = info.worker;
            return true;
        }
        return false;
    }
} NodeStatusInfo;

//节点日志信息
typedef struct NodeLogInfo
{
    int nodeId; //节点ID
    std::string nodeType; //节点类型
    int nodeInnerPort; //节点内网端口
    std::string nodeInnerIp; //节点内网IP
    int nodeAccessPort; //节点外部访问端口
    std::string nodeAccessIp; //节点外部访问IP
    int workerNum; //工作进程数量
    net::uint64 activeTime; //活跃时间
    //负载描述
    //如：[{"load":8,"connect":8,"recv_num":1,"recv_byte":14,"send_num":1,"send_byte":14,"client":5}
    std::string worker;
    //统计数据
    int addUpLoad;//服务器负载累计
    int counterLoad;//服务器负载计数
    int addUpConnect;//服务器连接数累计
    int counterConnect;//服务器连接数计数
    int addUpClient;//客户端连接数累计
    int counterClient;//客户端连接数计数
    int addUpRecvNum;//接收数据包累计
    int counterRecvNum;//接收数据包计数
    int addUpRecvByte;//接收字节累计
    int counterRecvByte;//接收字节计数
    int addUpSendNum;//发送数据包累计
    int counterSendNum;//发送数据包计数
    int addUpSendByte;//发送字节累计
    int counterSendByte;//发送字节计数
    //最大值
    int maxLoad;//服务器负载最大值
    int maxConnect;//服务器连接数最大值
    int maxClient;//客户端连接数最大值
    int maxRecvNum;//接收数据包最大值
    int maxRecvByte;//接收字节最大值
    int maxSendNum;//发送数据包最大值
    int maxSendByte;//发送字节最大值
    NodeLogInfo()
    {
        clean();
    }
    void clean()
    {
        nodeId = 0;
        nodeType.clear();
        nodeInnerPort = 0;
        nodeInnerIp.clear();
        nodeAccessPort = 0;
        nodeAccessIp.clear();
        workerNum = 0;
        activeTime = 0;
        worker.clear();
        //统计数据
        addUpLoad = 0;
        counterLoad = 0;
        addUpConnect = 0;
        counterConnect = 0;
        addUpClient = 0;
        counterClient = 0;
        addUpRecvNum = 0;
        counterRecvNum = 0;
        addUpRecvByte = 0;
        counterRecvByte = 0;
        addUpSendNum = 0;
        counterSendNum = 0;
        addUpSendByte = 0;
        counterSendByte = 0;
        //最大值
        maxLoad = 0;//服务器负载最大值
        maxConnect = 0;//服务器连接数最大值
        maxClient = 0;//客户端连接数最大值
        maxRecvNum = 0;//接收数据包最大值
        maxRecvByte = 0;//接收字节最大值
        maxSendNum = 0;//发送数据包最大值
        maxSendByte = 0;//发送字节最大值
    }
    void cleanLoad()
    {
        //清除狀態
        activeTime = 0;
        workerNum = 0;
        worker.clear();
        //统计数据
        addUpLoad = 0;
        counterLoad = 0;
        addUpConnect = 0;
        counterConnect = 0;
        addUpClient = 0;
        counterClient = 0;
        addUpRecvNum = 0;
        counterRecvNum = 0;
        addUpRecvByte = 0;
        counterRecvByte = 0;
        addUpSendNum = 0;
        counterSendNum = 0;
        addUpSendByte = 0;
        counterSendByte = 0;
        //最大值
        maxLoad = 0;//服务器负载最大值
        maxConnect = 0;//服务器连接数最大值
        maxClient = 0;//客户端连接数最大值
        maxRecvNum = 0;//接收数据包最大值
        maxRecvByte = 0;//接收字节最大值
        maxSendNum = 0;//发送数据包最大值
        maxSendByte = 0;//发送字节最大值
    }
    //初始化节点日志对象
    NodeLogInfo(const NodeStatusInfo& nodeStatus)
    {
        nodeId = nodeStatus.nodeId; //节点ID
        nodeType = nodeStatus.nodeType; //节点类型
        nodeInnerPort = nodeStatus.nodeInnerPort; //节点内网端口
        nodeInnerIp = nodeStatus.nodeInnerIp; //节点内网IP
        nodeAccessPort = nodeStatus.nodeAccessPort; //节点外部访问端口
        nodeAccessIp = nodeStatus.nodeAccessIp; //节点外部访问IP
        workerNum = nodeStatus.workerNum; //工作进程数量
        activeTime = nodeStatus.activeTime; //活跃时间
        //负载描述
        //如：[{"load":8,"connect":8,"recv_num":1,"recv_byte":14,"send_num":1,"send_byte":14,"client":5}
        worker = nodeStatus.worker;
        //统计数据
        addUpLoad = nodeStatus.load;//服务器负载累计
        counterLoad = 1;//服务器负载计数
        addUpConnect = nodeStatus.connect;//服务器连接数累计
        counterConnect = 1;//服务器连接数计数
        addUpClient = nodeStatus.client;//客户端连接数累计
        counterClient = 1;//客户端连接数计数
        addUpRecvNum = nodeStatus.recvNum;//接收数据包累计
        counterRecvNum = 1;//接收数据包计数
        addUpRecvByte = nodeStatus.recvByte;//接收字节累计
        counterRecvByte = 1;//接收字节计数
        addUpSendNum = nodeStatus.sendNum;//发送数据包累计
        counterSendNum = 1;//发送数据包计数
        addUpSendByte = nodeStatus.sendByte;//发送字节累计
        counterSendByte = 1;//发送字节计数
        //最大值
        maxLoad = nodeStatus.load;//服务器负载最大值
        maxConnect = nodeStatus.connect;//服务器连接数最大值
        maxClient = nodeStatus.client;//客户端连接数最大值
        maxRecvNum = nodeStatus.recvNum;//接收数据包最大值
        maxRecvByte = nodeStatus.recvByte;//接收字节最大值
        maxSendNum = nodeStatus.sendNum;//发送数据包最大值
        maxSendByte = nodeStatus.sendByte;//发送字节最大值
    }
    NodeLogInfo(const NodeLogInfo& info)
    {
        nodeId = info.nodeId;
        nodeType = info.nodeType;
        nodeInnerPort = info.nodeInnerPort;
        nodeInnerIp = info.nodeInnerIp;
        nodeAccessPort = info.nodeAccessPort;
        nodeAccessIp = info.nodeAccessIp;
        workerNum = info.workerNum;
        activeTime = info.activeTime;
        worker = info.worker;
        //统计数据
        addUpLoad = info.addUpLoad;
        counterLoad = info.counterLoad;
        addUpConnect = info.addUpConnect;
        counterConnect = info.counterConnect;
        addUpClient = info.addUpClient;
        counterClient = info.counterClient;
        addUpRecvNum = info.addUpRecvNum;
        counterRecvNum = info.counterRecvNum;
        addUpRecvByte = info.addUpRecvByte;
        counterRecvByte = info.counterRecvByte;
        addUpSendNum = info.addUpSendNum;
        counterSendNum = info.counterSendNum;
        addUpSendByte = info.addUpSendByte;
        counterSendByte = info.counterSendByte;
        //最大值
        maxLoad = info.maxLoad;//服务器负载最大值
        maxConnect = info.maxConnect;//服务器连接数最大值
        maxClient = info.maxClient;//客户端连接数最大值
        maxRecvNum = info.maxRecvNum;//接收数据包最大值
        maxRecvByte = info.maxRecvByte;//接收字节最大值
        maxSendNum = info.maxSendNum;//发送数据包最大值
        maxSendByte = info.maxSendByte;//发送字节最大值
    }
    NodeLogInfo &operator =(const NodeLogInfo &info)
    {
        nodeId = info.nodeId;
        nodeType = info.nodeType;
        nodeInnerPort = info.nodeInnerPort;
        nodeInnerIp = info.nodeInnerIp;
        nodeAccessPort = info.nodeAccessPort;
        nodeAccessIp = info.nodeAccessIp;
        workerNum = info.workerNum;
        activeTime = info.activeTime;
        worker = info.worker;
        //统计数据
        addUpLoad = info.addUpLoad;
        counterLoad = info.counterLoad;
        addUpConnect = info.addUpConnect;
        counterConnect = info.counterConnect;
        addUpClient = info.addUpClient;
        counterClient = info.counterClient;
        addUpRecvNum = info.addUpRecvNum;
        counterRecvNum = info.counterRecvNum;
        addUpRecvByte = info.addUpRecvByte;
        counterRecvByte = info.counterRecvByte;
        addUpSendNum = info.addUpSendNum;
        counterSendNum = info.counterSendNum;
        addUpSendByte = info.addUpSendByte;
        counterSendByte = info.counterSendByte;
        //最大值
        maxLoad = info.maxLoad;//服务器负载最大值
        maxConnect = info.maxConnect;//服务器连接数最大值
        maxClient = info.maxClient;//客户端连接数最大值
        maxRecvNum = info.maxRecvNum;//接收数据包最大值
        maxRecvByte = info.maxRecvByte;//接收字节最大值
        maxSendNum = info.maxSendNum;//发送数据包最大值
        maxSendByte = info.maxSendByte;//发送字节最大值
        return *this;
    }
    //获取节点的key,为innerIp:innerPort组成的字符串
    const std::string getNodeKey() const
    {
        char strNodeKey[100] = { 0 };
        sprintf(strNodeKey, "%s:%d", nodeInnerIp.c_str(), nodeInnerPort);
        return std::string(strNodeKey);
    }
    //日志累计一个节点状态
    bool AddUp(const NodeStatusInfo &nodeStatus)
    {
        if ((nodeInnerIp == nodeStatus.nodeInnerIp) && (nodeInnerPort == nodeStatus.nodeInnerPort))
        {
            //更新状态
            activeTime = nodeStatus.activeTime;
            workerNum = nodeStatus.workerNum;
            worker = nodeStatus.worker;
            //累计数据
            addUpLoad += nodeStatus.load;
            ++counterLoad;
            addUpConnect += nodeStatus.connect;
            ++counterConnect;
            addUpClient += nodeStatus.client;
            ++counterClient;
            addUpRecvNum += nodeStatus.recvNum;
            ++counterRecvNum;
            addUpRecvByte += nodeStatus.recvByte;
            ++counterRecvByte;
            addUpSendNum += nodeStatus.sendNum;
            ++counterSendNum;
            addUpSendByte += nodeStatus.sendByte;
            ++counterSendByte;
            //最大值
            if(maxLoad < nodeStatus.load)
            {
                maxLoad = nodeStatus.load;//服务器负载最大值
            }
            if(maxConnect < nodeStatus.connect)
            {
                maxConnect = nodeStatus.connect;//服务器连接数最大值
            }
            if(maxClient < nodeStatus.client)
            {
                maxClient = nodeStatus.client;//客户端连接数最大值
            }
            if(maxRecvNum < nodeStatus.recvNum)
            {
                maxRecvNum = nodeStatus.recvNum;//接收数据包最大值
            }
            if(maxRecvByte < nodeStatus.recvByte)
            {
                maxRecvByte = nodeStatus.recvByte;//接收字节最大值
            }
            if(maxSendNum < nodeStatus.sendNum)
            {
                maxSendNum = nodeStatus.sendNum;//发送数据包最大值
            }
            if(maxSendByte < nodeStatus.sendByte)
            {
                maxSendByte = nodeStatus.sendByte;//发送字节最大值
            }
            return true;
        }
        return false;
    }
    void Debug()
    {
    	LOG4_DEBUG("nodeId(%d),nodeType(%s),nodeInnerPort(%d),nodeInnerIp(%s),nodeAccessPort(%d),nodeAccessIp(%s),"
                        "workerNum(%d),activeTime(%llu),worker(%s),"
                        "addUpLoad(%d),counterLoad(%d),addUpConnect(%d),counterConnect(%d),"
                        "addUpClient(%d),counterClient(%d),addUpRecvNum(%d),counterRecvNum(%d),"
                        "addUpRecvByte(%d),counterRecvByte(%d),addUpSendNum(%d),counterSendNum(%d),"
                        "addUpSendByte(%d),counterSendByte(%d),"
                        "maxLoad(%d),maxConnect(%d),maxClient(%d),maxRecvNum(%d),maxRecvByte(%d),maxSendNum(%d),maxSendByte(%d)",
                        nodeId, //节点ID
                        nodeType.c_str(), //节点类型
                        nodeInnerPort, //节点内网端口
                        nodeInnerIp.c_str(), //节点内网IP
                        nodeAccessPort, //节点外部访问端口
                        nodeAccessIp.c_str(), //节点外部访问IP

                        workerNum, //工作进程数量
                        activeTime, //活跃时间
                        worker.c_str(),//负载描述,如：[{"load":8,"connect":8,"recv_num":1,"recv_byte":14,"send_num":1,"send_byte":14,"client":5}
                        //统计数据
                        addUpLoad,//服务器负载累计
                        counterLoad,//服务器负载计数
                        addUpConnect,//服务器连接数累计
                        counterConnect,//服务器连接数计数
                        addUpClient,//客户端连接数累计
                        counterClient,//客户端连接数计数
                        addUpRecvNum,//接收数据包累计
                        counterRecvNum,//接收数据包计数
                        addUpRecvByte,//接收字节累计
                        counterRecvByte,//接收字节计数
                        addUpSendNum,//发送数据包累计
                        counterSendNum,//发送数据包计数
                        addUpSendByte,//发送字节累计
                        counterSendByte,//发送字节计数
                        //最大值
                        maxLoad,//服务器负载最大值
                        maxConnect,//服务器连接数最大值
                        maxClient,//客户端连接数最大值
                        maxRecvNum,//接收数据包最大值
                        maxRecvByte,//接收字节最大值
                        maxSendNum,//发送数据包最大值
                        maxSendByte//发送字节最大值
        );
    }
    //统计接口
    int GetAverageLoad()const
    {
        if(addUpLoad && counterLoad)
        {
            return addUpLoad / counterLoad;
        }
        return 0;
    }
    int GetAverageConnect()const
    {
        if(addUpConnect && counterConnect)
        {
            return addUpConnect / counterConnect;
        }
        return 0;
    }
    int GetAverageClient()const
    {
        if(addUpClient && counterClient)
        {
            return addUpClient / counterClient;
        }
        return 0;
    }
    int GetAverageRecvNum()const
    {
        if(addUpRecvNum && counterRecvNum)
        {
            return addUpRecvNum / counterRecvNum;
        }
        return 0;
    }
    int GetAverageRecvByte()const
    {
        if(addUpRecvByte && counterRecvByte)
        {
            return addUpRecvByte / counterRecvByte;
        }
        return 0;
    }
    int GetAverageSendNum()const
    {
        if(addUpSendNum && counterSendNum)
        {
            return addUpSendNum / counterSendNum;
        }
        return 0;
    }
    int GetAverageSendByte()const
    {
        if(addUpSendByte && counterSendByte)
        {
            return addUpSendByte / counterSendByte;
        }
        return 0;
    }
} NodeLogInfo;


//节点信息信息
typedef struct NodesStaisticsInfo
{
    int nodeId; //节点ID
    std::string nodeType; //节点类型
    int nodeInnerPort; //节点内网端口
    std::string nodeInnerIp; //节点内网IP
    int nodeAccessPort; //节点外部访问端口
    std::string nodeAccessIp; //节点外部访问IP
    int workerNum; //工作进和数量
    int load;//服务器负载
    int connect;//服务器连接数
    int client;//客户端连接数
    int recvNum;//接收数据包
    int recvByte;//接收字节
    int sendNum;//发送数据包
    int sendByte;//发送字节
    NodesStaisticsInfo()
    {
        clear();
    }
    void clear()
    {
        nodeId = 0;
        nodeType = "";
        nodeInnerPort = 0;
        nodeInnerIp = "";
        nodeAccessPort = 0;
        nodeAccessIp = "";
        workerNum = 0;
        load = 0;
        connect = 0;
        client = 0;
        recvNum = 0;
        recvByte = 0;
        sendNum = 0;
        sendByte = 0;
    }
    void clearLoad()
    {
        recvNum = 0;
        recvByte = 0;
        sendNum = 0;
        sendByte = 0;
    }
    NodesStaisticsInfo(const NodesStaisticsInfo &info)
    {
        nodeId = info.nodeId;
        nodeType = info.nodeType;
        nodeInnerPort = info.nodeInnerPort;
        nodeInnerIp = info.nodeInnerIp;
        nodeAccessPort = info.nodeAccessPort;
        nodeAccessIp = info.nodeAccessIp;
        workerNum = info.workerNum;
        load = info.load;
        connect = info.connect;
        client = info.client;
        recvNum = info.recvNum;
        recvByte = info.recvByte;
        sendNum = info.sendNum;
        sendByte = info.sendByte;
    }
    NodesStaisticsInfo &operator =(const NodesStaisticsInfo &info)
    {
        nodeId = info.nodeId;
        nodeType = info.nodeType;
        nodeInnerPort = info.nodeInnerPort;
        nodeInnerIp = info.nodeInnerIp;
        nodeAccessPort = info.nodeAccessPort;
        nodeAccessIp = info.nodeAccessIp;
        workerNum = info.workerNum;
        load = info.load;
        connect = info.connect;
        client = info.client;
        recvNum = info.recvNum;
        recvByte = info.recvByte;
        sendNum = info.sendNum;
        sendByte = info.sendByte;
        return *this;
    }
    NodesStaisticsInfo &operator +=(const NodesStaisticsInfo &info)
    {
        if ((nodeType == info.nodeType) && (nodeInnerIp == info.nodeInnerIp)
                        && (nodeInnerPort == info.nodeInnerPort))
        {
            recvNum += info.recvNum;
            recvByte += info.recvByte;
            sendNum += info.sendNum;
            sendByte += info.sendByte;
        }
        return *this;
    }
    NodesStaisticsInfo(const NodeStatusInfo &info)
    {
        nodeId = info.nodeId;
        nodeType = info.nodeType;
        nodeInnerPort = info.nodeInnerPort;
        nodeInnerIp = info.nodeInnerIp;
        nodeAccessPort = info.nodeAccessPort;
        nodeAccessIp = info.nodeAccessIp;
        workerNum = info.workerNum;
        load = info.load;
        connect = info.connect;
        client = info.client;
        recvNum = info.recvNum;
        recvByte = info.recvByte;
        sendNum = info.sendNum;
        sendByte = info.sendByte;
    }
    NodesStaisticsInfo &operator =(const NodeStatusInfo &info)
    {
        nodeId = info.nodeId;
        nodeType = info.nodeType;
        nodeInnerPort = info.nodeInnerPort;
        nodeInnerIp = info.nodeInnerIp;
        nodeAccessPort = info.nodeAccessPort;
        nodeAccessIp = info.nodeAccessIp;
        workerNum = info.workerNum;
        load = info.load;
        connect = info.connect;
        client = info.client;
        recvNum = info.recvNum;
        recvByte = info.recvByte;
        sendNum = info.sendNum;
        sendByte = info.sendByte;
        return *this;
    }
    NodesStaisticsInfo &operator +=(const NodeStatusInfo &info)
    {
        if ((nodeInnerIp == info.nodeInnerIp) && (nodeInnerPort == info.nodeInnerPort))
        {
            //统计负载部分
            ///工作进程数量，服务器负载，服务器连接数，客户端连接数
            //接收数据包，接收字节，发送数据包，发送字节
            if (workerNum != info.workerNum)
            {
                workerNum = info.workerNum;
            }
            if (load != info.load)
            {
                load = info.load;
            }
            if (connect != info.connect)
            {
                connect = info.connect;
            }
            if (client != info.client)
            {
                client = info.client;
            }
            recvNum += info.recvNum;
            recvByte += info.recvByte;
            sendNum += info.sendNum;
            sendByte += info.sendByte;
        }
        return *this;
    }
} NodesStaisticsInfo;

//节点负载状态
struct NodeLoadStatus
{
    NodeLoadStatus()
    {
        clear();
    }
    NodeLoadStatus(const NodeLoadStatus& nodeLoadStatus)
    {
        nodetype = nodeLoadStatus.nodetype;
        serverload = nodeLoadStatus.serverload;
        innerport = nodeLoadStatus.innerport;
        innerip = nodeLoadStatus.innerip;
        outerport = nodeLoadStatus.outerport;
        outerip = nodeLoadStatus.outerip;
    }
    NodeLoadStatus &operator =(const NodeLoadStatus &nodeLoadStatus)
    {
        nodetype = nodeLoadStatus.nodetype;
        serverload = nodeLoadStatus.serverload;
        innerport = nodeLoadStatus.innerport;
        innerip = nodeLoadStatus.innerip;
        outerport = nodeLoadStatus.outerport;
        outerip = nodeLoadStatus.outerip;
        return *this;
    }
    NodeLoadStatus(const NodeStatusInfo &nodeInfo)
    {
        nodetype = nodeInfo.nodeType;
        serverload = nodeInfo.load;
        innerport = nodeInfo.nodeInnerPort;
        innerip = nodeInfo.nodeInnerIp;
        outerport = nodeInfo.nodeAccessPort;
        outerip = nodeInfo.nodeAccessIp;
    }
    NodeLoadStatus &operator =(const NodeStatusInfo &nodeInfo)
    {
        nodetype = nodeInfo.nodeType;
        serverload = nodeInfo.load;
        innerport = nodeInfo.nodeInnerPort;
        innerip = nodeInfo.nodeInnerIp;
        outerport = nodeInfo.nodeAccessPort;
        outerip = nodeInfo.nodeAccessIp;
        return *this;
    }

    std::string nodetype;
    net::uint16 serverload;
    net::uint16 innerport;
    std::string innerip;
    net::uint16 outerport;
    std::string outerip;
    void clear()
    {
        nodetype.clear();
        serverload = 0;
        innerport = 0;
        innerip.clear();
        outerport = 0;
        outerip.clear();
    }
};

//服务器白名单
struct WhiteNode
{
    char inner_ip[32]; //内网IP
    bool loadFromMapRow(util::T_mapRow& valmap)
    {
        if (!valmap["inner_ip"].empty())
        {
            snprintf(inner_ip, sizeof(inner_ip) - 1,valmap["inner_ip"].c_str());
            return true;
        }
        return false;
    }
};


//中心活跃状态
struct CenterActive
{
    char inner_ip[32]; //内网IP
    int inner_port; //内网端口
    uint16 status;//主从状态
    uint64 activetime;//活跃时间
    bool loadFromMapRow(util::T_mapRow& valmap)
    {
        if (valmap["inner_ip"].empty())
        {
            return false;
        }
        snprintf(inner_ip, sizeof(inner_ip) - 1,
                                    valmap["inner_ip"].c_str());
        if (valmap["inner_port"].empty())
        {
            return false;
        }
        inner_port = atoi(valmap["inner_port"].c_str());
        if (valmap["status"].empty())
        {
            return false;
        }
        status = atoi(valmap["status"].c_str());
        if (valmap["activetime"].empty())
        {
            return false;
        }
        activetime = atoi(valmap["activetime"].c_str());
        return true;
    }
};

struct NodeType
{
	typedef std::vector<std::string> ServersList;
	typedef std::vector<std::string>::const_iterator ServersListCIT;
	typedef std::vector<std::string>::iterator ServersListIT;
	std::string nodetype;
	ServersList neededServers;
	void clear()
	{
		nodetype.clear();
		neededServers.clear();
	}
};
typedef std::vector<NodeType> NodeTypesVec;

//节点挂起状态
enum eNodeStatusInfoSuspend
{
    eNodeStatusInfoNormal = 0,//正常状态
    eNodeStatusInfoSuspend = 1,//挂起状态
};

//挂起节点
struct SuspendNode
{
    int suspend; //是否挂起
    bool loadFromMapRow(util::T_mapRow& valmap)
    {
        if (valmap["suspend"].empty())
        {
            return false;
        }
        suspend = atoi(valmap["suspend"].c_str());
        return true;
    }
};

struct CenterServer
{
    std::string center_inner_host;
    int center_inner_port;
    std::string server_identify;
    CenterServer()
    {
        clear();
    }
    CenterServer(const CenterServer &centerServer)
    {
        center_inner_host = centerServer.center_inner_host;
        center_inner_port = centerServer.center_inner_port;
        server_identify = centerServer.server_identify;
    }
    CenterServer &operator =(const CenterServer &centerServer)
    {
        center_inner_host = centerServer.center_inner_host;
        center_inner_port = centerServer.center_inner_port;
        server_identify = centerServer.server_identify;
        return *this;
    }
    void clear()
    {
        center_inner_host.clear();
        center_inner_port = 0;
        server_identify.clear();
    }
};

struct NodeConfigFile
{
    std::string node_type;
    int config_type;
    std::string file_name;
    short check;
    std::vector<std::string> nessesary_fields;
    std::string cmds;
    std::string url_paths;
    std::string description;
    NodeConfigFile()
    {
        clear();
    }
    NodeConfigFile(const NodeConfigFile &nodeConfigFile)
    {
        node_type = nodeConfigFile.node_type;
        config_type = nodeConfigFile.config_type;
        file_name = nodeConfigFile.file_name;
        check = nodeConfigFile.check;
        nessesary_fields = nodeConfigFile.nessesary_fields;
        cmds = nodeConfigFile.cmds;
        url_paths = nodeConfigFile.url_paths;
        description = nodeConfigFile.url_paths;
    }
    NodeConfigFile &operator =(const NodeConfigFile &nodeConfigFile)
    {
        node_type = nodeConfigFile.node_type;
        config_type = nodeConfigFile.config_type;
        file_name = nodeConfigFile.file_name;
        check = nodeConfigFile.check;
        nessesary_fields = nodeConfigFile.nessesary_fields;
        cmds = nodeConfigFile.cmds;
        url_paths = nodeConfigFile.url_paths;
        description = nodeConfigFile.url_paths;
        return *this;
    }
    void clear()
    {
        node_type.clear();
        config_type = 0;
        file_name.clear();
        check = 0;
        nessesary_fields.clear();
        cmds.clear();
        url_paths.clear();
        description.clear();
    }
};

} /* namespace core */

#endif /* SRC_NODEREG_COMM_HPP_ */
