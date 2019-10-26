/*
 * DataProxySession.h
 *
 *  Created on: 2018年1月8日
 *      Author: chen
 */
#ifndef CODE_SRC_DATAPROXYSSESSION_H_
#define CODE_SRC_DATAPROXYSSESSION_H_
#include <string>
#include <map>
#include <vector>
#include "util/json/CJsonObject.hpp"
#include "session/Session.hpp"
#include "SessionRedisNode.hpp"
#include "NetDefine.hpp"
#include "NetError.hpp"
#include "ProtoError.h"
#include "RedisStorageStep.hpp"
#include "step/Step.hpp"
#include "cmd/Cmd.hpp"
#include "SessionSyncDbData.hpp"

#define DATAPROXY_SESSION_ID (20000)

const int gc_iErrBuffSize = 256;

namespace core
{

class DataProxySession: public net::Session
{
public:
    DataProxySession(double session_timeout = 1.0):net::Session(DATAPROXY_SESSION_ID, session_timeout,"net::DataProxySession"),boInit(false)
    ,m_ScanSyncDataTime(0),m_ScanSyncDataLastTime(0)
    {
    }
    ~DataProxySession(){};
    net::E_CMD_STATUS Timeout();
    bool Init();
    bool ReadNosqlClusterConf();
    bool ReadNosqlDbRelativeConf();

    //数据发送检查
    bool GetSectionFactor(int32 data_purpose,uint64 uiFactor,std::string& strSectionFactor);
    bool GetSectionFactor(int32 iDataType, int32 iSectionFactorType, uint64 uiFactor,std::string& strSessionFactor);
    bool GetNosqlTableName(int32 data_purpose,std::string& strTableName);
    bool PreprocessRedis(DataMem::MemOperate& oMemOperate);
    bool CheckRedisOperate(const DataMem::MemOperate::RedisOperate& oRedisOperate);//检查RedisOperate合法性
    bool CheckDbOperate(const DataMem::MemOperate::DbOperate& oDbOperate);//检查DbOperate合法性
    bool CheckDataSet(const DataMem::MemOperate& oMemOperate,const std::string& strRedisDataPurpose);
    bool CheckJoinField(const DataMem::MemOperate& oMemOperate, const std::string& strRedisDataPurpose);
    //数据填写
    bool PrepareForWriteBothWithDataset(DataMem::MemOperate& oMemOperate, const std::string& strRedisDataPurpose);
    bool PrepareForWriteBothWithFieldJoin(DataMem::MemOperate& oMemOperate, const std::string& strRedisDataPurpose);

    //任务
    bool ScanSyncData();
    bool boInit;
    util::CJsonObject m_oNosqlDbRelative;
	std::map<std::string, std::set<uint32> > m_mapFactorSection; //分段因子区间配置，key为因子类型
	std::map<std::string, std::set<std::string> > m_mapTableFields; //表的组成字段，key为表名，value为字段名集合，用于查找请求的字段名是否存在

	char m_pErrBuff[256];
	uint32 m_ScanSyncDataTime;
	uint32 m_ScanSyncDataLastTime;
};

DataProxySession* GetDataProxySession();

}
;

#endif /* CODE_WEBSERVER_SRC_WEBSESSION_H_ */
