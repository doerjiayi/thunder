/*******************************************************************************
 * Project:  Thunder
 * @file     CustomMsgCodec.hpp
 * @brief    与手机客户端通信协议编解码器
 * @author   cjy
 * @date:    2017年10月9日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CustomMsgCodec_HPP_
#define SRC_CODEC_CustomMsgCodec_HPP_

#include "ThunderCodec.hpp"

namespace thunder
{

class CustomMsgCodec: public ThunderCodec
{
public:
    CustomMsgCodec(llib::E_CODEC_TYPE eCodecType, const std::string& strKey = "That's a lizard.");
    virtual ~CustomMsgCodec();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, llib::CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(llib::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
    virtual E_CODEC_STATUS Decode(tagConnectionAttr* pConn,MsgHead& oMsgHead, MsgBody& oMsgBody);
};

} /* namespace thunder */

#endif /* SRC_CODEC_CustomMsgCodec_HPP_ */
