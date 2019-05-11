/*******************************************************************************
 * Project:  Hello
 * @file     ModuleHello.hpp
 * @brief 
 * @author   cjy
 * @date:    2016年4月19日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ModuleHello_ModuleHello_HPP_
#define SRC_ModuleHello_ModuleHello_HPP_
#include <map>

#include "ProtoError.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/map.h"
#include "google/protobuf/any.pb.h"
#include "test_proto3.pb.h"
#include "util/encrypt/base64.h"
#include "cmd/Module.hpp"
#include "cmd/Cmd.hpp"
#include "step/Step.hpp"
#include "step/HttpStep.hpp"
#include "../HelloSession.h"

namespace core
{

class ModuleHello: public net::Module
{
public:
    ModuleHello();
    virtual ~ModuleHello();
    virtual bool Init();
    void Tests();
    virtual bool AnyMessage(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
private:
    void Response(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,int iCode);
#define PG_TB_TEST "tb_test"
    void InsertPostgres(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType);
    void SetGetPostgres(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType);
    void GetPostgres(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType);
    void SetPostgres(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType);
    void AddUpPostgres(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,uint32 id,const std::string &sName,uint32 sum,const std::string &nodeType);
#define PROXY "PROXYSSDB"
#define TEST_SSDB_KEY "1:2:testkey"
    //redis basic
    void SetValueFromRedis(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType=PROXY);
    void OnlySetValueFromRedis(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType=PROXY);
    void OnlyGetValueFromRedis(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &nodeType=PROXY);

    //redisearch http://redisearch.io/Commands/
    void RedisearchAdd(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sDoc,const std::string &sValue);
    void RedisearchSearch(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue);

    //redis geo
    void RedisGEOADD(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue);
    void RedisGEORADIUSBYMEMBER(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue);

    //redis bitmap
#define  SETBIT_KEY "4:4:SETBIT"
    void RedisbitmapSETBIT(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &sKey=SETBIT_KEY,const std::string &sNode=PROXY);
    void RedisbitmapGETBIT(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &sKey=SETBIT_KEY,const std::string &sNode=PROXY);
    void RedisbitmapBITPOS(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &sKey=SETBIT_KEY,const std::string &sNode=PROXY);
    void RedisbitmapGET(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &sKey=SETBIT_KEY,const std::string &sNode=PROXY);
    void RedisbitmapGET_GET(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &sKey1=SETBIT_KEY,const std::string &sKey2=SETBIT_KEY,const std::string &sNode=PROXY);

    static void String2UserData(const std::string & col_value,std::vector<uint32>& usersData);
    static void OPUserData(const std::vector<uint32>& usersData1,const std::vector<uint32>& usersData2);

#define  MSG_KEY "1:11:MSG"
    void SsdbMsgHset(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sValue,const std::string &sKey=MSG_KEY,const std::string &sNode=PROXY);
    void SsdbMsgHsetall(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &sKey=MSG_KEY,const std::string &sNode=PROXY);
    void SsdbMsgHscan(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &key_start,const std::string &sKey=MSG_KEY,const std::string &sNode=PROXY);

    //db
    void TestDBSELECT(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg);

    //crytopp
    void TestRSA();

    //Coroutinue
    void TestCoroutinue();
    void TestCoroutinueAuto();
    void TestStepCoFuncDataProxy(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &str);

    //stage machine
    bool TestHttpRequestState(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
	bool TestHttpRequestStateFunc(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
	bool TestHttpRequestStateFuncDataProxy(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg,const std::string &str);

    //pb
    void Base64Encode(const char* data,unsigned int datalen,std::string &strEncode);
    void Base64Decode(const char* data,unsigned int datalen,std::string &strDecode);
    void PrintBin(const char* data,unsigned int datalen);
    bool TestJson2pb(const net::tagMsgShell& stMsgShell,const HttpMsg& oInHttpMsg);
    void TestProto3Type();
    void TestJson2pbRepeatedFields();

    bool boTests;
    net::RunClock m_RunClock;
};

} /* namespace core */

#endif /* SRC_ModuleHello_ModuleHello_HPP_ */
