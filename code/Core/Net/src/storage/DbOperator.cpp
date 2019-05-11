/*******************************************************************************
 * Project:  DataProxyServer
 * @file     DbOperater.cpp
 * @brief 
 * @author   cjy
 * @date:    2015年11月19日
 * @note
 * Modify history:
 ******************************************************************************/
#include "DbOperator.hpp"

namespace net
{

DbOperator::DbOperator(
                uint64 uiSectionFactor,
                const std::string& strTableName,
                DataMem::MemOperate::DbOperate::E_QUERY_TYPE eQueryType,
                uint64 uiModFactor)
    : m_pDbMemRequest(NULL), m_pDbOperate(NULL), m_uiSectionFactor(uiSectionFactor)
{
    m_pDbOperate = new DataMem::MemOperate::DbOperate();
    m_pDbOperate->set_table_name(strTableName);
    m_pDbOperate->set_query_type(eQueryType);
    if (uiModFactor > 0)
    {
        m_pDbOperate->set_mod_factor(uiModFactor);
    }
    else if (uiSectionFactor > 0)
    {
        m_pDbOperate->set_mod_factor(uiSectionFactor);
    }
}

DbOperator::~DbOperator()
{
    if (m_pDbMemRequest != NULL)
    {
        delete m_pDbMemRequest;
        m_pDbMemRequest = NULL;
    }
    else
    {
        if (m_pDbOperate != NULL)
        {
            delete m_pDbOperate;
            m_pDbOperate = NULL;
        }
    }
}

DataMem::MemOperate* DbOperator::MakeMemOperate()
{
    if (m_pDbMemRequest == NULL)
    {
        m_pDbMemRequest = new DataMem::MemOperate();
    }
    else
    {
        return(m_pDbMemRequest);
    }
    m_pDbMemRequest->set_section_factor(m_uiSectionFactor);
    m_pDbMemRequest->set_allocated_db_operate(m_pDbOperate);
    return(m_pDbMemRequest);
}

bool DbOperator::AddDbField(const std::string& strFieldName, const std::string& strFieldValue,
                DataMem::E_COL_TYPE eFieldType, const std::string& strColAs,
                bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    DataMem::Field* pField = m_pDbOperate->add_fields();
    pField->set_col_name(strFieldName);
    pField->set_col_type(eFieldType);
    pField->set_col_value(strFieldValue);
    if (strColAs.size() > 0)
    {
        pField->set_col_as(strColAs);
    }
    if (bGroupBy)
    {
        m_pDbOperate->add_groupby_col(strFieldName);
    }
    if (bOrderBy)
    {
        DataMem::MemOperate::DbOperate::OrderBy *pOrderBy = m_pDbOperate->add_orderby_col();
        pOrderBy->set_col_name(strFieldName);
        if (std::string("DESC") == strOrder || std::string("desc") == strOrder)
        {
            pOrderBy->set_relation(DataMem::MemOperate::DbOperate::OrderBy::DESC);
        }
        else
        {
            pOrderBy->set_relation(DataMem::MemOperate::DbOperate::OrderBy::ASC);
        }
    }
    return(true);
}

bool DbOperator::AddDbField(const std::string& strFieldName, int32 iFieldValue,
                const std::string& strColAs, bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%d", iFieldValue);
    return(AddDbField(strFieldName, (std::string)szFieldValue, DataMem::INT, strColAs, bGroupBy, bOrderBy, strOrder));
}

bool DbOperator::AddDbField(const std::string& strFieldName, uint32 uiFieldValue,
                const std::string& strColAs, bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lu", uiFieldValue);
    return(AddDbField(strFieldName, (std::string)szFieldValue, DataMem::INT, strColAs, bGroupBy, bOrderBy, strOrder));
}

bool DbOperator::AddDbField(const std::string& strFieldName, int64 llFieldValue,
                const std::string& strColAs, bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lld", llFieldValue);
    return(AddDbField(strFieldName, (std::string)szFieldValue, DataMem::BIGINT, strColAs, bGroupBy, bOrderBy, strOrder));
}

bool DbOperator::AddDbField(const std::string& strFieldName, uint64 ullFieldValue,
                const std::string& strColAs, bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%llu", ullFieldValue);
    return(AddDbField(strFieldName, (std::string)szFieldValue, DataMem::BIGINT, strColAs, bGroupBy, bOrderBy, strOrder));
}

bool DbOperator::AddDbField(const std::string& strFieldName, float fFieldValue,
                const std::string& strColAs, bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lf", fFieldValue);
    return(AddDbField(strFieldName, (std::string)szFieldValue, DataMem::FLOAT, strColAs, bGroupBy, bOrderBy, strOrder));
}

bool DbOperator::AddDbField(const std::string& strFieldName, double dFieldValue,
                const std::string& strColAs, bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lf", dFieldValue);
    return(AddDbField(strFieldName, (std::string)szFieldValue, DataMem::DOUBLE, strColAs, bGroupBy, bOrderBy, strOrder));
}

bool DbOperator::AddCondition(DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, const std::string& strFieldValue,
                DataMem::E_COL_TYPE eFieldType, const std::string& strRightFieldName)
{
    DataMem::MemOperate::DbOperate::ConditionGroup* pConditionGroup;
    DataMem::MemOperate::DbOperate::Condition* pCondition;
    if (m_pDbOperate->conditions_size() > 0)
    {
        int iConditionIdx = m_pDbOperate->conditions_size() - 1;
        pConditionGroup = m_pDbOperate->mutable_conditions(iConditionIdx);
        pCondition = pConditionGroup->add_condition();
    }
    else
    {
        pConditionGroup = m_pDbOperate->add_conditions();
        pCondition = pConditionGroup->add_condition();
    }
    pConditionGroup->set_relation(DataMem::MemOperate::DbOperate::ConditionGroup::AND);
    pCondition->set_relation(eRelation);
    pCondition->set_col_name(strFieldName);
    pCondition->set_col_type(eFieldType);
    if (strRightFieldName.length() > 0)
    {
        pCondition->set_col_name_right(strRightFieldName);
    }
    else
    {
        pCondition->add_col_values(strFieldValue);
    }
    return(true);
}

bool DbOperator::AddCondition(DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, int32 iFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%d", iFieldValue);
    return(AddCondition(eRelation, strFieldName, szFieldValue, DataMem::INT, strRightFieldName));
}

bool DbOperator::AddCondition(DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, uint32 uiFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%u", uiFieldValue);
    return(AddCondition(eRelation, strFieldName, szFieldValue, DataMem::INT, strRightFieldName));
}

bool DbOperator::AddCondition(DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, int64 llFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lld", llFieldValue);
    return(AddCondition(eRelation, strFieldName, szFieldValue, DataMem::BIGINT, strRightFieldName));
}

bool DbOperator::AddCondition(DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, uint64 ullFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%llu", ullFieldValue);
    return(AddCondition(eRelation, strFieldName, szFieldValue, DataMem::BIGINT, strRightFieldName));
}

bool DbOperator::AddCondition(DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, float fFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%f", fFieldValue);
    return(AddCondition(eRelation, strFieldName, szFieldValue, DataMem::BIGINT, strRightFieldName));
}

bool DbOperator::AddCondition(DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, double dFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lf", dFieldValue);
    return(AddCondition(eRelation, strFieldName, szFieldValue, DataMem::BIGINT, strRightFieldName));
}

bool DbOperator::AddCondition(DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                    const std::string& strFieldName, const std::vector<uint32>& vecFieldValues)
{
    char szFieldValue[64] = {0};
    DataMem::MemOperate::DbOperate::ConditionGroup* pConditionGroup;
    DataMem::MemOperate::DbOperate::Condition* pCondition;
    if (m_pDbOperate->conditions_size() > 0)
    {
        int iConditionIdx = m_pDbOperate->conditions_size() - 1;
        pConditionGroup = m_pDbOperate->mutable_conditions(iConditionIdx);
        pCondition = pConditionGroup->add_condition();
    }
    else
    {
        pConditionGroup = m_pDbOperate->add_conditions();
        pCondition = pConditionGroup->add_condition();
    }
    pConditionGroup->set_relation(DataMem::MemOperate::DbOperate::ConditionGroup::AND);
    pCondition->set_relation(eRelation);
    pCondition->set_col_name(strFieldName);
    pCondition->set_col_type(DataMem::INT);
    for (std::vector<uint32>::const_iterator c_iter = vecFieldValues.begin();
                    c_iter != vecFieldValues.end(); ++c_iter)
    {
        snprintf(szFieldValue, sizeof(szFieldValue), "%u", *c_iter);
        pCondition->add_col_values(szFieldValue);
    }
    return(true);
}

bool DbOperator::AddCondition(DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, const std::vector<uint64>& vecFieldValues)
{
    char szFieldValue[64] = {0};
    DataMem::MemOperate::DbOperate::ConditionGroup* pConditionGroup;
    DataMem::MemOperate::DbOperate::Condition* pCondition;
    if (m_pDbOperate->conditions_size() > 0)
    {
        int iConditionIdx = m_pDbOperate->conditions_size() - 1;
        pConditionGroup = m_pDbOperate->mutable_conditions(iConditionIdx);
        pCondition = pConditionGroup->add_condition();
    }
    else
    {
        pConditionGroup = m_pDbOperate->add_conditions();
        pCondition = pConditionGroup->add_condition();
    }
    pConditionGroup->set_relation(DataMem::MemOperate::DbOperate::ConditionGroup::AND);
    pCondition->set_relation(eRelation);
    pCondition->set_col_name(strFieldName);
    pCondition->set_col_type(DataMem::BIGINT);
    for (std::vector<uint64>::const_iterator c_iter = vecFieldValues.begin();
                    c_iter != vecFieldValues.end(); ++c_iter)
    {
        snprintf(szFieldValue, sizeof(szFieldValue), "%llu", *c_iter);
        pCondition->add_col_values(szFieldValue);
    }
    return(true);
}

bool DbOperator::AddCondition(DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, const std::vector<std::string>& vecFieldValues)
{
    DataMem::MemOperate::DbOperate::ConditionGroup* pConditionGroup;
    DataMem::MemOperate::DbOperate::Condition* pCondition;
    if (m_pDbOperate->conditions_size() > 0)
    {
        int iConditionIdx = m_pDbOperate->conditions_size() - 1;
        pConditionGroup = m_pDbOperate->mutable_conditions(iConditionIdx);
        pCondition = pConditionGroup->add_condition();
    }
    else
    {
        pConditionGroup = m_pDbOperate->add_conditions();
        pCondition = pConditionGroup->add_condition();
    }
    pConditionGroup->set_relation(DataMem::MemOperate::DbOperate::ConditionGroup::AND);
    pCondition->set_relation(eRelation);
    pCondition->set_col_name(strFieldName);
    pCondition->set_col_type(DataMem::STRING);
    for (std::vector<std::string>::const_iterator c_iter = vecFieldValues.begin();
                    c_iter != vecFieldValues.end(); ++c_iter)
    {
        pCondition->add_col_values(*c_iter);
    }
    return(true);
}

bool DbOperator::AddCondition(int iGroupIdx,
                DataMem::MemOperate::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, const std::string& strFieldValue,
                DataMem::E_COL_TYPE eFieldType, const std::string& strRightFieldName)
{
    if (iGroupIdx >= m_pDbOperate->conditions_size())
    {
        DataMem::MemOperate::DbOperate::ConditionGroup* pConditionGroup = m_pDbOperate->add_conditions();
        pConditionGroup->set_relation(eGroupRelation);
        DataMem::MemOperate::DbOperate::Condition* pCondition = pConditionGroup->add_condition();
        pCondition->set_relation(eRelation);
        pCondition->set_col_name(strFieldName);
        pCondition->set_col_type(eFieldType);
        if (strRightFieldName.length() > 0)
        {
            pCondition->set_col_name_right(strRightFieldName);
        }
        else
        {
            pCondition->add_col_values(strFieldValue);
        }
    }
    else
    {
        DataMem::MemOperate::DbOperate::Condition* pCondition = m_pDbOperate->mutable_conditions(iGroupIdx)->add_condition();
        pCondition->set_relation(eRelation);
        pCondition->set_col_name(strFieldName);
        pCondition->set_col_type(eFieldType);
        if (strRightFieldName.length() > 0)
        {
            pCondition->set_col_name_right(strRightFieldName);
        }
        else
        {
            pCondition->add_col_values(strFieldValue);
        }
    }
    return(true);
}

bool DbOperator::AddCondition(int iGroupIdx,
                DataMem::MemOperate::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, int32 iFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%d", iFieldValue);
    return(AddCondition(iGroupIdx, eGroupRelation, eRelation, strFieldName, szFieldValue, DataMem::INT, strRightFieldName));
}

bool DbOperator::AddCondition(int iGroupIdx,
                DataMem::MemOperate::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, uint32 uiFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%u", uiFieldValue);
    return(AddCondition(iGroupIdx, eGroupRelation, eRelation, strFieldName, szFieldValue, DataMem::INT, strRightFieldName));
}

bool DbOperator::AddCondition(int iGroupIdx,
                DataMem::MemOperate::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, int64 llFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lld", llFieldValue);
    return(AddCondition(iGroupIdx, eGroupRelation, eRelation, strFieldName, szFieldValue, DataMem::BIGINT, strRightFieldName));
}

bool DbOperator::AddCondition(int iGroupIdx,
                DataMem::MemOperate::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, uint64 ullFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%llu", ullFieldValue);
    return(AddCondition(iGroupIdx, eGroupRelation, eRelation, strFieldName, szFieldValue, DataMem::BIGINT, strRightFieldName));
}

bool DbOperator::AddCondition(int iGroupIdx,
                DataMem::MemOperate::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, float fFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%f", fFieldValue);
    return(AddCondition(iGroupIdx, eGroupRelation, eRelation, strFieldName, szFieldValue, DataMem::FLOAT, strRightFieldName));
}

bool DbOperator::AddCondition(int iGroupIdx,
                DataMem::MemOperate::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                DataMem::MemOperate::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, double dFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[60] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lf", dFieldValue);
    return(AddCondition(iGroupIdx, eGroupRelation, eRelation, strFieldName, szFieldValue, DataMem::DOUBLE, strRightFieldName));
}

void DbOperator::SetConditionGroupRelation(DataMem::MemOperate::DbOperate::ConditionGroup::E_RELATION eRelation)
{
    m_pDbOperate->set_group_relation(eRelation);
}

void DbOperator::AddLimit(unsigned int uiLimit, unsigned int uiLimitFrom)
{
    m_pDbOperate->set_limit(uiLimit);
    if (uiLimitFrom > 0)
    {
        m_pDbOperate->set_limit_from(uiLimitFrom);
    }
}

} /* namespace net */
