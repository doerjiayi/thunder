/*
 * LbsProxySession.h
 *
 *  Created on: 2016年2月19日
 *      Author: chenjiayi
 */
#ifndef CODE_LBSPROXYSERVER_SRC_LBS_PROXY_SESSION_H_
#define CODE_LBSPROXYSERVER_SRC_LBS_PROXY_SESSION_H_
#include <string>
#include <map>
#include "session/Session.hpp"

//需要常住内存，超时时间一年，使用后继续延长
#define LBS_PROXY_SESSION_TIME (3600 * 24 * 30 * 12)
#define LBS_PROXY_SESSIN_ID (30000)

namespace im
{

#define NEAR_DIS_PEAPLE (100000)
#define NEAR_DIS_GROUP (200000)


class LbsProxySession: public net::Session
{
public:
    LbsProxySession(): net::Session(LBS_PROXY_SESSIN_ID, LBS_PROXY_SESSION_TIME,
                                    std::string("im::LbsProxySession")), boInit(
                                    false), client(NULL), m_currenttime(0), near_dis_people(NEAR_DIS_PEAPLE),
                                    near_dis_group(NEAR_DIS_GROUP)
    {
    }
    virtual ~LbsProxySession()
    {
        if (boInit)
        {
            if (client)
            {
                mongoc_client_destroy(client);
            }
            mongoc_cleanup();
            boInit = false;
        }
    }
    static const std::string SessionClass()
    {
        return std::string("im::LbsProxySession");
    }
    bool Init(const util::CJsonObject& conf);
    /*
     * bson格式参数（简单操作可直接使用bson格式）
     * */
    //插入一个文档
    //param 1:数据库名
    //param 2:表名(即collection)
    //param 3:要插入的文档
    bool InsertDoc(const char*db, const char*tb, const bson_t *doc);
    //搜索文档
    //param 1:数据库名
    //param 2:表名
    //param 3:搜索条件，NULL为全部
    //param 4:返回结果的游标
    //param 5:跳过的条数，0则不跳
    //param 6:限制的返回的条数，0则不限制
    //param 7:批操作每次处理的数据的大小，0则不限制
    //param 8:指定返回 的字段，NULL则全部
    bool SearchDocs(const char*db, const char*tb, const bson_t *cond,
                    mongoc_cursor_t *&cursor, unsigned int skip = 0,
                    unsigned int limit = 0, unsigned int batch_size = 0,
                    const bson_t *fields = NULL);
    //更新一个文档
    //param 1:数据库名
    //param 2:表名(即collection)
    //param 3:更新条件
    //param 4:更新数据
    bool UpdateDoc(const char*db, const char*tb, const bson_t *cond,
                    const bson_t *updateddoc);
    //找到则更新，否则插入
    //param 1:数据库名
    //param 2:表名(即collection)
    //param 3:更新条件
    //param 4:更新数据（需要全量更新，需要所有的key）
    bool UpsertDoc(const char*db, const char*tb, const bson_t *cond,
                    const bson_t *updateddoc);
    //删除一个文档(只删除符合条件的第一个)
    //param 1:数据库名
    //param 2:表名(即集合collection)
    //param 3:删除条件
    bool DeleteDocOnce(const char*db, const char*tb, const bson_t *cond);
    //删除一个文档(删除所有符合条件的文档)
    //param 1:数据库名
    //param 2:表名(即集合collection)
    //param 3:删除条件
    bool DeleteDocAll(const char*db, const char*tb, const bson_t *cond);
    //获取符合条件的文档的个数
    //param 1:数据库名
    //param 2:表名(即集合collection)
    //param 3:查询条件
    //param 4:返回结果数量(查询失败则为-1，否则为大于等于0)
    bool Count(const char*db, const char*tb, const bson_t *cond,
                    long long &count);
    //执行命令
    //param 1:数据库名
    //param 2:表名(即集合collection)
    //param 3:命令
    bool Command(const char*db, const char*tb, const bson_t *command,
                    bson_t &reply);

    /*
     * json格式参数(复杂操作可使用json格式)
     * */
    //插入一个文档
    //param 1:数据库名
    //param 2:表名
    //param 3:要插入的文档
    //return 0则成功，其他则失败
    int InsertDoc(const char*db, const char*tb,
                    const util::CJsonObject &insertdObj);
    //搜索文档
    //param 1:数据库名
    //param 2:表名
    //param 3:搜索条件
    //param 4:返回结果数组
    //param 5:指定返回 的字段，NULL则全部
    //param 6:跳过的条数，0则不跳
    //param 7:限制的返回的条数，0则不限制
    //param 8:批操作每次处理的数据的大小，0则不限制
    //return 0则成功，其他则失败
    int SearchDocs(const char*db, const char*tb,
                    const util::CJsonObject &condObj,
                    std::vector<util::CJsonObject> &replyObjs,
                    const util::CJsonObject &fields, unsigned int skip = 0,
                    unsigned int limit = 0, unsigned int batch_size = 0);
    //更新一个文档
    //param 1:数据库名
    //param 2:表名(即集合collection)
    //param 3:更新条件
    //param 4:更新数据
    //return 0则成功，其他则失败
    int UpdateDoc(const char*db, const char*tb,
                    const util::CJsonObject &condObj,
                    const util::CJsonObject &updatedObj);
    //找到则更新，否则插入
    //param 1:数据库名
    //param 2:表名(即集合collection)
    //param 3:更新条件
    //param 4:更新数据（需要全量更新，需要所有的key）
    //return 0则成功，其他则失败
    int UpsertDoc(const char*db, const char* tb,
                    const util::CJsonObject &condObj,
                    const util::CJsonObject &updatedObj);

    //删除一个文档(只删除符合条件的第一个)
    //param 1:数据库名
    //param 2:表名(即集合collection)
    //param 3:删除条件
    //return 0则成功，其他则失败
    int DeleteDocOnce(const char*db, const char*tb,
                    const util::CJsonObject &cond);
    //删除一个文档(删除所有符合条件的文档)
    //param 1:数据库名
    //param 2:表名(即集合collection)
    //param 3:删除条件
    //return 0则成功，其他则失败
    int DeleteDocAll(const char*db, const char*tb,
                    const util::CJsonObject &cond);
    //获取符合条件的文档的个数
    //param 1:数据库名
    //param 2:表名(即集合collection)
    //param 3:查询条件
    //param 4:返回结果数量
    //return 0则成功，其他则失败
    int Count(const char*db, const char*tb, const util::CJsonObject &cond,
                    long long &count);
    //执行命令
    //param 1:数据库名
    //param 2:表名(即集合collection)
    //param 3:命令
    //param 4：返回结果
    //return 0则成功，其他则失败
    int Command(const char*db, const char*tb, const util::CJsonObject &command,
                    util::CJsonObject &reply);
    /*逻辑接口*/
    //更新用户坐标
    bool UpsertUserCoordinateInfo(unsigned int user_id,double longitude,double latitude);
    //更新群坐标
    bool UpsertGroupCoordinateInfo(unsigned int group_id,double longitude,double latitude);
    //获取用户地理信息
    bool GetUserCoordinateInfo(unsigned int user_id,util::CJsonObject &userObj);
    //获取群地理信息
    bool GetGroupCoordinateInfo(unsigned int group_id,util::CJsonObject &groupObj);
    //删除用户坐标
    bool DeleteUserCoordinate(unsigned int imid);
    //删除群坐标
    bool DeleteGroupCoordinate(unsigned int group_id);

    //会话超时延时
    net::E_CMD_STATUS Timeout()
    {
        return net::STATUS_CMD_RUNNING;
    }
    mongoc_client_t *GetClient() const
    {
        return client;
    }
    const bson_error_t& GetLastError() const
    {
        return error;
    }
    int GetNearDisPeople() const
    {
        return near_dis_people;
    }
    int GetNearDisGroup() const
    {
        return near_dis_group;
    }
    void SetCurrentTime(){m_currenttime = ::time(NULL);}
    net::int64 GetCurrentTime(){return m_currenttime;}
    void AppendDateTime(bson_t * updatedoc,const char* name,time_t datatime);
public:
    std::string group_dbname;    //db_coordinate_group
    std::string group_tbname;    //tb_coordinate_group
    std::string user_dbname;    //db_coordinate_user
    std::string user_tbname;    //tb_coordinate_user
private:
    bool boInit;
    char connectStr[128];
    mongoc_client_t *client;
    bson_error_t error;
    net::int64 m_currenttime; //当前时间
    int near_dis_people; //附近距离（单位米）,100000，目前100千米
    int near_dis_group; //附近距离（单位米）,200000，目前200千米
};

}
;

#endif /* CODE_LBSPROXYSERVER_SRC_LBS_PROXY_SESSION_H_ */
