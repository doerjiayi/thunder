/*******************************************************************************
 * Project:  DataProxy
 * @file     SessionSyncDbData.hpp
 * @brief    数据同步
 * @author   cjy
 * @date:    2016年7月18日
 * @note     因连接问题或数据库故障写redis成功但写数据库失败的请求都会被写到文件，
 * 待故障恢复后再从文件中读出并重新写入数据库。
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CMDDATAPROXY_SESSIONSYNCDBDATA_HPP_
#define SRC_CMDDATAPROXY_SESSIONSYNCDBDATA_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <map>
#include "util/UnixTime.hpp"
#include "util/CBuffer.hpp"
#include "codec/ProtoCodec.hpp"
#include "session/Session.hpp"
#include "storage/dataproxy.pb.h"
#include "StepSyncToDb.hpp"
#include "ProtoError.h"

namespace core
{

class StepSyncToDb;

#define SYNC_DATA_DIR "/data/"

class SessionSyncDbData: public net::Session
{
public:
    SessionSyncDbData(const std::string& strTableName, const std::string& strDataPath,
    		ev_tstamp dSessionTimeout = 10.0);
    virtual ~SessionSyncDbData();

    virtual net::E_CMD_STATUS Timeout();

    /**
     * @brief 检查写入本地文件
     */
    void CheckWritingFile();
    /**
     * @brief 同步数据到本地文件
     * @param oMemOperate 请求存储操作数据
     * @return 是否成功同步
     */
    bool SyncData(const DataMem::MemOperate& oMemOperate);

    /**
     * @brief 获取同步文件中的数据
     * @param[out] oMsgHead 数据包头
     * @param[out] oMsgBody 数据包体
     * @return 是否获取到数据
     */
    bool GetSyncData(MsgHead& oMsgHead, MsgBody& oMsgBody);

    /**
     * @brief 数据发送失败或处理失败回退读指针
     * @param oMsgHead  发送失败的数据包头
     * @param oMsgBody  发送失败的数据包体
     */
    void GoBack(const MsgHead& oMsgHead, const MsgBody& oMsgBody);

    /**
     * @brief 扫描目录下需同步的文件
     * @return 是否扫描成功
     */
    bool ScanSyncData();
    /**
     * @brief 获取读计数器
     */
    uint32 GetReadCounter()const {return m_ReadCounter;}
    /**
     * @brief 获取写计数器
     */
    uint32 GetWritedCounter()const {return m_WritedCounter;}
private:
    std::string m_strTableName;
    std::string m_strDataPath;
    util::CBuffer* m_pBuffR;
    util::CBuffer* m_pBuffWaitR;
    util::CBuffer* m_pBuffW;
    net::ProtoCodec* m_pCodec;
    int m_iSendingSize;
    int m_iReadingFd;
    int m_iWritingFd;
    std::string m_strReadingFile;
    std::string m_strWritingFile;
    time_t m_WritingTime;
    uint32 m_WritedCounter;
    uint32 m_ReadCounter;
    std::set<std::string> m_setDataFiles;// set (strFileName)
};

SessionSyncDbData* GetSessionSyncDbData(const std::string& strTableName, const std::string& strDataPath, ev_tstamp dSessionTimeout=10.0);

} /* namespace core */

#endif /* SRC_CMDDATAPROXY_SESSIONSYNCDBDATA_HPP_ */
