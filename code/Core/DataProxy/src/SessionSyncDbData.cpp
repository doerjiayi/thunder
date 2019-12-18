/*******************************************************************************
 * Project:  DataProxy
 * @file     SessionSyncDbData.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年7月18日
 * @note
 * Modify history:
 ******************************************************************************/
#include "util/FileUtil.h"
#include "SessionSyncDbData.hpp"

namespace core
{
static uint64 g_uiReadSyncCounter = 0;
static uint64 g_uiWriteSyncCounter = 0;

SessionSyncDbData::SessionSyncDbData(const std::string& strTableName, const std::string& strDataPath, ev_tstamp dSessionTimeout)
    : net::Session(strTableName, dSessionTimeout, "net::SessionSyncDbData"),
      m_strTableName(strTableName), m_strDataPath(strDataPath),
	  m_pBuffR(NULL),m_pBuffWaitR(NULL),m_pBuffW(NULL),m_pCodec(NULL), m_iSendingSize(0), m_iReadingFd(-1), m_iWritingFd(-1),m_WritingTime(0),
      m_WritedCounter(0),m_ReadCounter(0)
{
	m_pBuffR = new util::CBuffer();
	m_pBuffW = new util::CBuffer();
	m_pBuffWaitR = new util::CBuffer();
}

SessionSyncDbData::~SessionSyncDbData()
{
    if (m_pBuffR != NULL)
    {
        delete m_pBuffR;
        m_pBuffR = NULL;
    }
    if (m_pBuffWaitR != NULL)
	{
		delete m_pBuffWaitR;
		m_pBuffWaitR = NULL;
	}
    if (m_pBuffW != NULL)
	{
		delete m_pBuffW;
		m_pBuffW = NULL;
	}
    if (m_pCodec != NULL)
    {
        delete m_pCodec;
        m_pCodec = NULL;
    }
    if (-1 != m_iReadingFd)
    {
        close(m_iReadingFd);
    }
    if (-1 != m_iWritingFd)
    {
        close(m_iWritingFd);
    }
    m_setDataFiles.clear();
}

net::E_CMD_STATUS SessionSyncDbData::Timeout()
{
    LOG4_DEBUG("setDataFiles size() = %u", m_setDataFiles.size());
    if (0 == m_setDataFiles.size())
    {
        return(net::STATUS_CMD_COMPLETED);
    }
    CheckWritingFile();
    std::vector<std::string> strIdentifys;
    net::GetNodeIdentifys(AGENT_W,strIdentifys);
    if (strIdentifys.size() > 0)
    {//获取消息
    	LOG4_DEBUG("DBAGENT_W nodes routes size(%u)",strIdentifys.size());
    	MsgHead oMsgHead;
		MsgBody oMsgBody;
        if(GetSyncData(oMsgHead, oMsgBody))
        {
            if (!GetLabor()->ExecStep(new StepSyncToDb(m_strTableName, oMsgHead, oMsgBody)))
            {
            	LOG4_ERROR("failed to ExecStep(new StepSyncToDb)");
            }
        }
        if(m_setDataFiles.empty())
        {
            LOG4_DEBUG("no local file for strTableName(%s) now",m_strTableName.c_str());
        }
    }
    else
    {
    	LOG4_DEBUG("don't has DBAGENT_W nodes routes,waiting");
    }
    LOG4_DEBUG("continue to check for strTableName(%s)",m_strTableName.c_str());
    return(net::STATUS_CMD_RUNNING);
}

void SessionSyncDbData::CheckWritingFile()
{
    if (m_WritingTime + 60 < net::GetNowTime())//最后写文件时间超过1分钟则关闭最后写文件
    {
        if (-1 != m_iWritingFd)
        {
            close(m_iWritingFd);
            m_iWritingFd = -1;
        }
        m_strWritingFile.clear();
    }
}

bool SessionSyncDbData::SyncData(const DataMem::MemOperate& oMemOperate)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (NULL == m_pCodec)
    {
        m_pCodec = new net::ProtoCodec(util::CODEC_PROTOBUF);
    }
    char szWriteFileName[64] = {0};//0.test.201709271758.dat
    snprintf(szWriteFileName, sizeof(szWriteFileName), "%u.%s.%s.dat", net::GetWorkerIndex(),
				oMemOperate.db_operate().table_name().c_str(),util::time_t2TimeStr(net::GetNowTime(), "YYYYMMDDHHMI").c_str());
    if (m_strWritingFile != std::string(szWriteFileName))
    {
        if (m_iWritingFd > 0)
        {
            close(m_iWritingFd);
            m_iWritingFd = -1;
            m_strWritingFile.clear();
        }

        if (!util::IsDirectory(m_strDataPath.c_str()))
        {
            std::string strDir = std::string(".") + SYNC_DATA_DIR;
            if (!util::DeepCreateDirectory(strDir.c_str()))
            {
                LOG4_ERROR("%s().failed to create dir %s error:%d,%s",__FUNCTION__,m_strDataPath.c_str(), errno,strerror(errno));
                return(false);
            }
            LOG4_INFO("%s().succ to create data dir(%s)",__FUNCTION__,strDir.c_str());
        }
        m_iWritingFd = open(std::string(m_strDataPath + szWriteFileName).c_str(), O_CREAT|O_APPEND|O_WRONLY, S_IRUSR|S_IWUSR);
        if (m_iWritingFd < 0)
        {
            LOG4_ERROR("open file %s error:%d,%s", std::string(m_strDataPath + szWriteFileName).c_str(), errno,strerror(errno));
            return(false);
        }
        m_strWritingFile = szWriteFileName;
    }
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    oMsgBody.set_body(oMemOperate.SerializeAsString());
    oMsgHead.set_cmd(net::CMD_REQ_STORATE);
    oMsgHead.set_seq(0);
    oMsgHead.set_msgbody_len(oMsgBody.ByteSize());
    m_pBuffW->Clear();
    m_pCodec->Encode(oMsgHead,oMsgBody,m_pBuffW);
    int iNeedWriteLen = 0;
    int iWriteLen = 0;
    iNeedWriteLen = m_pBuffW->ReadableBytes();
    LOG4_DEBUG("%s(),szWriteFileName(%s),oMsgHead.ByteSize(%d)",
                    __FUNCTION__,std::string(m_strDataPath + szWriteFileName).c_str(),oMsgHead.ByteSize());
    int ret(0);
    while (iWriteLen < iNeedWriteLen)
    {
        ret = write(m_iWritingFd, m_pBuffW->GetRawBuffer() + iWriteLen, iNeedWriteLen - iWriteLen);
        if(-1 == ret)
        {
            LOG4_ERROR("failed to write to file(%s)",std::string(m_strDataPath + szWriteFileName).c_str());
            return false;
        }
        iWriteLen += ret;
    }
    m_setDataFiles.insert(m_strWritingFile);//读文件时会检查
    m_WritingTime = net::GetNowTime();//最后写文件时间
    ++m_WritedCounter;
    ++g_uiWriteSyncCounter;
    LOG4_DEBUG("%s(),WritedCounter(%u),g_uiWriteSyncCounter(%llu),iNeedWriteLen(%d),write file(%s)",
            __FUNCTION__,m_WritedCounter,g_uiWriteSyncCounter,iNeedWriteLen,szWriteFileName);
    return(true);
}

bool SessionSyncDbData::GetSyncData(MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (NULL == m_pCodec)
    {
        m_pCodec = new net::ProtoCodec(util::CODEC_PROTOBUF);
    }
    if (m_iReadingFd < 0)
    {
        if (0 == m_setDataFiles.size())
        {
            LOG4_DEBUG("no local file for strTableName(%s)",m_strTableName.c_str());
            return(false);
        }
        m_strReadingFile = *(m_setDataFiles.begin());
        if (m_strReadingFile == m_strWritingFile)//读写分离
        {
            LOG4_DEBUG("strReadingFile(%s) and strWritingFile(%s) are the same,wait to read",
                            m_strReadingFile.c_str(),m_strWritingFile.c_str());
            return(false);
        }
        m_iReadingFd = open(std::string(m_strDataPath + m_strReadingFile).c_str(), O_RDONLY);
        LOG4_TRACE("%s()open local File(%s)", __FUNCTION__,m_strReadingFile.c_str());
        if (m_iReadingFd < 0)
        {
            LOG4_ERROR("open file %s error %d", m_strReadingFile.c_str(), errno);
            m_setDataFiles.erase(m_strReadingFile);
            return(false);
        }
    }
    LOG4_TRACE("continue to read from file(%s),iReadingFd(%d)",m_strReadingFile.c_str(),m_iReadingFd);
    int iErrno = 0;
    int iReadLen = 0;
    do
    {
        if(m_pBuffWaitR->ReadableBytes() > 0)
        {//继续解析上一次回退数据
            net::E_CODEC_STATUS eCodecStatus = m_pCodec->Decode(m_pBuffWaitR, oMsgHead, oMsgBody);
            if (net::CODEC_STATUS_OK == eCodecStatus)
            {
                ++m_ReadCounter;
                ++g_uiReadSyncCounter;
                LOG4_DEBUG("succ to Decode for strTableName(%s),local file(%s),ReadCounter(%u)",
                                m_strTableName.c_str(),m_strReadingFile.c_str(),m_ReadCounter);
                m_iSendingSize = oMsgHead.ByteSize() + oMsgHead.msgbody_len();
                return(true);
            }
            else if (net::CODEC_STATUS_ERR == eCodecStatus)
            {
                LOG4_ERROR("failed to Decode,data file(%s)",m_strReadingFile.c_str());
                break;
            }
        }
        if(m_pBuffR->ReadableBytes() > 0)
		{//继续解析上一次已读数据
			net::E_CODEC_STATUS eCodecStatus = m_pCodec->Decode(m_pBuffR, oMsgHead, oMsgBody);
			if (net::CODEC_STATUS_OK == eCodecStatus)
			{
				++m_ReadCounter;
				++g_uiReadSyncCounter;
				LOG4_DEBUG("succ to Decode for strTableName(%s),local file(%s),ReadCounter(%u)",
								m_strTableName.c_str(),m_strReadingFile.c_str(),m_ReadCounter);
				m_iSendingSize = oMsgHead.ByteSize() + oMsgHead.msgbody_len();
				return(true);
			}
			else if (net::CODEC_STATUS_ERR == eCodecStatus)
			{
				LOG4_ERROR("failed to Decode,data file(%s)",m_strReadingFile.c_str());
				break;
			}
		}
        iReadLen = m_pBuffR->ReadFD(m_iReadingFd, iErrno);
        if(iReadLen < 0)
        {
            LOG4_ERROR("failed to read from file(%s),iErrno(%d)",
                            m_strReadingFile.c_str(),iErrno);
            break;
        }
        else if (iReadLen > 0)
        {
            net::E_CODEC_STATUS eCodecStatus = m_pCodec->Decode(m_pBuffR, oMsgHead, oMsgBody);
            if (net::CODEC_STATUS_OK == eCodecStatus)
            {
                ++m_ReadCounter;
                ++g_uiReadSyncCounter;
                LOG4_DEBUG("succ to Decode for strTableName(%s),local file(%s),ReadCounter(%u)",
                                m_strTableName.c_str(),m_strReadingFile.c_str(),m_ReadCounter);
                m_iSendingSize = oMsgHead.ByteSize() + oMsgHead.msgbody_len();
                return(true);
            }
            else if (net::CODEC_STATUS_ERR == eCodecStatus)
            {
                LOG4_ERROR("failed to Decode,data file(%s)",m_strReadingFile.c_str());
                break;
            }
        }
        else
        {
            LOG4_DEBUG("file(%s) has notings to read now",m_strReadingFile.c_str());
        }
    }
    while (iReadLen > 0);
    close(m_iReadingFd);
    m_iReadingFd = -1;
    unlink(std::string(m_strDataPath + m_strReadingFile).c_str());
    LOG4_INFO("%s().remove file(%s) for sync data.",__FUNCTION__,std::string(m_strDataPath + m_strReadingFile).c_str());
    LOG4_DEBUG("%s().g_uiReadSyncCounter(%llu),g_uiWriteSyncCounter(%llu)",__FUNCTION__,g_uiReadSyncCounter,g_uiWriteSyncCounter);
    m_setDataFiles.erase(m_strReadingFile);
    m_strReadingFile.clear();
    return false;
}

void SessionSyncDbData::GoBack(const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (NULL == m_pCodec)
    {
        m_pCodec = new net::ProtoCodec(util::CODEC_PROTOBUF);
    }
    if (m_iSendingSize == (oMsgHead.ByteSize() + (int)oMsgHead.msgbody_len()))
    {
    	m_pCodec->Encode(oMsgHead, oMsgBody, m_pBuffWaitR);//一般m_pBuffWaitR最多只有一个消息
    	LOG4_TRACE("%s() m_pBuffWaitR ReadableBytes(%u)", __FUNCTION__,m_pBuffR->ReadableBytes());
    }
    else
    {
    	LOG4_ERROR("%s() m_iSendingSize(%u) != msg size(%d) m_pBuffWaitR ReadableBytes(%u)",
    			__FUNCTION__,m_iSendingSize,(oMsgHead.ByteSize() + (int)oMsgHead.msgbody_len()),m_pBuffWaitR->ReadableBytes());
    }
}

bool SessionSyncDbData::ScanSyncData()
{
    LOG4_TRACE("%s() scan files name to sync from strDataPath(%s)", __FUNCTION__,m_strDataPath.c_str());
    DIR* dir;
    struct dirent* dirent_ptr;
    dir = opendir(m_strDataPath.c_str());
    if (dir != NULL)
    {
        size_t uiCurrentPos = 0;
        size_t uiNextPos = 0;
        std::string strFileName;
        std::string strWorkerIndex;
        while ((dirent_ptr = readdir(dir)) != NULL)
        {
            strFileName = dirent_ptr->d_name;
            if (strFileName.size() > 4 && std::string(".dat") == strFileName.substr(strFileName.size() - 4, 4))
            {
                //strFileName(0.tb_userinfo.201610191341.dat),strTableName(tb_userinfo)
                LOG4_DEBUG("strFileName(%s),strTableName(%s)",strFileName.c_str(),m_strTableName.c_str());
                uiNextPos = strFileName.find('.', uiCurrentPos);
                strWorkerIndex = strFileName.substr(uiCurrentPos, uiNextPos - uiCurrentPos);//strWorkerIndex 0
                if (std::string::npos != strFileName.find(m_strTableName)
                                && strtoul(strWorkerIndex.c_str(), NULL, 10) == net::GetWorkerIndex())//只处理自己工作者的该表的同步文件
                {
                    //strTableName(tb_userinfo)
                	LOG4_TRACE("setDataFiles add strTableName(%s)",m_strTableName.c_str());
                    m_setDataFiles.insert(strFileName);
                }
            }
            else
            {
                LOG4_TRACE("skip strFileName(%s)",strFileName.c_str());
            }
        }
        closedir(dir);
        return(true);
    }
    else
    {
        LOG4_ERROR("open dir %s error %d", m_strDataPath.c_str(), errno);
        return(false);
    }
}

SessionSyncDbData* GetSessionSyncDbData(const std::string& strTableName, const std::string& strDataPath, ev_tstamp dSessionTimeout)
{
	SessionSyncDbData* pSessionSyncDbData = (SessionSyncDbData*)net::GetSession(strTableName,"net::SessionSyncDbData");
	if(NULL == pSessionSyncDbData)
	{
		pSessionSyncDbData = new SessionSyncDbData(strTableName, strDataPath);
		if (!net::RegisterCallback(pSessionSyncDbData))//每个表有一个定时器检查同步
		{
			delete pSessionSyncDbData;
			pSessionSyncDbData = NULL;
			LOG4_ERROR("failed to RegisterCallback(pSessionSyncDbData) for table(%s)",strTableName.c_str());
			return NULL;//这里一般不会发生
		}
	}
	return pSessionSyncDbData;
}

} /* namespace core */
