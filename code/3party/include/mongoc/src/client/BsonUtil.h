/*
 * BsonUtil.h
 *
 *  Created on: 2016年2月23日
 *      Author: chen
 */

#ifndef CODE_SRC_BSONUTIL_H_
#define CODE_SRC_BSONUTIL_H_
#include <mongoc/src/libbson/src/bson/bson.h>
#include <mongoc/src/mongoc/mongoc.h>
#include "util/json/CJsonObject.hpp"
//#include "NetDefine.hpp"
//#include "NetError.hpp"

//创建单key文档
bson_t *GetBsonDoc(const char* field,bson_oid_t &oid);
bson_t *GetBsonDoc(const char* field,int value);
bson_t *GetBsonDoc(const char* field,const char* value);
bson_t *GetBsonDocInt64(const char* field,long long value);
bson_t *GetBsonDocDouble(const char* field,double value);
bson_t *GetBsonDocBool(const char* field,bool value);
//由json获取复杂Bson对象
bson_t *GetBsonDoc(const util::CJsonObject &jObj,bson_error_t &error);

//添加新key到Doc
bson_t *BsonDocAppend(bson_t *b,const char* field,int value);
bson_t *BsonDocAppend(bson_t *b,const char* field,bool value);
bson_t *BsonDocAppend(bson_t *b,const char* field,const char* value);
bson_t *BsonDocAppend(bson_t *b,const char* field/*="$set"*/,bson_t *value);

void DebugOutPutBson(const log4cplus::Logger &log,const bson_t *b,const char* msg=NULL);

#define SAFE_DESTROY_BSON(doc) if (doc) {bson_destroy (doc);doc = NULL;}



#endif /* CODE_LBSSERVER_SRC_BSONUTIL_H_ */
