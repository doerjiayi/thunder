/*
 * BsonUtil.cpp
 *
 *  Created on: 2016年2月23日
 *      Author: chen
 */
#include "BsonUtil.h"

/*
bson_oid_t oid;
bson_t *cond = BCON_NEW ("_id", BCON_OID (&oid));//条件为id "_id" : ObjectId("55ef549236fe322f9490e17b")
return cond;
 * */
bson_t *GetBsonDoc(const char* field/*=_id*/,bson_oid_t &oid)
{
    bson_t *cond = BCON_NEW (field, BCON_OID (&oid));//条件为id "_id" : ObjectId("55ef549236fe322f9490e17b")
    return cond;
}

bson_t *GetBsonDoc(const char* field,int value)
{
    return  BCON_NEW(field, BCON_INT32(value));
}

bson_t *GetBsonDoc(const char* field,const char* value)
{
    return  BCON_NEW(field, BCON_UTF8(value));
}
bson_t *GetBsonDoc(const util::CJsonObject &jObj,bson_error_t &error)
{
    const std::string jObjStr = jObj.ToString();
    bson_t * doc = bson_new_from_json(
                    (const unsigned char*) jObjStr.c_str(),
                    (net::uint32) jObjStr.length(), &error);
    return doc;
}

bson_t *GetBsonDocInt64(const char* field,long long value)
{
    return  BCON_NEW(field, BCON_INT64(value));
}
bson_t *GetBsonDocDouble(const char* field,double value)
{
    return  BCON_NEW(field, BCON_DOUBLE(value));
}
bson_t *GetBsonDocBool(const char* field,bool value)
{
    return  BCON_NEW(field, BCON_BOOL(value));
}

/*
BSON_APPEND_INT32 (&b, "a", 1);
BSON_APPEND_UTF8 (&b, "hello", "world");
BSON_APPEND_BOOL (&b, "bool", true);
 * */
bson_t *BsonDocAppend(bson_t *b,const char* field,int value)
{
    BSON_APPEND_INT32 (b, field, value);
    return b;
}

bson_t *BsonDocAppend(bson_t *b,const char* field,bool value)
{
    BSON_APPEND_BOOL (b, field, value);
    return b;
}

bson_t *BsonDocAppend(bson_t *b,const char* field,const char* value)
{
    BSON_APPEND_UTF8 (b, field, value);
    return b;
}
/*
bson_t parent;
bson_t child;
char *str;

bson_init (&parent);
bson_append_document_begin (&parent, "$set", 3, &child);
bson_append_int32 (&child, "baz", 3, 1);
bson_append_document_end (&parent, &child);

str = bson_as_json (&parent, NULL);
printf ("%s\n", str);
bson_free (str);

bson_destroy (&parent);
 * */
bson_t *BsonDocAppend(bson_t *b,const char* field/*="$set"*/,bson_t *value)
{
    int l = strlen(field);
    bson_append_document_begin (b, field,l, value);
    bson_append_document_end (b, value);
    return b;
}

void DebugOutPutBson(const log4cplus::Logger &log,const bson_t *b,const char* msg)
{
    char *bStr = bson_as_json(b, NULL);
    if(msg)
    {
        LOG4CPLUS_DEBUG_FMT(log,
                            "%s,bson:%s",msg,bStr);
    }
    else
    {
        LOG4CPLUS_DEBUG_FMT(log,"bson:%s",bStr);
    }
    bson_free(bStr);//调用的是free
}


