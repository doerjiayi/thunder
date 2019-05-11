/*
 * LbsUtil.h
 *
 *  Created on: 2016年2月23日
 *      Author: chenjiayi
 */
#ifndef CODE_LBSSERVER_SRC_LBSUTIL_H_
#define CODE_LBSSERVER_SRC_LBSUTIL_H_
#include <mongoc/src/libbson/src/bson/bson.h>
#include <mongoc/src/mongoc/mongoc.h>
#include "util/json/CJsonObject.hpp"
#include "NetDefine.hpp"
#include "NetError.hpp"

//mongoc 官方api参考 http://api.mongodb.org/c/current/
//libbson 官方api参考 http://api.mongodb.org/libbson/current/
//mongoc 聚合参考 http://www.runoob.com/mongodb/mongodb-aggregate.html
/*
 一、索引
 可以建立两种索引（2d和2dsphere），尽量使用2dsphere索引，效率会高点。尽量不使用geoNear来返回距离，而是使用nearSphere指令，返回结果后使用谷歌距离公式来计算。
 使用explain()函数可以检查命令使用的是什么索引：S2NearCursor使用的是2dsphere索引，GeoSearchCursor使用的是2d索引。如果2dsphere和2d索引同时存在，默认会使用
 2d索引。
 db.tb_coordinate_user.ensureIndex({'coordinate':'2dsphere'})
 db.tb_coordinate_user.ensureIndex({'coordinate':'2d'})
 db.tb_coordinate_group.ensureIndex({'coordinate':'2dsphere'})
 db.tb_coordinate_group.ensureIndex({'coordinate':'2d'})
 索引user_id 升序排序
 db.tb_coordinate_user.ensureIndex({'user_id':1})
  索引group_id 升序排序
 db.tb_coordinate_group.ensureIndex({'group_id':1})
 getIndexes函数可以查看索引
 > db.tb_coordinate_user.getIndexes()
[
        {
                "v" : 1,
                "key" : {
                        "_id" : 1
                },
                "name" : "_id_",
                "ns" : "db_coordinate_user.tb_coordinate_user"
        },
        {
                "v" : 1,
                "key" : {
                        "coordinate" : "2dsphere"
                },
                "name" : "coordinate_2dsphere",
                "ns" : "db_coordinate_user.tb_coordinate_user",
                "2dsphereIndexVersion" : 2
        }
]
$nearSphere 可以使用了2dsphere索引的查询（如果有2d索引会默认使用2d索引）
db.tb_coordinate_user.find({'coordinate':{$nearSphere: [113.944006, 22.543]}}).explain()
{
        "cursor" : "S2NearCursor",
        "isMultiKey" : false,
        "n" : 1,
        "nscannedObjects" : 32,
        "nscanned" : 32,
        "nscannedObjectsAllPlans" : 32,
        "nscannedAllPlans" : 32,
        "scanAndOrder" : false,
        "indexOnly" : false,
        "nYields" : 0,
        "nChunkSkips" : 0,
        "millis" : 3,
        "indexBounds" : {

        },
        "server" : "im.niiwoo.com:27088",
        "filterSet" : false
}
可以显式指定使用的索引。如下是指定使用索引coordinate_2dsphere
db.tb_coordinate_user.find({'coordinate':{$nearSphere: [113.944006, 22.543]}}).hint("coordinate_2dsphere").explain()
{
        "cursor" : "S2NearCursor",
        "isMultiKey" : false,
        "n" : 1,
        "nscannedObjects" : 32,
        "nscanned" : 32,
        "nscannedObjectsAllPlans" : 32,
        "nscannedAllPlans" : 32,
        "scanAndOrder" : false,
        "indexOnly" : false,
        "nYields" : 0,
        "nChunkSkips" : 0,
        "millis" : 2,
        "indexBounds" : {

        },
        "server" : "im.niiwoo.com:27088",
        "filterSet" : false
}

$near 使用了2d索引的查询（不建立2d索引则不可使用指令$near）
db.tb_coordinate_user.find({'coordinate':{$near: [113.944006, 22.543]}}).explain()
{
        "cursor" : "GeoSearchCursor",
        "isMultiKey" : false,
        "n" : 1,
        "nscannedObjects" : 1,
        "nscanned" : 1,
        "nscannedObjectsAllPlans" : 1,
        "nscannedAllPlans" : 1,
        "scanAndOrder" : false,
        "indexOnly" : false,
        "nYields" : 0,
        "nChunkSkips" : 0,
        "millis" : 0,
        "indexBounds" : {

        },
        "server" : "im.niiwoo.com:27088",
        "filterSet" : false
}
二、查询
 几何范围内查询
box = [[100, 20], [120, 30]]  
db.tb_coordinate_user.find({"coordinate" : {"$within" : {"$box" : box}}})  
返回结果
{ "_id" : ObjectId("56cc1556b01963def4c69a1b"), "coordinate" : [ 113.944006, 22.543043 ], "user_id" : 12345, "updatetime" : 1456215383 }
2.1查询附近点
将按离目标点(121.4905,31.2646)距离最近的20个点(距离倒序排列)，如果想指定返回的结果个数，可以使用limit()函数，若不指定，默认是返回100个点
查询方式如：
（1）这里$maxDistance单位公里
db.tb_coordinate_user.find({'coordinate':{$nearSphere:[113.944006, 22.543],$maxDistance:100}}).limit(20)
返回结果
{ "_id" : ObjectId("56cc1556b01963def4c69a1b"), "coordinate" : [ 113.944006, 22.543043 ], "user_id" : 12345, "updatetime" : 1456215383 }
（2）这里$maxDistance单位米
db.tb_coordinate_user.find({'coordinate':{$near: {$geometry: {type: "Point" ,coordinates: [113.944006, 22.543]},$maxDistance: 1000}}}).limit(20)
返回结果
{ "_id" : ObjectId("56cc1556b01963def4c69a1b"), "coordinate" : [ 113.944006, 22.543043 ], "user_id" : 12345, "updatetime" : 1456215383 }
（3）
db.tb_coordinate_user.find({'coordinate':{$geoNear: {$geometry: {type: "Point" ,coordinates: [113.944006, 22.543]},$maxDistance: 1000}}}).limit(20)
返回结果
{ "_id" : ObjectId("56cc1556b01963def4c69a1b"), "coordinate" : [ 113.944006, 22.543043 ], "user_id" : 12345, "updatetime" : 1456215383 }
2.2 查询附近点并获取距离信息
（1）geoNear指令，指定了spherical为true， dis的值为弧度
db.runCommand({ geoNear : "tb_coordinate_user", near : [113.944006, 22.543], spherical : true,maxDistance : 1/6371, distanceMultiplier: 6371})
 返回结果，dis单位为km
 {
        "results" : [
                {
                        "dis" : 0.0047813818463011285,
                        "obj" : {
                                "_id" : ObjectId("56cc1556b01963def4c69a1b"),
                                "coordinate" : [
                                        113.944006,
                                        22.543043
                                ],
                                "user_id" : 12345,
                                "updatetime" : 1456215383
                        }
                }
        ],
        "stats" : {
                "nscanned" : NumberLong(4),
                "objectsLoaded" : NumberLong(4),
                "avgDistance" : 0.0047813818463011285,
                "maxDistance" : 0.0047813818463011285,
                "time" : 0
        },
        "ok" : 1
}
查询2500米范围内的点，返回的结果中dis单位为米，返回结果最多10个
db.runCommand({ geoNear : "tb_coordinate_user" , near : [113.944006, 22.543], distanceMultiplier: 6378137, maxDistance:2500/6378137,spherical:true,num:10})
{
        "results" : [
                {
                        "dis" : 4.786738104696522,
                        "obj" : {
                                "_id" : ObjectId("56cc1556b01963def4c69a1b"),
                                "coordinate" : [
                                        113.944006,
                                        22.543043
                                ],
                                "user_id" : 12345,
                                "updatetime" : 1456215383
                        }
                }
        ],
        "stats" : {
                "nscanned" : NumberLong(4),
                "objectsLoaded" : NumberLong(4),
                "avgDistance" : 4.786738104696522,
                "maxDistance" : 4.786738104696522,
                "time" : 0
        },
        "ok" : 1
}
 * */
//地球半径，单位公里
#define EARTH_RADIUS  (6378.137)

#define PI (3.141592653589793)
//度转弧度
#define TORAD(d)  (d * PI / 180.0)
//获取两点的距离
//param1：点A的 latitude,
//param2：点A的 longitude,
//param3：点B的 Latitude,
//param4：点B的 Longitude
//返回距离，单位米
double GetDistance(double lat1, double lng1, double lat2, double lng2);

//获取查询附近命令(GeoJson格式命令)
//param1：点的 latitude,
//param2：点的 longitude,
//param3：距离,单位米
//param4：返回命令
void FindNearSphereGeoJsonCond(double longitude,double latitude,int dis,util::CJsonObject &mongocfindCond);
//获取查询附近命令
//param1：点的 latitude,
//param2：点的 longitude,
//param3：距离,单位米
//param4：返回命令
void FindNearSphereCond(double longitude,double latitude,int dis,util::CJsonObject &mongocfindCond);


#endif

