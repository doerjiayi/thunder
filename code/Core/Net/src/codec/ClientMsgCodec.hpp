/*******************************************************************************
 * Project:  Net
 * @file     ClientMsgCodec.hpp
 * @brief    与手机客户端通信协议编解码器
 * @author   cjy
 * @date:    2019年10月9日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CLIENTMSGCODEC_HPP_
#define SRC_CODEC_CLIENTMSGCODEC_HPP_

#include "StarshipCodec.hpp"

namespace net
{

class ClientMsgCodec: public StarshipCodec
{
public:
    ClientMsgCodec(util::E_CODEC_TYPE eCodecType, const std::string& strKey = "client key");
    virtual ~ClientMsgCodec();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, util::CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(util::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
    virtual E_CODEC_STATUS Decode(tagConnectionAttr* pConn,MsgHead& oMsgHead, MsgBody& oMsgBody);
};

} /* namespace net */

#endif /* SRC_CODEC_CLIENTMSGCODEC_HPP_ */
