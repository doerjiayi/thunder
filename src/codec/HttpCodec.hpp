/*******************************************************************************
 * Project:  Thunder
 * @file     HttpCodec.hpp
 * @brief 
 * @author   cjy
 * @date:    2015年10月6日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_HTTPCODEC_HPP_
#define SRC_CODEC_HTTPCODEC_HPP_

#include "utility/http/http_parser.h"
#include "protocol/http.pb.h"
#include "ThunderCodec.hpp"

namespace thunder
{

class HttpCodec: public ThunderCodec
{
public:
    HttpCodec(thunder::E_CODEC_TYPE eCodecType, const std::string& strKey = "That's a lizard.");
    virtual ~HttpCodec();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, thunder::CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(thunder::CBuffer* pBuff,MsgHead& oMsgHead, MsgBody& oMsgBody);
    //连接缓存消息解码
    virtual E_CODEC_STATUS Decode(tagConnectionAttr* pConn,MsgHead& oMsgHead, MsgBody& oMsgBody);

    virtual E_CODEC_STATUS Encode(const HttpMsg& oHttpMsg, thunder::CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(thunder::CBuffer* pBuff, HttpMsg& oHttpMsg);

    /**
     * @brief 添加http头
     * @note 在encode前，允许框架根据连接属性添加http头
     */
    virtual void AddHttpHeader(const std::string& strHeaderName, const std::string& strHeaderValue);

    const std::string& ToString(const HttpMsg& oHttpMsg);
protected:
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

private:
    http_parser_settings m_parser_setting;
    http_parser m_parser;
    std::string m_strHttpString;
    std::map<std::string, std::string> m_mapAddingHttpHeader;       ///< encode前添加的http头，encode之后要清空
};

} /* namespace thunder */

#endif /* SRC_CODEC_HTTPCODEC_HPP_ */
