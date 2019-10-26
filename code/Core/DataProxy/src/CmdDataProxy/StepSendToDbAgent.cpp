/*******************************************************************************
 * Project:  DataProxy
 * @file     StepReadFromDb.cpp
 * @brief 
 * @author   cjy
 * @date:    2016年3月19日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepSendToDbAgent.hpp"

namespace core
{

StepSendToDbAgent::StepSendToDbAgent(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead,
                const DataMem::MemOperate& oMemOperate,SessionRedisNode* pNodeSession,int iRelative,
                const util::CJsonObject* pTableField,const std::string& strKeyField,const util::CJsonObject* pJoinField)
    : m_stMsgShell(stMsgShell), m_oReqMsgHead(oInMsgHead), m_oMemOperate(oMemOperate),
      m_iRelative(iRelative), m_strKeyField(strKeyField), m_oTableField(pTableField), m_oJoinField(pJoinField),
      m_bFieldFilter(false), m_bNeedResponse(true),
      m_pNodeSession(pNodeSession), pStepWriteBackToRedis(NULL)
{
}

StepSendToDbAgent::~StepSendToDbAgent()
{
}

net::E_CMD_STATUS StepSendToDbAgent::Emit(int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    if (DataMem::MemOperate::DbOperate::SELECT != m_oMemOperate.db_operate().query_type() && m_oMemOperate.has_redis_operate())
    {
        m_bNeedResponse = false;//写操作需要检查结果集后返回
    }
    LOG4_TRACE("%s(m_bNeedResponse = %d) m_oMemOperate(%s)", __FUNCTION__, m_bNeedResponse,m_oMemOperate.DebugString().c_str());
    if (RELATIVE_DATASET == m_iRelative && DataMem::MemOperate::DbOperate::SELECT == m_oMemOperate.db_operate().query_type())
    {
        for (int i = 0; i < m_oMemOperate.db_operate().fields_size(); ++i)
        {
            if ((m_oTableField(i) != m_oMemOperate.db_operate().fields(i).col_name())
            		&& (m_oMemOperate.db_operate().fields(i).col_as().size() && m_oTableField(i) != m_oMemOperate.db_operate().fields(i).col_as()))
            {
                m_bFieldFilter = true;
                break;
            }
        }
        // 客户端请求表的部分字段，但redis需要所有字段来回写，故修改向数据库的请求数据，待返回结果集再筛选部分字段回复客户端，所有字段用来回写redis
        if (m_bFieldFilter)
        {
            DataMem::MemOperate oMemOperate = m_oMemOperate;
            DataMem::MemOperate::DbOperate* oDbOper = oMemOperate.mutable_db_operate();
            oDbOper->clear_fields();
            for (int i = 0; i < m_oTableField.GetArraySize(); ++i)
            {
                DataMem::Field* pField = oDbOper->add_fields();
                pField->set_col_name(m_oTableField(i));
            }
            oOutMsgBody.set_body(oMemOperate.SerializeAsString());
        }
        else
        {
            oOutMsgBody.set_body(m_oMemOperate.SerializeAsString());
        }
    }
    else
    {
        oOutMsgBody.set_body(m_oMemOperate.SerializeAsString());
    }
    oOutMsgHead.set_cmd(net::CMD_REQ_STORATE);
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());

    if (m_oMemOperate.db_operate().SELECT == m_oMemOperate.db_operate().query_type())
    {
        if (!net::SendToNext(AGENT_R, oOutMsgHead, oOutMsgBody))
        {
            LOG4_ERROR("SendToNext(\"%s\") error:%d!",AGENT_R,ERR_DATA_TRANSFER);
            Response(m_stMsgShell, m_oReqMsgHead, ERR_DATA_TRANSFER, "SendToNext(\"DBAGENT_R\") error!");
            return(net::STATUS_CMD_FAULT);
        }
    }
    else
    {
        if (!net::SendToNext(AGENT_W, oOutMsgHead, oOutMsgBody))
        {
        	LOG4_ERROR("SendToNext(\"%s\") error:%d!",AGENT_W,ERR_DATA_TRANSFER);
            SessionSyncDbData* pSessionSync = GetSessionSyncDbData(m_oMemOperate.db_operate().table_name(),net::GetWorkPath() + SYNC_DATA_DIR);
            if(pSessionSync)
            {
                LOG4_INFO("%s() SyncData to local files", __FUNCTION__);
                if (pSessionSync->SyncData(m_oMemOperate))
                {
                	Response(m_stMsgShell, m_oReqMsgHead, ERR_OK, "SyncData to local files ok!");//需要回应
                }
                else
                {
                	LOG4_ERROR("SyncData to local files error!");
                	Response(m_stMsgShell, m_oReqMsgHead, ERR_DATA_TRANSFER, "SyncData to local files error!");
                }
                return(net::STATUS_CMD_COMPLETED);
            }
            LOG4_ERROR("pSessionSync null,failed to SyncData,error:%d",ERR_DATA_TRANSFER);
            Response(m_stMsgShell, m_oReqMsgHead, ERR_DATA_TRANSFER, "SendToNext(\"DBAGENT_W\") error!");
            return(net::STATUS_CMD_FAULT);
        }
    }
    return(net::STATUS_CMD_RUNNING);
}

net::E_CMD_STATUS StepSendToDbAgent::Callback(const net::tagMsgShell& stMsgShell,const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, void* data)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    MsgHead oOutMsgHead = m_oReqMsgHead;
    MsgBody oOutMsgBody;
    DataMem::MemRsp oRsp;
    if (!oRsp.ParseFromString(oInMsgBody.body()))
    {
        LOG4_ERROR("DataMem::MemRsp oRsp.ParseFromString() failed!");
        if (m_bNeedResponse)
        {
            oOutMsgBody.set_body(oInMsgBody.body());
            oOutMsgHead.set_cmd(m_oReqMsgHead.cmd() + 1);
            oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
            if (!SendTo(m_stMsgShell, oOutMsgHead, oOutMsgBody))
            {
                LOG4_ERROR("send to tagMsgShell(fd %d, seq %u) error!", m_stMsgShell.iFd, m_stMsgShell.ulSeq);
                return(net::STATUS_CMD_FAULT);
            }
        }
        return(net::STATUS_CMD_FAULT);
    }

    WriteBackToRedis(m_stMsgShell, m_oReqMsgHead, oRsp);

    if (m_bNeedResponse)
    {
        if (m_bFieldFilter)     // 需筛选回复字段
        {
            DataMem::MemRsp oRspToClient;
            oRspToClient.set_err_no(oRsp.err_no());
            oRspToClient.set_err_msg(oRsp.err_msg());
            std::vector<int> vecColForClient;
            std::vector<int>::iterator iter;
            for (int i = 0; i < m_oMemOperate.db_operate().fields_size(); ++i)
            {
                for (int j = 0; j < m_oTableField.GetArraySize(); ++j)
                {
                    if ((m_oTableField(j) == m_oMemOperate.db_operate().fields(i).col_name())
                                || (m_oTableField(j) == m_oMemOperate.db_operate().fields(i).col_as()))
                    {
                        vecColForClient.push_back(j);
                        break;
                    }
                }
            }
            if(RELATIVE_DATASET == m_iRelative)
            {
                if (oRsp.record_data_size() > 0)
                {
                    ::DataMem::Record* pAddRecord = oRspToClient.add_record_data();
                    for (int i = 0; i < oRsp.record_data_size(); ++i)//把db返回的每个结果Record作为返回客户端Record里的一个Field
                    {
                        ::DataMem::Record oRecord;
                        for (int j = 0; j < oRsp.record_data(i).field_info_size(); ++j)
                        {
                            DataMem::Field* pField = oRecord.add_field_info();
                            for (iter = vecColForClient.begin(); iter != vecColForClient.end(); ++iter)
                            {
                                pField->set_col_value(oRsp.record_data(i).field_info(*iter).col_value());
                            }
                        }
                        ::DataMem::Field* pAddField = pAddRecord->add_field_info();
                        pAddField->set_col_value(oRecord.SerializeAsString());
                    }
                }
                LOG4_DEBUG("send to tagMsgShell(fd %d, seq %u) oRspToClient(%s)!",m_stMsgShell.iFd, m_stMsgShell.ulSeq,oRspToClient.DebugString().c_str());
            }
            else
            {
                for(int i = 0; i < oRsp.record_data_size(); i++)
                {
                    DataMem::Record* pRecord = oRspToClient.add_record_data();
                    for (iter = vecColForClient.begin(); iter != vecColForClient.end(); ++iter)
                    {
                        DataMem::Field* pField = pRecord->add_field_info();
                        pField->set_col_value(oRsp.record_data(i).field_info(*iter).col_value());
                    }
                }
                LOG4_DEBUG("send to tagMsgShell(fd %d, seq %u) oRspToClient(%s)!",m_stMsgShell.iFd, m_stMsgShell.ulSeq,oRspToClient.DebugString().c_str());
            }
            oOutMsgBody.set_body(oRspToClient.SerializeAsString());
        }
        else
        {
            if(RELATIVE_DATASET == m_iRelative)
            {
                DataMem::MemRsp oRspToClient;
                oRspToClient.set_err_no(oRsp.err_no());
                oRspToClient.set_err_msg(oRsp.err_msg());
                if (oRsp.record_data_size() > 0)
                {
                    ::DataMem::Record* pAddRecord = oRspToClient.add_record_data();
                    for (int i = 0; i < oRsp.record_data_size(); ++i)//把db返回的每个结果Record作为返回客户端Record里的一个Field
                    {
                        ::DataMem::Field* pField = pAddRecord->add_field_info();
                        pField->set_col_value(oRsp.record_data(i).SerializeAsString());
                    }
                }
                LOG4_DEBUG("send to tagMsgShell(fd %d, seq %u) oRspToClient(%s)!",
                                m_stMsgShell.iFd, m_stMsgShell.ulSeq,oRspToClient.DebugString().c_str());
                oOutMsgBody.set_body(oRspToClient.SerializeAsString());
            }
            else
            {
                LOG4_DEBUG("send to tagMsgShell(fd %d, seq %u) oInMsgBody.body(%s)!",m_stMsgShell.iFd, m_stMsgShell.ulSeq,oInMsgBody.body().c_str());
                oOutMsgBody.set_body(oInMsgBody.body());
            }
        }
        oOutMsgHead.set_cmd(m_oReqMsgHead.cmd() + 1);
        oOutMsgHead.set_msgbody_len(oOutMsgBody.ByteSize());
        if (!SendTo(m_stMsgShell, oOutMsgHead, oOutMsgBody))
        {
            LOG4_ERROR("send to tagMsgShell(fd %d, seq %u) error!", m_stMsgShell.iFd, m_stMsgShell.ulSeq);
            return(net::STATUS_CMD_FAULT);
        }
    }
    else    // 无需回复请求方
    {
        if (ERR_OK != oRsp.err_no())
        {
            LOG4_ERROR("%d: %s", oRsp.err_no(), oRsp.err_msg().c_str());
            if (DataMem::MemOperate::DbOperate::SELECT != m_oMemOperate.db_operate().query_type()
                            && ((oRsp.err_no() >= 2001 && oRsp.err_no() <= 2018)
                            || (oRsp.err_no() >= 2038 && oRsp.err_no() <= 2046)))//网络原因写失败时同步文件
            {
                SessionSyncDbData* pSessionSync = GetSessionSyncDbData(m_oMemOperate.db_operate().table_name(),net::GetWorkPath() + SYNC_DATA_DIR);
                //发送后由于网络原因失败则写入文件,且扫描本地数据文件名
                if (pSessionSync && pSessionSync->SyncData(m_oMemOperate) && pSessionSync->ScanSyncData())
                {
                    LOG4_WARN("%d: %s. sync data", oRsp.err_no(), oRsp.err_msg().c_str());
                }
                else
                {
                    LOG4_ERROR("%d: %s. failed to sync data", oRsp.err_no(), oRsp.err_msg().c_str());
                }
            }
            return(net::STATUS_CMD_FAULT);
        }
    }

    if (oRsp.totalcount() == oRsp.curcount())
    {
        return(net::STATUS_CMD_COMPLETED);
    }
    return(net::STATUS_CMD_RUNNING);
}

net::E_CMD_STATUS StepSendToDbAgent::Timeout()
{
    Response(m_stMsgShell, m_oReqMsgHead, ERR_TIMEOUT, "read from db or write to db timeout!");
    return(net::STATUS_CMD_FAULT);
}

void StepSendToDbAgent::WriteBackToRedis(const net::tagMsgShell& stMsgShell, const MsgHead& oInMsgHead, const DataMem::MemRsp& oRsp)
{
    if (m_oMemOperate.has_redis_operate()
			&& (DataMem::MemOperate::RedisOperate::T_READ == m_oMemOperate.redis_operate().op_type())
			&& (ERR_OK == oRsp.err_no())
			&& (m_oMemOperate.redis_operate().redis_cmd_write().size()>=3))//写redis命为空，就不回写redis了
    {
        pStepWriteBackToRedis = new StepWriteBackToRedis(stMsgShell, oInMsgHead, m_oMemOperate,m_pNodeSession, m_iRelative, m_strKeyField, &m_oJoinField);
        g_pLabor->ExecStep(pStepWriteBackToRedis);
    }
}

} /* namespace core */
