/*******************************************************************************
 * Project:  Net
 * @file     StarshipCodec.hpp
 * @brief    Starship编解码器
 * @author   cjy
 * @date:    2015年10月6日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_STARSHIPCODEC_HPP_
#define SRC_CODEC_STARSHIPCODEC_HPP_

#include <arpa/inet.h>
#include <zlib.h>
#include <zconf.h>

#include "log4cplus/loggingmacros.h"
#include "util/StreamCodec.hpp"
#include "util/CBuffer.hpp"
#include "protocol/msg.pb.h"
#include "labor/Labor.hpp"
#include "labor/duty/Attribution.hpp"
#include "NetDefine.hpp"
#include "cmd/CW.hpp"

namespace net
{

const unsigned int gc_uiGzipBit = 0x10000000;          ///< 采用zip压缩
const unsigned int gc_uiZipBit  = 0x20000000;          ///< 采用gzip压缩
const unsigned int gc_uiRc5Bit  = 0x01000000;          ///< 采用12轮Rc5加密
const unsigned int gc_uiAesBit  = 0x02000000;          ///< 采用128位aes加密

/**
 * @brief 编解码状态
 * @note 编解码状态用于判断编解码是否成功，其中解码发生CODEC_STATUS_ERR情况时
 * 调用方需关闭对应连接；解码发生CODEC_STATUS_PAUSE时，解码函数应将缓冲区读位
 * 置重置回读开始前的位置。
 */
enum E_CODEC_STATUS
{
    CODEC_STATUS_OK         = 0,    ///< 编解码成功
    CODEC_STATUS_ERR        = 1,    ///< 编解码失败
    CODEC_STATUS_PAUSE      = 2,    ///< 编解码暂停（数据不完整，等待数据完整之后再解码）
};

class StarshipCodec: public util::CStreamCodec
{
public:
    StarshipCodec(util::E_CODEC_TYPE eCodecType, const std::string& strKey = "That's a lizard.");
    virtual ~StarshipCodec();
    /**
     * @brief 字节流编码
     * @param[in] oMsgHead  消息包头
     * @param[in] oMsgBody  消息包体
     * @param[out] pBuff  数据缓冲区
     * @return 编解码状态
     */
    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead,const MsgBody& oMsgBody, util::CBuffer* pBuff) = 0;

    /**
     * @brief 字节流解码
     * @param[in,out] pBuff 数据缓冲区
     * @param[out] oMsgHead 消息包头
     * @param[out] oMsgBody 消息包体
     * @return 编解码状态
     */
    virtual E_CODEC_STATUS Decode(util::CBuffer* pBuff,MsgHead& oMsgHead, MsgBody& oMsgBody) = 0;

    /**
     * @brief 连接的字节流解码,需要处理连接初始化状态
     * @param pConn 连接封装对象
     * @param[out] oMsgHead 消息包头
     * @param[out] oMsgBody 消息包体
     * @return 编解码状态
     */
    virtual E_CODEC_STATUS Decode(tagConnectionAttr* pConn,MsgHead& oMsgHead, MsgBody& oMsgBody) = 0;
protected:
    const std::string& GetKey() const
    {
        return(m_strKey);
    }

    bool Zip(const std::string& strSrc, std::string& strDest);
    bool Unzip(const std::string& strSrc, std::string& strDest);
    bool Gzip(const std::string& strSrc, std::string& strDest);
    bool Gunzip(const std::string& strSrc, std::string& strDest);
    bool Rc5Encrypt(const std::string& strSrc, std::string& strDest);
    bool Rc5Decrypt(const std::string& strSrc, std::string& strDest);
    bool AesEncrypt(const std::string& strSrc, std::string& strDest);
    bool AesDecrypt(const std::string& strSrc, std::string& strDest);
public:
private:
    std::string m_strKey;       // 密钥
//    util::Aes m_oAes;
};

} /* namespace net */

#endif /* SRC_CODEC_STARSHIPCODEC_HPP_ */
