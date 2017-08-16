/*******************************************************************************
 * Project:  DataProxyServer
 * @file     MemOperator.hpp
 * @brief 
 * @author   cjy
 * @date:    2017年11月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef CLIENT_MEMOPERATOR_HPP_
#define CLIENT_MEMOPERATOR_HPP_

#include "DbOperator.hpp"
#include "RedisOperator.hpp"

namespace thunder
{

enum E_MEM_FIELD_OPERATOR
{
    FIELD_OPERATOR_DB       = 0x0001,    ///< 对数据库有效
    FIELD_OPERATOR_REDIS    = 0x0002,    ///< 对Redis有效
};

/**
 * @brief StarStorage存储请求协议生成器
 * @note 存储请求协议生成，用于同时需要请求Redis和数据库的场景。
 */
class MemOperator: public RedisOperator, public DbOperator
{
public:
    /**
     * @brief 存储请求协议生成
     * @note 存储请求协议生成，用于同时需要请求Redis和数据库的场景。
     * @param uiSectionFactor   hash分段因子
     * @param strTableName      表名
     * @param eQueryType        查询类型（SELECT，INSERT，REPLACE等）
     * @param strRedisKey       Redis key
     * @param strWriteCmd       Redis写命令（若是读，写命令用于Redis未命中，DataProxy从DB中读取到数据写回Redis时使用）
     * @param strReadCmd        Redis读命令（若是写，读命令为""空串）
     * @param uiModFactor       分表取模因子，当该参数为0时使用uiSectionFactor
     */
    MemOperator(
                    uint64 uiSectionFactor,
                    const std::string& strTableName,
                    DataMem::MemOperate::DbOperate::E_QUERY_TYPE eQueryType,
                    const std::string& strRedisKey,
                    const std::string& strWriteCmd,
                    const std::string& strReadCmd = "",
                    uint64 uiModFactor = 0);
    virtual ~MemOperator();

    virtual DataMem::MemOperate* MakeMemOperate();

    /**
     * @brief 添加字段
     * @param strFieldName 字段名
     * @param strFieldValue 字段值
     * @param eFieldType 字段类型
     * @param bGroupBy 是否作为GroupBy字段
     * @param bOrderBy 是否作为OrderBy字段
     * @param strOrder OrederBy方式（DESC AESC）
     * @return 是否添加成功
     */
    virtual bool AddField(const std::string& strFieldName, const std::string& strFieldValue = "",
                    DataMem::E_COL_TYPE eFieldType = DataMem::STRING, int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");

    virtual bool AddField(const std::string& strFieldName, int32 iFieldValue,
                    int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddField(const std::string& strFieldName, uint32 uiFieldValue,
                    int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddField(const std::string& strFieldName, int64 llFieldValue,
                    int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddField(const std::string& strFieldName, uint64 ullFieldValue,
                    int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddField(const std::string& strFieldName, float fFieldValue,
                    int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddField(const std::string& strFieldName, double dFieldValue,
                    int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");

private:
    DataMem::MemOperate* m_pMemRequest;
    uint64 m_uiSectionFactor;
};

} /* namespace thunder */

#endif /* CLIENT_MEMOPERATOR_HPP_ */
