/*******************************************************************************
 * Project:  DataProxyServer
 * @file     RedisOperator.cpp
 * @brief 
 * @author   cjy
 * @date:    2019年11月19日
 * @note
 * Modify history:
 ******************************************************************************/
#include "RedisOperator.hpp"

namespace net
{

RedisOperator::RedisOperator(
                uint64 uiSectionFactor,
                const std::string& strRedisKey,
                const std::string& strWriteCmd,
                const std::string& strReadCmd)
    : m_pRedisMemRequest(NULL), m_pRedisOperate(NULL), m_uiSectionFactor(uiSectionFactor)
{
    std::string strSplitData[3];
    int iSegNo = 0;
    int iPosBegin = 0;
    int iPosEnd = 0;
    for (iSegNo = 0; iSegNo < 3; ++iSegNo)
    {
        iPosEnd = strRedisKey.find(':', iPosBegin);
        strSplitData[iSegNo] = strRedisKey.substr(iPosBegin, iPosEnd - iPosBegin);
        iPosBegin = iPosEnd + 1;
    }
    m_pRedisOperate = new DataMem::MemOperate::RedisOperate();
    m_pRedisOperate->set_key_name(strRedisKey);
    m_pRedisOperate->set_redis_cmd_read(strReadCmd);
    m_pRedisOperate->set_redis_cmd_write(strWriteCmd);
    m_pRedisOperate->set_redis_structure(atoi(strSplitData[0].c_str()));
    m_pRedisOperate->set_data_purpose(atoi(strSplitData[1].c_str()));
    m_pRedisOperate->set_hash_key(strSplitData[2]);
    if (strReadCmd.length() == 0)
    {
        m_pRedisOperate->set_op_type(DataMem::MemOperate::RedisOperate::T_WRITE);
    }
    else
    {
        m_pRedisOperate->set_op_type(DataMem::MemOperate::RedisOperate::T_READ);
    }
}

RedisOperator::~RedisOperator()
{
    if (m_pRedisMemRequest != NULL)
    {
        delete m_pRedisMemRequest;
        m_pRedisMemRequest = NULL;
    }
    else
    {
        if (m_pRedisOperate != NULL)
        {
            delete m_pRedisOperate;
            m_pRedisOperate = NULL;
        }
    }
}

DataMem::MemOperate* RedisOperator::MakeMemOperate()
{
    if (m_pRedisMemRequest == NULL)
    {
        m_pRedisMemRequest = new DataMem::MemOperate();
    }
    else
    {
        return(m_pRedisMemRequest);
    }
    m_pRedisMemRequest->set_section_factor(m_uiSectionFactor);
    m_pRedisMemRequest->set_allocated_redis_operate(m_pRedisOperate);
    return(m_pRedisMemRequest);
}

bool RedisOperator::AddRedisField(const std::string& strFieldName, const std::string& strFieldValue)
{
    DataMem::Field* pField = m_pRedisOperate->add_fields();
    pField->set_col_name(strFieldName.c_str(),strFieldName.size());
    pField->set_col_value(strFieldValue.c_str(),strFieldValue.size());
    return(true);
}

bool RedisOperator::AddRedisField(const std::string& strFieldName, int32 iFieldValue)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%d", iFieldValue);
    return(AddRedisField(strFieldName, szFieldValue));
}

bool RedisOperator::AddRedisField(const std::string& strFieldName, uint32 uiFieldValue)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%u", uiFieldValue);
    return(AddRedisField(strFieldName, szFieldValue));
}

bool RedisOperator::AddRedisField(const std::string& strFieldName, int64 llFieldValue)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lld", llFieldValue);
    return(AddRedisField(strFieldName, szFieldValue));
}

bool RedisOperator::AddRedisField(const std::string& strFieldName, uint64 ullFieldValue)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%llu", ullFieldValue);
    return(AddRedisField(strFieldName, szFieldValue));
}

bool RedisOperator::AddRedisField(const std::string& strFieldName, float fFieldValue)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%f", fFieldValue);
    return(AddRedisField(strFieldName, szFieldValue));
}

bool RedisOperator::AddRedisField(const std::string& strFieldName, double dFieldValue)
{
    char szFieldValue[60] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lf", dFieldValue);
    return(AddRedisField(strFieldName, szFieldValue));
}

} /* namespace net */
