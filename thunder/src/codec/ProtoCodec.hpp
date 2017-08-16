/*******************************************************************************
 * Project:  Thunder
 * @file     ProtoCodec.hpp
 * @brief 
 * @author   cjy
 * @date:    2017年10月6日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_PROTOCODEC_HPP_
#define SRC_CODEC_PROTOCODEC_HPP_

#include "ThunderCodec.hpp"

namespace thunder
{

class ProtoCodec: public ThunderCodec
{
public:
    ProtoCodec(thunder::E_CODEC_TYPE eCodecType, const std::string& strKey = "That's a lizard.");
    virtual ~ProtoCodec();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, thunder::CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(thunder::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
    /**
     * @brief 连接的字节流解码
     * @return 编解码状态
     */
    virtual E_CODEC_STATUS Decode(tagConnectionAttr* pConn,MsgHead& oMsgHead, MsgBody& oMsgBody);
};

} /* namespace thunder */

#endif /* SRC_CODEC_PROTOCODEC_HPP_ */
