/*******************************************************************************
 * Project:  Nebula
 * @file     CodecCustom.hpp
 * @brief    protobuf编解码器
 * @author   Bwar
 * @date:    2016年8月11日
 * @note     对应proto里msg.proto的MsgHead和MsgBody
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODECCUSTOM_HPP_
#define SRC_CODEC_CODECCUSTOM_HPP_

#include "StarshipCodec.hpp"

namespace net
{
#pragma pack(1)
struct clientMsgHead
{
    unsigned short body_len;                ///< 校验码（2字节）
    unsigned int seq;                       ///< 序列号（4字节）
    clientMsgHead() :body_len(0), seq(0)
    {
    }
};
#pragma pack(0)

class CodecCustom: public StarshipCodec
{
public:
	CodecCustom(util::E_CODEC_TYPE eCodecType, const std::string& strKey = "That's a lizard.");
    virtual ~CodecCustom();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, util::CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(util::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
    E_CODEC_STATUS Decode(tagConnectionAttr* pConn,MsgHead& oMsgHead, MsgBody& oMsgBody);
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECCUSTOM_HPP_ */
