/*******************************************************************************
 * Project:  DataProxyServer
 * @file     PgOperator.hpp
 * @brief
 * @author   cjy
 * @date:    2018年10月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef CLIENT_PGOPERATOR_HPP_
#define CLIENT_PGOPERATOR_HPP_
#include <string>
#include <map>
#include "MemOperator.hpp"

namespace net
{

//插入更新操作
struct PgUpsertOper
{
	enum eType
	{
		eType_int64,
		eType_string,
		eType_array_int,
		eType_array_text,
		eType_hll_int,
		eType_hll_text,
	};
	enum eKey
	{
		eKey_yes,//主键
		eKey_update,//更新字段
		eKey_addup,//累加字段
	};
	struct FieldValue
	{
		eType type;eKey key;
		std::string str;
		FieldValue(eType t,eKey k,int64 i):type(t),key(k){str = std::to_string(i);}
		FieldValue(eType t,eKey k,const std::string& s):type(t),key(k),str(s){}
	};
	PgUpsertOper(const std::string &table,const std::string& redisKey = "",DataMem::MemOperate::DbOperate::E_QUERY_TYPE type = DataMem::MemOperate::DbOperate::CUSTOM):
		strTable(table),oMemOperator(NULL),oDbOperator(NULL),m_OperType(type),m_uiLastRecordSize(0)
	{
		if (redisKey.size())oMemOperator = new net::MemOperator(0,table.c_str(),type,redisKey,"multi_hset");
		else oDbOperator = new net::DbOperator(0,table.c_str(),type);
	}
	~PgUpsertOper(){if (oMemOperator) delete oMemOperator;if (oDbOperator) delete oDbOperator;}
	void AddField(const std::string& strFieldName, const std::string& strFieldValue,eKey k = eKey_update)
	{
		if (DataMem::MemOperate::DbOperate::BULK == m_OperType)//批量插入(不支持hll类型和累加，不支持nosql),批量插入接口和每条插入接口不能同时使用
		{
			mapBulkFields[strFieldName].push_back(FieldValue(eType_string,eKey_update,strFieldValue));
		}
		else//每条插入
		{
			mapFields.insert(std::make_pair(strFieldName,FieldValue(eType_string,k,strFieldValue)));
			if (oMemOperator) oMemOperator->AddRedisField(strFieldName,strFieldValue);
		}
	}
	void AddField(const std::string& strFieldName, int64 llFieldValue,eKey k = eKey_update)
	{
		if (DataMem::MemOperate::DbOperate::BULK == m_OperType)
		{
			mapBulkFields[strFieldName].push_back(FieldValue(eType_int64,eKey_update,llFieldValue));
		}
		else
		{
			mapFields.insert(std::make_pair(strFieldName,FieldValue(eType_int64,k,llFieldValue)));
			if (oMemOperator) oMemOperator->AddRedisField(strFieldName,llFieldValue);
		}
	}
	void AddFieldArray(const std::string& strFieldName, const std::string& strFieldValue,eKey k = eKey_update)
	{
		if (DataMem::MemOperate::DbOperate::BULK == m_OperType)
		{
			mapBulkFields[strFieldName].push_back(FieldValue(eType_array_text,eKey_update,strFieldValue));
		}
		else
		{
			mapFields.insert(std::make_pair(strFieldName,FieldValue(eType_array_text,k,strFieldValue)));
		}
	}
	void AddFieldArray(const std::string& strFieldName, int64 llFieldValue,eKey k = eKey_update)
	{
		if (DataMem::MemOperate::DbOperate::BULK == m_OperType)
		{
			mapBulkFields[strFieldName].push_back(FieldValue(eType_array_int,eKey_update,llFieldValue));
		}
		else
		{
			mapFields.insert(std::make_pair(strFieldName,FieldValue(eType_array_int,k,llFieldValue)));
		}
	}
	void AddFieldHLL(const std::string& strFieldName, const std::string& strFieldValue,eKey k = eKey_update)
	{
		if (DataMem::MemOperate::DbOperate::CUSTOM == m_OperType)
		mapFields.insert(std::make_pair(strFieldName,FieldValue(eType_hll_text,k,strFieldValue)));
	}
	void AddFieldHLL(const std::string& strFieldName, int64 llFieldValue,eKey k = eKey_update)
	{
		if (DataMem::MemOperate::DbOperate::CUSTOM == m_OperType)
		mapFields.insert(std::make_pair(strFieldName,FieldValue(eType_hll_int,k,llFieldValue)));
	}
	DataMem::MemOperate* MakeMemOperate()
	{
		if (oMemOperator)
		{
			if (oMemOperator->GetDBFieldSize() == 0)
			{
				const std::string& str = SqlLine();
				oMemOperator->AddDbField(str);
				return oMemOperator->MakeMemOperate();
			}
			return oMemOperator->MakeMemOperate();
		}
		else
		{
			if (oDbOperator->GetDBFieldSize() == 0)
			{
				const std::string& str = SqlLine();
				oDbOperator->AddDbField(str);
				return oDbOperator->MakeMemOperate();
			}
			return oDbOperator->MakeMemOperate();
		}
	}
	const std::string& SqlLine()
	{
		std::string strFieldnames;//date,app_id
		std::string strFieldvalues;//'1',1,array[1],hll_add_agg(hll_hash_integer(1))
		std::string strFieldvaluesGroup;//('1',1,array[1]),('1',1,array[1])
		std::string strKeys;//app_id,gmid
		std::string strUpdates;
		m_uiLastRecordSize = 0;
		if (mapBulkFields.size())
		{
			std::vector<std::string> vec;//["'1',1,array[1],","'1',1,array[1],"]
			for(const auto& field:mapBulkFields)
			{
				strFieldnames += field.first + ",";
				if (vec.size() < field.second.size())vec.resize(field.second.size());
				int i(0);std::string tmp;
				for(const auto& vecField:field.second)
				{
					tmp.clear();
					if (vecField.type == eType_int64)tmp =vecField.str+",";
					else if (vecField.type == eType_string)tmp =std::string("'") + vecField.str + "',";
					else if (vecField.type == eType_array_int)tmp =std::string("array[") + vecField.str + "],";
					else if (vecField.type == eType_array_text)tmp =std::string("array['") + vecField.str + "'],";
					if (tmp.size())vec[i++].append(tmp.c_str(),tmp.size());
				}
			}

			for(const auto& field:vec)strFieldvaluesGroup += std::string("(") + field.substr(0,field.size()-1) + "),";
			m_uiLastRecordSize += vec.size();
		}
		else
		{
			m_uiLastRecordSize = 1;
			//vv= tb.vv+excluded.vv,
			//array_uids=array(SELECT DISTINCT unnest(array_append(tb.array_uids,%u))),
			//hll_uids=hll_add(tb.hll_uids,hll_hash_integer(%u))
			for(const auto &field:mapFields)
			{
				strFieldnames += field.first + ",";
				{
					if (field.second.type == eType_int64)strFieldvalues +=field.second.str+",";
					else if (field.second.type == eType_string)strFieldvalues +=std::string("'") + field.second.str + "',";
					else if (field.second.type == eType_array_int)strFieldvalues +=std::string("array[") + field.second.str + "],";
					else if (field.second.type == eType_array_text)strFieldvalues +=std::string("array['") + field.second.str + "'],";
					else if (field.second.type == eType_hll_int) strFieldvalues +=std::string("hll_add_agg(hll_hash_integer(") + field.second.str + ")),";
					else if (field.second.type == eType_hll_text) strFieldvalues +=std::string("hll_add_agg(hll_hash_text(") + field.second.str + ")),";
				}
				if (field.second.key == eKey_yes) strKeys += field.first + ",";
				else if (field.second.key == eKey_addup)
				{
					if (field.second.type == eType_int64)
					strUpdates += field.first+" = "+strTable+"."+field.first + " + excluded." + field.first + ",";
					else if (field.second.type == eType_array_int||field.second.type == eType_array_text)
					strUpdates += field.first + "=array(SELECT DISTINCT unnest(array_append("+ strTable + "." + field.first + "," + field.second.str +"))),";
					else if (field.second.type == eType_hll_int)
					strUpdates += field.first+"=hll_add("+ strTable+"."+field.first+",hll_hash_integer("+field.second.str+")),";
					else if (field.second.type == eType_hll_text)
					strUpdates += field.first+"=hll_add("+ strTable+"."+field.first+",hll_hash_text("+field.second.str+")),";
				}
				else if (field.second.key == eKey_update)
				{
					strUpdates += field.first + " = excluded." + field.first + ",";
				}
			}
		}

		if (strFieldnames.size())strFieldnames.erase(strFieldnames.size() -1);
		if (strFieldvalues.size())strFieldvalues.erase(strFieldvalues.size() -1);
		if (strFieldvaluesGroup.size())strFieldvaluesGroup.erase(strFieldvaluesGroup.size() -1);
		if (strKeys.size())strKeys.erase(strKeys.size() -1);
		if (strUpdates.size())strUpdates.erase(strUpdates.size() -1);

		if (strFieldvaluesGroup.size())
		{
			char sSql[4096];
			int n = snprintf(sSql,sizeof(sSql),"insert into %s(%s) values ",strTable.c_str(),strFieldnames.c_str());
			strSql.assign(sSql,n);
			strSql.append(strFieldvaluesGroup);
			return strSql;
		}
		else if (strKeys.size())
		{
			char sSql[4096];
			int n = snprintf(sSql,sizeof(sSql),"insert into %s(%s) select %s on conflict(%s) do update set %s",strTable.c_str(),strFieldnames.c_str(),strFieldvalues.c_str(),strKeys.c_str(),strUpdates.c_str());
			strSql.assign(sSql,n);
			return strSql;
		}
		else
		{
			char sSql[4096];
			int n = snprintf(sSql,sizeof(sSql),"insert into %s(%s) select %s",strTable.c_str(),strFieldnames.c_str(),strFieldvalues.c_str());
			strSql.assign(sSql,n);
			return strSql;
		}
	}
	void ClearFields()
	{
		mapFields.clear();
		mapBulkFields.clear();
		if (oDbOperator) oDbOperator->ClearDbFields();
		if (oMemOperator) oMemOperator->ClearFields();
	}
	unsigned int GetFieldSize()const {
		unsigned int counter = mapFields.size();
		for(const auto& fields:mapBulkFields)
		{
			counter += fields.second.size();
		}
		return counter;
	}
	const std::string strTable;
	std::unordered_map<std::string,FieldValue> mapFields;
	std::unordered_map<std::string,std::vector<FieldValue>> mapBulkFields;//bulk值支持pg批量插入一般类型数据(非hll)
	net::MemOperator *oMemOperator;
	net::DbOperator *oDbOperator;
	std::string strSql;
	DataMem::MemOperate::DbOperate::E_QUERY_TYPE m_OperType;
	uint32 m_uiLastRecordSize;
};

struct DbRecordReader//兼容ssdb和db返回值(用于ssdb和db指令返回行数不同的指令)
{
	const ::DataMem::Record& oRecord;
	int uiDbFieldSize;//db字段数量
	int uiCounter;
	DbRecordReader(const ::DataMem::Record& o,int uiDbSize):oRecord(o),uiDbFieldSize(uiDbSize),uiCounter(0){}
	void Read(int64 &value)
	{
		if (oRecord.field_info_size() == uiDbFieldSize && uiCounter < oRecord.field_info_size())
		{
			value = strtoll(oRecord.field_info(uiCounter++).col_value().c_str(),NULL,10);
		}
		else if (oRecord.field_info_size() == uiDbFieldSize * 2 && ++uiCounter < oRecord.field_info_size())
		{
			value = strtoll(oRecord.field_info(uiCounter++).col_value().c_str(),NULL,10);
		}
	}
	void Read(uint32 &value)
	{
		if (oRecord.field_info_size() == uiDbFieldSize && uiCounter < oRecord.field_info_size())
		{
			value = strtoull(oRecord.field_info(uiCounter++).col_value().c_str(),NULL,10);
		}
		else if (oRecord.field_info_size() == uiDbFieldSize * 2 && ++uiCounter < oRecord.field_info_size())
		{
			value = strtoull(oRecord.field_info(uiCounter++).col_value().c_str(),NULL,10);
		}
	}
	void Read(std::string &value)
	{
		if (oRecord.field_info_size() == uiDbFieldSize && uiCounter < oRecord.field_info_size())
		{
			value = oRecord.field_info(uiCounter++).col_value();
		}
		else if (oRecord.field_info_size() == uiDbFieldSize * 2 && ++uiCounter < oRecord.field_info_size())
		{
			value = oRecord.field_info(uiCounter++).col_value();
		}
	}
};

} /* namespace net */

#endif /* CLIENT_MEMOPERATOR_HPP_ */
