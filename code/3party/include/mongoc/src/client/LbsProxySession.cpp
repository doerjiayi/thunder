/*
 * LbsProxySession.cpp
 *
 *  Created on: 2015年11月6日
 *      Author: chen
 */
#include "LbsProxySession.h"

namespace im
{
bool LbsProxySession::Init(const util::CJsonObject& conf)
{
    if (boInit)
    {
        return true;
    }
    std::string dbip;
    int dbport;
    if (!conf.Get("dbip", dbip) || !conf.Get("dbport", dbport)
                    || !conf.Get("group_dbname", group_dbname)
                    || !conf.Get("group_tbname", group_tbname)
                    || !conf.Get("user_dbname", user_dbname)
                    || !conf.Get("user_tbname", user_tbname))
    {
        return false;
    }
    conf.Get("near_dis_people", near_dis_people);
    if(!near_dis_people)
    {
        near_dis_people = NEAR_DIS_PEAPLE;
    }
    conf.Get("near_dis_group", near_dis_group);
    if(!near_dis_group)
    {
        near_dis_group = NEAR_DIS_GROUP;
    }
    snprintf(connectStr, sizeof(connectStr), "mongodb://%s:%d/", dbip.c_str(),
                    dbport);
    mongoc_init();
    client = mongoc_client_new(connectStr); //"mongodb://localhost:27017/"
    if (!client)
    {
        return false;
    }
    boInit = true;
    return true;
}

/*
 mongoc api参考：http://api.mongodb.org/c/current/
 * */
/*
 (1)构建请求
 bson_oid_t oid;
 bson_t *doc;
 doc = bson_new ();
 bson_oid_init (&oid, NULL);
 BSON_APPEND_OID (doc, "_id", &oid);//文档id
 BSON_APPEND_UTF8 (doc, "hello", "world");//hello字段
 （2）插入操作
 InsertDoc("mydb", "mycoll",doc)
 （3）销毁请求
 bson_destroy (doc);
 * */
bool LbsProxySession::InsertDoc(const char*db, const char*tb, const bson_t *doc)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(client, db,
                    tb); //插入到数据库db的集合coll中
    if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL,
                    &error))
    {
        char *docStr = bson_as_json(doc, NULL);
        LOG4CPLUS_ERROR_FMT(GetLogger(),
                        "mongoc_collection_insert failed,cond:%s,error: %s\n",
                        docStr, error.message);
        bson_free(docStr);
        mongoc_collection_destroy(collection);
        return false;
    }
    mongoc_collection_destroy(collection);
    return true;
}
/*
 (1)查询条件
 bson_t *cond;
 cond = bson_new ();
 mongoc_cursor_t *cursor(NULL);
 (2)查询,返回游标cursor
 SearchDocs("mydb","mycoll",cond,cursor)
 (3)获取查询结果
 const bson_t *doc;
 char *str;
 while (mongoc_cursor_next (cursor, &doc)) {
     str = bson_as_json (doc, NULL);
     printf ("%s\n", str);
     bson_free (str);
 }
 (4) 销毁请求句柄
 bson_destroy (cond);
 mongoc_cursor_destroy (cursor);
 * */
bool LbsProxySession::SearchDocs(const char*db, const char*tb,
                const bson_t *cond, mongoc_cursor_t *&cursor, unsigned int skip,
                unsigned int limit, unsigned int batch_size,
                const bson_t *fields)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(client, db,
                    tb);
    cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, skip, limit,
                    batch_size, cond, fields, NULL); //获取查询对象查询后的结果
    if (!cursor)
    {
        char *condStr = bson_as_json(cond, NULL);
        LOG4CPLUS_ERROR_FMT(GetLogger(),
                        "SearchDocs mongoc_collection_find failed,cond:%s,error: %s\n",
                        condStr, error.message);
        bson_free(condStr);
        mongoc_collection_destroy(collection);
        return false;
    }
    mongoc_collection_destroy(collection);
    return true;
}
/*
 (1)更新条件和更新数据,如db.a.update({"_id" : ObjectId("55ef549236fe322f9490e17b")},{"$set":{"key":"new_value","updated":true}})
 bson_t *cond = BCON_NEW ("_id", BCON_OID (&oid));//条件为id "_id" : ObjectId("55ef549236fe322f9490e17b")
 bson_t *updatedoc = BCON_NEW ("$set", "{",
 "key", BCON_UTF8 ("new_value"),
 "updated", BCON_BOOL (true),
 "}");//{"$set"}
 (2)更新文档
 UpdateDoc("mydb","mycoll",cond,updatedoc)
 (3)销毁请求
 if (query)
 bson_destroy (query);
 if (update)
 bson_destroy (update);
 * */
bool LbsProxySession::UpdateDoc(const char*db, const char*coll,
                const bson_t *cond, const bson_t *updatedoc)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(client, db,
                    coll); //指定数据库和集合（即表）
    if (!collection) //mongoc_client_get_collection分配失败会执行abort,实际上可以不检查
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),
                        "UpdateDoc mongoc_client_get_collection failed:%s\n",
                        error.message);
        return false;
    }
    if (!mongoc_collection_update(collection, MONGOC_UPDATE_MULTI_UPDATE, cond,
                    updatedoc, NULL, &error)) //MONGOC_UPDATE_MULTI_UPDATE更新所有符合结果的
    {
        char *condStr = bson_as_json(cond, NULL);
        LOG4CPLUS_ERROR_FMT(GetLogger(),
                        "UpdateDoc mongoc_collection_update failed,cond:%s,error: %s\n",
                        condStr, error.message);
        bson_free(condStr);
        mongoc_collection_destroy(collection);
        return false;
    }
    mongoc_collection_destroy(collection);
    return true;
}
/*
 (1)更新条件和更新数据命令（没有则插入）
 bson_t *cond = BCON_NEW ("_id", BCON_OID (&oid));//条件为id "_id" : ObjectId("55ef549236fe322f9490e17b")
 bson_t *updatedoc = BCON_NEW ("$set", "{",
 "key", BCON_UTF8 ("new_value"),
 "updated", BCON_BOOL (true),
 "}");
 (2)更新文档
 UpsertDoc("mydb","mycoll",cond,updatedoc)
 (3)销毁请求
 if (cond)
 bson_destroy (cond);
 if (updatedoc)
 bson_destroy (updatedoc);
 * */
bool LbsProxySession::UpsertDoc(const char*db, const char*coll,
                const bson_t *cond, const bson_t *updatedoc)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(client, db,
                    coll); //指定数据库和集合（即表）
    if (!collection)
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),
                        "UpdateDoc mongoc_client_get_collection failed:%s\n",
                        error.message);
        return false;
    }
    if (!mongoc_collection_update(collection, MONGOC_UPDATE_UPSERT, cond,
                    updatedoc, NULL, &error)) //MONGOC_UPDATE_UPSERT 更新
    {
        char *condStr = bson_as_json(cond, NULL);
        LOG4CPLUS_ERROR_FMT(GetLogger(),
                        "UpsertDoc mongoc_collection_update failed,cond:%s,error: %s\n",
                        condStr, error.message);
        bson_free(condStr); //调用的是free
        mongoc_collection_destroy(collection);
        return false;
    }
    mongoc_collection_destroy(collection);
    return true;
}
/*
 (1)构建请求
 bson_t *cond = bson_new ();
 BSON_APPEND_OID (doc, "_id", &oid);
 (2)删除操作
 DeleteDoc("mydb","mycoll",cond)
 (3)销毁请求
 bson_destroy (cond);
 * */
bool LbsProxySession::DeleteDocOnce(const char*db, const char*coll,
                const bson_t *cond)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(client, db,
                    coll); //指定数据库和集合
    if (!collection)
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),
                        "DeleteDoc mongoc_client_get_collection failed");
        return false;
    }
    if (!mongoc_collection_remove(collection, MONGOC_REMOVE_SINGLE_REMOVE, cond,
    NULL, &error))
    {
        char *condStr = bson_as_json(cond, NULL);
        LOG4_ERROR("Delete failed,cond:%s,error: %s\n",
                        condStr, error.message);
        bson_free(condStr);
        mongoc_collection_destroy(collection);
        return false;
    }
    {
        char *condStr = bson_as_json(cond, NULL);
        LOG4_DEBUG("DeleteDocOnce ok,cond:%s",condStr);
        bson_free(condStr);
    }
    mongoc_collection_destroy(collection);
    return true;
}

bool LbsProxySession::DeleteDocAll(const char*db, const char*coll,
                const bson_t *cond)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(client, db,
                    coll); //指定数据库和集合
    if (!collection)
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),
                        "DeleteDoc mongoc_client_get_collection failed");
        return false;
    }
    if (!mongoc_collection_remove(collection, MONGOC_REMOVE_NONE, cond,
    NULL, &error))
    {
        char *condStr = bson_as_json(cond, NULL);
        LOG4_ERROR("Delete failed,cond:%s,error: %s\n",
                        condStr, error.message);
        bson_free(condStr);
        mongoc_collection_destroy(collection);
        return false;
    }
    mongoc_collection_destroy(collection);
    return true;
}

/*
 (1)构建请求
 bson_t *cond = bson_new_from_json ((const uint8_t *)"{\"hello\" : \"world\"}", -1, &error);//构造计数条件
 (2)执行计数请求
 long long count(0);
 Count("mydb", "mycoll",cond,count)
 * */
bool LbsProxySession::Count(const char*db, const char*coll, const bson_t *cond,
                long long &count)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(client, db,
                    coll); //指定数据库和集合
    if (!collection)
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),
                        "Count mongoc_client_get_collection failed");
        return false;
    }
    count = mongoc_collection_count(collection, MONGOC_QUERY_NONE, cond, 0, 0,
    NULL, &error);
    if (count < 0)
    {
        char *condStr = bson_as_json(cond, NULL);
        LOG4CPLUS_ERROR_FMT(GetLogger(),
                        "mongoc_collection_count failed,cond:%s,error: %s\n",
                        condStr, error.message);
        bson_free(condStr);
        mongoc_collection_destroy(collection);
        return false;
    }
    mongoc_collection_destroy(collection);
    return true;
}
/*
 (1)构建命令请求
 bson_t *command;
 command = BCON_NEW ("collStats", BCON_UTF8 ("mycoll"));
 (2)执行命令
 bson_t reply;//执行命令返回
 Command("mydb", "mycoll",command)
 (3)获取返回结果
 char *str = bson_as_json (&reply, NULL);//输出执行命令后的结果
 printf ("%s\n", str);
 bson_free (str);
 (4)销毁请求
 bson_destroy (command);
 bson_destroy (&reply);
 * */
bool LbsProxySession::Command(const char*db, const char*coll,
                const bson_t *command, bson_t &reply)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(client, db,
                    coll);
    if (!collection)
    {
        LOG4CPLUS_ERROR_FMT(GetLogger(),
                        "Count mongoc_client_get_collection failed");
        return false;
    }
    if (!mongoc_collection_command_simple(collection, command, NULL, &reply,
                    &error))
    {
        char *commandStr = bson_as_json(command, NULL);
        LOG4_ERROR("failed to run command:%s,error: %s\n",
                        commandStr, error.message);
        bson_free(commandStr);
        mongoc_collection_destroy(collection);
        return false;
    }
    mongoc_collection_destroy(collection);
    return true;
}

int LbsProxySession::InsertDoc(const char*db, const char*tb,
                const util::CJsonObject &insertdObj)
{
    int retCode(ERR_OK);
    LOG4_DEBUG("insertdObj json(%s)",
                    insertdObj.ToString().c_str());
    //插入的文档
    bson_t * insertddoc = GetBsonDoc(insertdObj, error);
    DebugOutPutBson(GetLogger(), insertddoc, "InsertDoc insertddoc");
    if (!insertddoc)
    {
        LOG4_WARN("insertdObj convert to bson failed(%s)",
                        insertdObj.ToString().c_str());
        retCode = ERR_REQ_MISS_PARAM;
        return retCode;
    }
    //有则更新，无则插入
    if (!InsertDoc(db, tb, insertddoc))
    {
        SAFE_DESTROY_BSON(insertddoc);
        retCode = ERR_OPERATOR_MONGO_ERROR;
        return retCode;
    }
    SAFE_DESTROY_BSON(insertddoc);
    return retCode;
}

int LbsProxySession::SearchDocs(const char*db, const char*tb,
                const util::CJsonObject &condObj,
                std::vector<util::CJsonObject> &replyObjs,
                const util::CJsonObject &fieldsObj, unsigned int skip,
                unsigned int limit, unsigned int batch_size)
{
    int retCode(ERR_OK);
    LOG4_DEBUG("condObj json(%s)",
                    condObj.ToString().c_str());
    //查询的条件
    bson_t * condDoc = GetBsonDoc(condObj, error);
    DebugOutPutBson(GetLogger(), condDoc, "SearchDocs condDoc");
    if (!condDoc)
    {
        LOG4_WARN("condObj convert to bson failed(%s)",
                        condObj.ToString().c_str());
        retCode = ERR_REQ_MISS_PARAM;
        return retCode;
    }
    //查询的fields
    bson_t * fieldsDoc(NULL);
    if (!fieldsObj.ToString().empty())
    {
        fieldsDoc = GetBsonDoc(fieldsObj, error);
        if (!fieldsDoc)
        {
            LOG4CPLUS_ERROR_FMT(GetLogger(),
                            "SearchDocs get fieldsDoc failed,error: %s\n",
                            error.message);
            retCode = ERR_REQ_MISS_PARAM;
            return retCode;
        }
    }
    DebugOutPutBson(GetLogger(), condDoc,"SearchDocs condDoc");
    if(fieldsDoc)
    {
        DebugOutPutBson(GetLogger(), fieldsDoc,"SearchDocs fieldsDoc");
    }
    else
    {
        LOG4_DEBUG("SearchDocs all fields");
    }
    mongoc_cursor_t *cursor(NULL);
    if (!SearchDocs(db, tb, condDoc, cursor, skip, limit, batch_size,
                    fieldsDoc))
    {
        LOG4_WARN("SearchDocs failed");
        SAFE_DESTROY_BSON(condDoc);
        SAFE_DESTROY_BSON(fieldsDoc);
        retCode = ERR_OPERATOR_MONGO_ERROR;
        return retCode;
    }
    replyObjs.clear();
    util::CJsonObject tmpReplyObj;
    //遍历结果
    const bson_t *doc;
    char *str;
    while (mongoc_cursor_next(cursor, &doc))
    {
        str = bson_as_json(doc, NULL);
        tmpReplyObj.Clear();
        tmpReplyObj.Parse(str);
        replyObjs.push_back(tmpReplyObj);
        LOG4_DEBUG("reply:%s\n", str);
        bson_free(str);
    }
    mongoc_cursor_destroy(cursor);
    SAFE_DESTROY_BSON(condDoc);
    SAFE_DESTROY_BSON(fieldsDoc);
    return retCode;
}

int LbsProxySession::UpdateDoc(const char*db, const char*tb,
                const util::CJsonObject &condObj,
                const util::CJsonObject &updatedObj)
{
    int retCode(ERR_OK);
    LOG4_DEBUG("condObj json(%s),updatedObj json(%s)",
                    condObj.ToString().c_str(), updatedObj.ToString().c_str());
    //更新的条件
    bson_t * conddoc = GetBsonDoc(condObj, error);
    DebugOutPutBson(GetLogger(), conddoc, "UpdateDoc conddoc");
    if (!conddoc)
    {
        LOG4_WARN("condObj convert to bson failed(%s)",
                        condObj.ToString().c_str());
        retCode = ERR_REQ_MISS_PARAM;
        return retCode;
    }
    //更新的文档
    bson_t * updatedoc = GetBsonDoc(updatedObj, error);
    DebugOutPutBson(GetLogger(), updatedoc);
    if (!updatedoc)
    {
        LOG4_WARN("updatedObj convert to bson failed(%s)",
                        updatedObj.ToString().c_str());
        retCode = ERR_REQ_MISS_PARAM;
        return retCode;
    }
    //有则更新，无则插入
    if (!UpdateDoc(db, tb, conddoc, updatedoc))
    {
        SAFE_DESTROY_BSON(conddoc);
        SAFE_DESTROY_BSON(updatedoc);
        retCode = ERR_OPERATOR_MONGO_ERROR;
        return retCode;
    }
    SAFE_DESTROY_BSON(conddoc);
    SAFE_DESTROY_BSON(updatedoc);
    return retCode;
}

int LbsProxySession::UpsertDoc(const char*db, const char* tb,
                const util::CJsonObject &condObj,
                const util::CJsonObject &updatedObj)
{
    int retCode(ERR_OK);
    LOG4_DEBUG("condObj json(%s),updatedObj json(%s)",
                    condObj.ToString().c_str(), updatedObj.ToString().c_str());
    //更新的条件
    bson_t * conddoc = GetBsonDoc(condObj, error);
    DebugOutPutBson(GetLogger(), conddoc, "UpsertDoc conddoc");
    if (!conddoc)
    {
        LOG4_WARN("condObj convert to bson failed(%s)",
                        condObj.ToString().c_str());
        retCode = ERR_REQ_MISS_PARAM;
        return retCode;
    }
    //更新的文档
    bson_t * updatedoc = GetBsonDoc(updatedObj, error);
    DebugOutPutBson(GetLogger(), updatedoc);
    if (!updatedoc)
    {
        LOG4_WARN("updatedObj convert to bson failed(%s)",
                        updatedObj.ToString().c_str());
        retCode = ERR_REQ_MISS_PARAM;
        return retCode;
    }
    //有则更新，无则插入
    if (!UpsertDoc(db, tb, conddoc, updatedoc))
    {
        SAFE_DESTROY_BSON(conddoc);
        SAFE_DESTROY_BSON(updatedoc);
        retCode = ERR_OPERATOR_MONGO_ERROR;
        return retCode;
    }
    SAFE_DESTROY_BSON(conddoc);
    SAFE_DESTROY_BSON(updatedoc);
    return retCode;
}

int LbsProxySession::DeleteDocOnce(const char*db, const char*tb,
                const util::CJsonObject &condObj)
{
    int retCode(ERR_OK);
    LOG4_DEBUG("condObj json(%s)",
                    condObj.ToString().c_str());
    //删除的条件
    bson_t * condDoc = GetBsonDoc(condObj, error);
    if (!condDoc)
    {
        LOG4_WARN("condObj convert to bson failed(%s)",
                        condObj.ToString().c_str());
        retCode = ERR_REQ_MISS_PARAM;
        return retCode;
    }
    DebugOutPutBson(GetLogger(), condDoc, "DeleteDocOnce conddoc");
    if (!DeleteDocOnce(db, tb, condDoc))
    {
        SAFE_DESTROY_BSON(condDoc);
        retCode = ERR_OPERATOR_MONGO_ERROR;
        return retCode;
    }
    SAFE_DESTROY_BSON(condDoc);
    return retCode;
}

int LbsProxySession::DeleteDocAll(const char*db, const char*tb,
                const util::CJsonObject &condObj)
{
    int retCode(ERR_OK);
    LOG4_DEBUG("condObj json(%s),updatedObj json(%s)",
                    condObj.ToString().c_str());
    //删除的条件
    bson_t * condDoc = GetBsonDoc(condObj, error);
    if (!condDoc)
    {
        LOG4_WARN("condObj convert to bson failed(%s)",
                        condObj.ToString().c_str());
        retCode = ERR_REQ_MISS_PARAM;
        return retCode;
    }
    DebugOutPutBson(GetLogger(), condDoc, "DeleteDocAll conddoc");
    if (!DeleteDocAll(db, tb, condDoc))
    {
        SAFE_DESTROY_BSON(condDoc);
        retCode = ERR_OPERATOR_MONGO_ERROR;
        return retCode;
    }
    SAFE_DESTROY_BSON(condDoc);
    return retCode;
}

int LbsProxySession::Count(const char*db, const char*tb,
                const util::CJsonObject &condObj, long long &count)
{
    int retCode(ERR_OK);
    LOG4_DEBUG("condObj json(%s)",
                    condObj.ToString().c_str());
    //计算的条件
    bson_t * condDoc = GetBsonDoc(condObj, error);
    if (!condDoc)
    {
        LOG4_WARN("condObj convert to bson failed(%s)",
                        condObj.ToString().c_str());
        retCode = ERR_REQ_MISS_PARAM;
        return retCode;
    }
    DebugOutPutBson(GetLogger(), condDoc);
    if (!Count(db, tb, condDoc, count))
    {
        SAFE_DESTROY_BSON(condDoc);
        retCode = ERR_OPERATOR_MONGO_ERROR;
        return retCode;
    }
    SAFE_DESTROY_BSON(condDoc);
    return retCode;
}

int LbsProxySession::Command(const char*db, const char*tb,
                const util::CJsonObject &commandObj,
                util::CJsonObject &replyObj)
{
    int retCode(ERR_OK);
    LOG4_DEBUG("commandObj json(%s),updatedObj json(%s)",
                    commandObj.ToString().c_str());
    //执行的命令
    bson_t * commandDoc = GetBsonDoc(commandObj, error);
    if (!commandDoc)
    {
        LOG4_WARN("commandObj convert to bson failed(%s)",
                        commandObj.ToString().c_str());
        retCode = ERR_REQ_MISS_PARAM;
        return retCode;
    }
    DebugOutPutBson(GetLogger(), commandDoc, "Command commandDoc");
    bson_t reply;
    if (!Command(db, tb, commandDoc, reply))
    {
        SAFE_DESTROY_BSON(commandDoc);
        retCode = ERR_OPERATOR_MONGO_ERROR;
        return retCode;
    }
    char *bStr = bson_as_json(&reply, NULL);
    replyObj.Clear();
    replyObj.Parse(bStr);
    bson_free(bStr); //调用的是free
    SAFE_DESTROY_BSON(commandDoc);
    return retCode;
}

bool LbsProxySession::GetUserCoordinateInfo(unsigned int user_id,util::CJsonObject &userObj)
{
    /*
      db.tb_coordinate_user.find({"user_id":12345})
                 返回：
      { "_id" : ObjectId("56cc1556b01963def4c69a1b"), "coordinate" : [ 113.944006, 22.543043 ], "user_id" : 12345, "updatetime" : 1456215383 }
     * */
    userObj.Clear();
    bson_t *condDoc = GetBsonDoc("user_id", user_id); //条件为user_id
    mongoc_cursor_t *cursor(NULL);
    if (!SearchDocs(user_dbname.c_str(), user_tbname.c_str(), condDoc, cursor))
    {
        LOG4_WARN("SearchDocs failed");
        SAFE_DESTROY_BSON(condDoc);
        return false;
    }
    //遍历结果
    const bson_t *doc;
    char *str;
    if (mongoc_cursor_next(cursor, &doc))
    {
        str = bson_as_json(doc, NULL);
        userObj.Parse(str);
        LOG4_DEBUG("reply:%s\n", str);
        bson_free(str);
        mongoc_cursor_destroy(cursor);
		SAFE_DESTROY_BSON(condDoc);
		return true;
    }
    else
    {
        mongoc_cursor_destroy(cursor);
        SAFE_DESTROY_BSON(condDoc);
        return false;
    }
}

bool LbsProxySession::GetGroupCoordinateInfo(unsigned int group_id,util::CJsonObject &groupObj)
{
    /*
     db.tb_coordinate_group.find({"group_id":1})
     * */
    groupObj.Clear();
    bson_t *condDoc = GetBsonDoc("group_id", group_id); //条件为group_id
    mongoc_cursor_t *cursor(NULL);
    if (!SearchDocs(group_dbname.c_str(), group_tbname.c_str(), condDoc, cursor))
    {
        LOG4_WARN("SearchDocs failed");
        SAFE_DESTROY_BSON(condDoc);
        return false;
    }
    //遍历结果
    const bson_t *doc;
    char *str;
    if (mongoc_cursor_next(cursor, &doc))
    {
        str = bson_as_json(doc, NULL);
        groupObj.Parse(str);
        LOG4_DEBUG("reply:%s\n", str);
        bson_free(str);
        mongoc_cursor_destroy(cursor);
		SAFE_DESTROY_BSON(condDoc);
		return true;
    }
    else
    {
        mongoc_cursor_destroy(cursor);
        SAFE_DESTROY_BSON(condDoc);
        return false;
    }
}

bool LbsProxySession::UpsertUserCoordinateInfo(unsigned int user_id,double longitude,double latitude)
{
    /*
     db.tb_coordinate_user.update({"user_id":1},{"user_id":1,"coordinate":[100,200],update_time:"2017"},true)
     { "_id" : { "$oid" : "54bec013a310012522417524" }, "user_id" : 10235, "coordinate" : [ 116.35843, 39.921399 ],
     "update_time" : { "$date" : "2015-02-26T14:44:30.398+0800" } }
     * */
    SetCurrentTime();
    bson_t *cond = GetBsonDocInt64("user_id", (long long)user_id); //条件为user_id
    util::CJsonObject requestjObj;
    requestjObj.AddEmptySubArray("coordinate");
    requestjObj["coordinate"].Add(longitude);
    requestjObj["coordinate"].Add(latitude);
    requestjObj.Add("user_id", (long long) user_id);
    LOG4_DEBUG("user coordinate json(%s)",
                                    requestjObj.ToString().c_str());
    bson_error_t error;
    bson_t * updatedoc = GetBsonDoc(requestjObj, error);
    if (!updatedoc)
    {
        LOG4_WARN(
                        "convert to bson failed(%s)",
                        requestjObj.ToString().c_str());
        return false;
    }
    AppendDateTime(updatedoc,"update_time",m_currenttime);//添加更新时间
    DebugOutPutBson(GetLogger(), updatedoc);
    //有则更新，无则插入
    if (!UpsertDoc(user_dbname.c_str(),
                    user_tbname.c_str(), cond, updatedoc))
    {
        SAFE_DESTROY_BSON(cond);
        SAFE_DESTROY_BSON(updatedoc);
        return false;
    }
    SAFE_DESTROY_BSON(cond);
    SAFE_DESTROY_BSON(updatedoc);
    return true;
}

bool LbsProxySession::UpsertGroupCoordinateInfo(unsigned int group_id,double longitude,double latitude)
{
    /*
     db.tb_coordinate_group.update({"group_id":1},{"group_id":1,"coordinate":[100,200],update_time:"2017"},true)
     { "_id" : { "$oid" : "54bf1036a310012522417554" }, "group_id" : 137, "coordinate" : [ 113.940885, 22.543072 ],
         "update_time" : { "$date" : "2015-01-21T10:34:30.905+0800" } }
     * */
    SetCurrentTime();
    bson_t *cond = GetBsonDoc("group_id", group_id); //条件为group_id
    util::CJsonObject requestjObj;
    requestjObj.AddEmptySubArray("coordinate");
    requestjObj["coordinate"].Add(longitude);
    requestjObj["coordinate"].Add(latitude);
    requestjObj.Add("group_id", (int) group_id);
    LOG4_DEBUG("group coordinate json(%s)",
                    requestjObj.ToString().c_str());
    bson_error_t error;
    bson_t * updatedoc = GetBsonDoc(requestjObj, error);
    if (!updatedoc)
    {
        LOG4_WARN(
                        "convert to bson failed(%s)",
                        requestjObj.ToString().c_str());
        return false;
    }
    AppendDateTime(updatedoc,"update_time",m_currenttime);//添加更新时间
    DebugOutPutBson(GetLogger(), updatedoc);
    //有则更新，无则插入
    if (!UpsertDoc(group_dbname.c_str(),
                    group_tbname.c_str(), cond, updatedoc))
    {
        SAFE_DESTROY_BSON(cond);
        SAFE_DESTROY_BSON(updatedoc);
        return false;
    }
    SAFE_DESTROY_BSON(cond);
    SAFE_DESTROY_BSON(updatedoc);
    return true;
}

bool LbsProxySession::DeleteGroupCoordinate(unsigned int group_id)
{
    bson_t *cond = GetBsonDoc("group_id", (int)group_id); //条件为group_id
    LOG4_DEBUG("DeleteGroupCoordinate group_id[%u]",group_id);
    //删除操作
    if(!DeleteDocOnce(group_dbname.c_str(),group_tbname.c_str(),cond))
    {
        SAFE_DESTROY_BSON(cond);
        LOG4_DEBUG("DeleteDoc() fault");
        return false;
    }
    SAFE_DESTROY_BSON(cond);
    return true;
}

bool LbsProxySession::DeleteUserCoordinate(unsigned int imid)
{
    LOG4_DEBUG("DeleteUserCoordinate user_id[%u]",imid);
    bson_t *cond = GetBsonDocInt64("user_id", (long long)imid); //条件为user_id
    //删除操作
    if(!DeleteDocOnce(user_dbname.c_str(),user_tbname.c_str(),cond))
    {
        LOG4_WARN(
                                "DeleteDocOnce failed,imid(%ll)",imid);
        return false;
    }
    SAFE_DESTROY_BSON(cond);
    return true;
}

void LbsProxySession::AppendDateTime(bson_t * updatedoc,const char* name,time_t datatime)
{
    int offset =8; //北京东8区
    int beijing = datatime + (3600*offset);
    bson_append_time_t(updatedoc,name,strlen(name),beijing);
}


}

;
//name space im
