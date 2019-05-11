/*******************************************************************************
 * Project:  Util
 * @file     StreamCoder.hpp
 * @brief 
 * @author   bwarliao
 * @date:    2014年9月10日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef STREAMCODER_HPP_
#define STREAMCODER_HPP_

namespace util
{

enum E_CODEC_TYPE
{
    CODEC_UNKNOW            = 0,        ///< 未知
    CODEC_TLV               = 1,        ///< TLV编解码
    CODEC_PROTOBUF          = 2,        ///< Protobuf编解码
    CODEC_HTTP              = 3,        ///< HTTP编解码
    CODEC_PRIVATE           = 4,        ///< 私有协议编解码（与客户端通信协议）
    CODEC_WEBSOCKET_EX_JS   = 5,        ///< websocket extent Json（与客户端通信协议）
    CODEC_WEBSOCKET_EX_PB   = 6,        ///< websocket extent Protobuf（与客户端通信协议）
};

class CStreamCodec
{
public:
    CStreamCodec(E_CODEC_TYPE eCodecType)
        : m_eCodecType(eCodecType)
    {
    };
    virtual ~CStreamCodec(){};

    E_CODEC_TYPE GetCodecType()
    {
        return(m_eCodecType);
    }

private:
    E_CODEC_TYPE m_eCodecType;
};

}

#endif /* STREAMCODER_HPP_ */
