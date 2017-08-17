/*******************************************************************************
 * Project:  Thunder
 * @file     CodecWsExtent.hpp
 * @brief    与手机客户端通信协议编解码器
 * @author   cjy
 * @date:    2016年9月3日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODECWSEXTENTPB_HPP_
#define SRC_CODEC_CODECWSEXTENTPB_HPP_
#include <string>
#include <map>

#include "../CustomMsgHead.hpp"
#include "http/http_parser.h"
#include "protocol/http.pb.h"
#include "ThunderCodec.hpp"
#include "CodecCommon.hpp"

namespace thunder
{

//CodecWsExtent可以解析 http请求以及websocket请求
class CodecWebSocketPb: public ThunderCodec
{
public:
    CodecWebSocketPb(llib::E_CODEC_TYPE eCodecType, const std::string& strKey = "That's a lizard.");
    virtual ~CodecWebSocketPb();
    //解码编码websocket请求
    //Decode解码时判别是http请求还是websocket请求来处理,因为是框架固定调用该函数
    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, llib::CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(llib::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
    /**
     * @brief 连接的字节流解码
     * @return 编解码状态
     */
    virtual E_CODEC_STATUS Decode(tagConnectionAttr* pConn,MsgHead& oMsgHead, MsgBody& oMsgBody);
    //解码编码http请求
    virtual E_CODEC_STATUS Encode(const HttpMsg& oHttpMsg, llib::CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(llib::CBuffer* pBuff, HttpMsg& oHttpMsg);
private:
    E_CODEC_STATUS EncodeHandShake(const HttpMsg& oHttpMsg,
                    llib::CBuffer* pBuff);
    E_CODEC_STATUS EncodeHttp(const HttpMsg& oHttpMsg, llib::CBuffer* pBuff);
private:
    //http parse
    static int OnMessageBegin(http_parser *parser);
    static int OnUrl(http_parser *parser, const char *at, size_t len);
    static int OnStatus(http_parser *parser, const char *at, size_t len);
    static int OnHeaderField(http_parser *parser, const char *at, size_t len);
    static int OnHeaderValue(http_parser *parser, const char *at, size_t len);
    static int OnHeadersComplete(http_parser *parser);
    static int OnBody(http_parser *parser, const char *at, size_t len);
    static int OnMessageComplete(http_parser *parser);
    static int OnChunkHeader(http_parser *parser);
    static int OnChunkComplete(http_parser *parser);
    
	/*
     * oHttpMsg序列化为字符串
     * */
    const std::string& ToString(const HttpMsg& oHttpMsg);
    http_parser_settings m_parser_setting;
    http_parser m_parser;
    std::string m_strHttpString;
    std::map<std::string, std::string> m_mapAddingHttpHeader;       ///< encode前添加的http头，encode之后要清空
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECWSEXTENTPB_HPP_ */
