/*
 * LbsUtil.cpp
 *
 *  Created on: 2016年2月23日
 *      Author: chenjiayi
 */
#include "../client/BsonUtil.h"
#include "LbsComm.h"
//public static double getDistance(double myLat, double myLng, double objLat, double objLng){
//    double radLat1 = myLat * RAD;
//    double radLat2 = objLat * RAD;
//    double a = radLat1 - radLat2;
//    double b = (myLng - objLng) * RAD;
//    double s = 2 * Math.asin(Math.sqrt(Math.pow(Math.sin(a / 2), 2) +
//            Math.cos(radLat1) * Math.cos(radLat2) * Math.pow(Math.sin(b / 2), 2)));
//    s = s * EARTH_RADIUS;
//    s = Math.round(s * 10000) / 10;// 10000 表示公里
//    return s;
//}

//param 1:坐标A纬度
//param 2:坐标A经度
//param 3:坐标B纬度
//param 4:坐标B经度
//返回距离，单位米
double GetDistance(double lat1, double lng1, double lat2, double lng2)
{
    double radLat1 = TORAD(lat1);
    double radLat2 = TORAD(lat2);
    double a = radLat1 - radLat2;
    double b = TORAD(lng1) - TORAD(lng2);
    double s = 2 * ::asin(sqrt(::pow(::sin(a/2),2) + ::cos(radLat1)*::cos(radLat2)*::pow(::sin(b/2),2)));
    s = s * EARTH_RADIUS;
    s = ::round(s * 10000) / 10;// 如果10换成10000 则表示公里
    return s;
}
/*
$nearSphere 指令，参数GeoJson $maxDistance距离单位默认为米。1000米查询如下：
db.tb_coordinate_group.find({"coordinate":{$nearSphere:{ $geometry:{type:"Point",coordinates:[114.99,23.543043]},$maxDistance:1000}}})
 * */
void FindNearSphereGeoJsonCond(double longitude,double latitude,int dis,util::CJsonObject &mongocfindCond)
{
    util::CJsonObject coordinate;
    {
        //$nearSphere
        util::CJsonObject nearSphere;
        {
            //$geometry
            util::CJsonObject geometry;
            geometry.Add("type","Point");
            geometry.AddEmptySubArray("coordinates");
            geometry["coordinates"].Add(longitude);
            geometry["coordinates"].Add(latitude);
            nearSphere.Add("$geometry",geometry);
            //$maxDistance
            nearSphere.Add("$maxDistance",dis);
        }
        coordinate.Add("$nearSphere",nearSphere);
    }

    //    <location field> 为coordinate
    mongocfindCond.Add("coordinate",coordinate);
}

/*
$nearSphere 指令，参数$maxDistance距离单位默认为弧度,500米查询如下
db.tb_coordinate_group.find( { "coordinate" : { $nearSphere: [ 114.99,23.543043 ] , $maxDistance: 0.5/6371}})
 * */
void FindNearSphereCond(double longitude,double latitude,int dis,util::CJsonObject &mongocfindCond)
{
    //$nearSphere
    util::CJsonObject coordinates;
    {
        coordinates.AddEmptySubArray("$nearSphere");
        coordinates["$nearSphere"].Add(longitude);
        coordinates["$nearSphere"].Add(latitude);
        double tmpDis = dis/1000;//换算为公里
        tmpDis = tmpDis/6371;//换算为弧度
        coordinates.Add("$maxDistance", tmpDis);
    }
    //    <location field> 为coordinate
    mongocfindCond.Add("coordinate", coordinates);
}

