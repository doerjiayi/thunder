/*******************************************************************************
 * Project:  Thunder
 * @file     CustomMsgCodec.cpp
 * @brief 
 * @author   cjy
 * @date:    2015年10月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include <netinet/in.h>

#include "../CustomMsgHead.hpp"
#include "CustomMsgCodec.hpp"

namespace thunder
{

CustomMsgCodec::CustomMsgCodec(thunder::E_CODEC_TYPE eCodecType, const std::string& strKey)
    : ThunderCodec(eCodecType, strKey)
{
}

CustomMsgCodec::~CustomMsgCodec()
{
}

E_CODEC_STATUS CustomMsgCodec::Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, thunder::CBuffer* pBuff)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    tagCustomMsgHead sttagCustomMsgHead;
    sttagCustomMsgHead.version = 1;        // version暂时无用
    sttagCustomMsgHead.encript = (unsigned char)(oMsgHead.cmd() >> 24);
    sttagCustomMsgHead.cmd = htons((unsigned short)(gc_uiCmdBit & oMsgHead.cmd()));
    sttagCustomMsgHead.body_len = htonl((unsigned int)oMsgHead.msgbody_len());
    sttagCustomMsgHead.seq = htonl(oMsgHead.seq());
    sttagCustomMsgHead.checksum = 0;//发送出去的消息不需要校验 htons((unsigned short)sttagCustomMsgHead.checksum);
    if (oMsgBody.ByteSize() > 64000000) // pb 最大限制
    {
        LOG4_ERROR("oMsgBody.ByteSize() > 64000000");
        return(CODEC_STATUS_ERR);
    }
    int iErrno = 0;
    int iHadWriteLen = 0;
    int iWriteLen = 0;
    int iNeedWriteLen = sizeof(sttagCustomMsgHead);
    LOG4_TRACE("cmd %u, seq %u, len %u", oMsgHead.cmd(), oMsgHead.seq(), oMsgHead.msgbody_len());
    if (oMsgHead.msgbody_len() == 0)    // 无包体（心跳包等）
    {
        iWriteLen = pBuff->Write(&sttagCustomMsgHead, iNeedWriteLen);
        LOG4_TRACE("sizeof(sttagCustomMsgHead) = %d, iWriteLen = %d", sizeof(sttagCustomMsgHead), iWriteLen);
        if (iWriteLen != iNeedWriteLen)
        {
            LOG4_ERROR("buff write head iWriteLen != sizeof(sttagCustomMsgHead)");
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
            return(CODEC_STATUS_ERR);
        }
//        pBuff->Compact(8192);
        return(CODEC_STATUS_OK);
    }
    iHadWriteLen += iWriteLen;
    if (sttagCustomMsgHead.encript == 0)       // 不压缩也不加密
    {
        iWriteLen = pBuff->Write(&sttagCustomMsgHead, iNeedWriteLen);
        LOG4_TRACE("sizeof(sttagCustomMsgHead) = %d, iWriteLen = %d", sizeof(sttagCustomMsgHead), iWriteLen);
        if (iWriteLen != iNeedWriteLen)
        {
            LOG4_ERROR("buff write head iWriteLen != sizeof(sttagCustomMsgHead)");
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
            return(CODEC_STATUS_ERR);
        }
        iNeedWriteLen = oMsgBody.ByteSize();
        iWriteLen = pBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
        if (iWriteLen != iNeedWriteLen)
        {
            LOG4_ERROR("buff write head iWriteLen != sizeof(sttagCustomMsgHead)");
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
            return(CODEC_STATUS_ERR);
        }
        iHadWriteLen += iWriteLen;
    }
    else
    {
        std::string strCompressData;
        std::string strEncryptData;
        if (gc_uiZipBit & oMsgHead.cmd())
        {
            if (!Zip(oMsgBody.SerializeAsString(), strCompressData))
            {
                LOG4_ERROR("zip error!");
                return(CODEC_STATUS_ERR);
            }
        }
        else if (gc_uiGzipBit & oMsgHead.cmd())
        {
            if (!Gzip(oMsgBody.SerializeAsString(), strCompressData))
            {
                LOG4_ERROR("gzip error!");
                return(CODEC_STATUS_ERR);
            }
        }
        if (gc_uiRc5Bit & oMsgHead.cmd())
        {
            if (strCompressData.size() > 0)
            {
                if (!Rc5Encrypt(strCompressData, strEncryptData))
                {
                    LOG4_ERROR("Rc5Encrypt error!");
                    return(CODEC_STATUS_ERR);
                }
            }
            else
            {
                if (!Rc5Encrypt(oMsgBody.SerializeAsString(), strEncryptData))
                {
                    LOG4_ERROR("Rc5Encrypt error!");
                    return(CODEC_STATUS_ERR);
                }
            }
        }

        if (strEncryptData.size() > 0)              // 加密后的数据包
        {
            sttagCustomMsgHead.body_len = htonl((unsigned int)strEncryptData.size());
            iWriteLen = pBuff->Write(&sttagCustomMsgHead, iNeedWriteLen);
            LOG4_TRACE("sizeof(sttagCustomMsgHead) = %d, iWriteLen = %d", sizeof(sttagCustomMsgHead), iWriteLen);
            if (iWriteLen != iNeedWriteLen)
            {
                LOG4_ERROR("buff write head iWriteLen != sizeof(sttagCustomMsgHead)");
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
            iNeedWriteLen = strEncryptData.size();
            iWriteLen = pBuff->Write(strEncryptData.c_str(), strEncryptData.size());
            if (iWriteLen != iNeedWriteLen)
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
        }
        else if (strCompressData.size() > 0)        // 压缩后的数据包
        {
            sttagCustomMsgHead.body_len = htonl((unsigned int)strCompressData.size());
            iWriteLen = pBuff->Write(&sttagCustomMsgHead, iNeedWriteLen);
            LOG4_TRACE("sizeof(sttagCustomMsgHead) = %d, iWriteLen = %d", sizeof(sttagCustomMsgHead), iWriteLen);
            if (iWriteLen != iNeedWriteLen)
            {
                LOG4_ERROR("buff write head iWriteLen != sizeof(sttagCustomMsgHead)");
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
            iNeedWriteLen = strCompressData.size();
            iWriteLen = pBuff->Write(strCompressData.c_str(), strCompressData.size());
            if (iWriteLen != iNeedWriteLen)
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
        }
        else    // 无效的压缩或加密算法，打包原数据
        {
            iWriteLen = pBuff->Write(&sttagCustomMsgHead, iNeedWriteLen);
            LOG4_TRACE("sizeof(sttagCustomMsgHead) = %d, iWriteLen = %d", sizeof(sttagCustomMsgHead), iWriteLen);
            if (iWriteLen != iNeedWriteLen)
            {
                LOG4_ERROR("buff write head iWriteLen != sizeof(sttagCustomMsgHead)");
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
            iNeedWriteLen = oMsgBody.ByteSize();
            iWriteLen = pBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
            if (iWriteLen != iNeedWriteLen)
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
        }
    }
    LOG4_TRACE("oMsgBody.ByteSize() = %d, iWriteLen = %d(compress or encrypt maybe)", oMsgBody.ByteSize(), iWriteLen);
//    pBuff->Compact(8192);
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CustomMsgCodec::Decode(thunder::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE("%s() pBuff->ReadableBytes() = %u", __FUNCTION__, pBuff->ReadableBytes());
    size_t uiHeadSize = sizeof(tagCustomMsgHead);
    if (pBuff->ReadableBytes() >= uiHeadSize)
    {
        tagCustomMsgHead sttagCustomMsgHead;
        int iReadIdx = pBuff->GetReadIndex();
        pBuff->Read(&sttagCustomMsgHead, uiHeadSize);
        sttagCustomMsgHead.cmd = ntohs(sttagCustomMsgHead.cmd);
        sttagCustomMsgHead.body_len = ntohl(sttagCustomMsgHead.body_len);
        sttagCustomMsgHead.seq = ntohl(sttagCustomMsgHead.seq);
        sttagCustomMsgHead.checksum = ntohs(sttagCustomMsgHead.checksum);
        LOG4_TRACE("cmd %u, seq %u, len %u, pBuff->ReadableBytes() %u",
                        sttagCustomMsgHead.cmd, sttagCustomMsgHead.seq, sttagCustomMsgHead.body_len,
                        pBuff->ReadableBytes());
        oMsgHead.set_cmd(((unsigned int)sttagCustomMsgHead.encript << 24) | sttagCustomMsgHead.cmd);
        oMsgHead.set_msgbody_len(sttagCustomMsgHead.body_len);
        oMsgHead.set_seq(sttagCustomMsgHead.seq);
        oMsgHead.set_checksum(sttagCustomMsgHead.checksum);
        if (0 == sttagCustomMsgHead.body_len)      // 心跳包无包体
        {
//            pBuff->Compact(8192);
            return(CODEC_STATUS_OK);
        }
        if (pBuff->ReadableBytes() >= sttagCustomMsgHead.body_len)
        {
            bool bResult = false;
            if (sttagCustomMsgHead.encript == 0)       // 未压缩也未加密
            {
                bResult = oMsgBody.ParseFromArray(pBuff->GetRawReadBuffer(), sttagCustomMsgHead.body_len);
            }
            else    // 有压缩或加密，先解密再解压，然后用MsgBody反序列化
            {
                std::string strUncompressData;
                std::string strDecryptData;
                if (gc_uiRc5Bit & oMsgHead.cmd())
                {
                    std::string strRawData;
                    strRawData.assign((const char*)pBuff->GetRawReadBuffer(), sttagCustomMsgHead.body_len);
                    if (!Rc5Decrypt(strRawData, strDecryptData))
                    {
                        LOG4_WARN("Rc5Decrypt error!");
                        return(CODEC_STATUS_ERR);
                    }
                }
                if (gc_uiZipBit & oMsgHead.cmd())
                {
                    if (strDecryptData.size() > 0)
                    {
                        if (!Unzip(strDecryptData, strUncompressData))
                        {
                            LOG4_WARN("uncompress error!");
                            return(CODEC_STATUS_ERR);
                        }
                    }
                    else
                    {
                        std::string strRawData;
                        strRawData.assign((const char*)pBuff->GetRawReadBuffer(), sttagCustomMsgHead.body_len);
                        if (!Unzip(strRawData, strUncompressData))
                        {
                            LOG4_WARN("uncompress error!");
                            return(CODEC_STATUS_ERR);
                        }
                    }
                }
                else if (gc_uiGzipBit & oMsgHead.cmd())
                {
                    if (strDecryptData.size() > 0)
                    {
                        if (!Gunzip(strDecryptData, strUncompressData))
                        {
                            LOG4_WARN("uncompress error!");
                            return(CODEC_STATUS_ERR);
                        }
                    }
                    else
                    {
                        std::string strRawData;
                        strRawData.assign((const char*)pBuff->GetRawReadBuffer(), sttagCustomMsgHead.body_len);
                        if (!Gunzip(strRawData, strUncompressData))
                        {
                            LOG4_WARN("uncompress error!");
                            return(CODEC_STATUS_ERR);
                        }
                    }
                }

                if (strUncompressData.size() > 0)       // 解压后的数据
                {
                    oMsgHead.set_msgbody_len(strUncompressData.size());
                    bResult = oMsgBody.ParseFromString(strUncompressData);
                }
                else if (strDecryptData.size() > 0)     // 解密后的数据
                {
                    oMsgHead.set_msgbody_len(strDecryptData.size());
                    bResult = oMsgBody.ParseFromString(strDecryptData);
                }
                else    // 无效的压缩或解密算法，仍然解析原数据
                {
                    bResult = oMsgBody.ParseFromArray(pBuff->GetRawReadBuffer(), sttagCustomMsgHead.body_len);
                }
            }
            if (bResult)
            {
                pBuff->SkipBytes(oMsgBody.ByteSize());
//                pBuff->Compact(8192);
                return(CODEC_STATUS_OK);
            }
            else
            {
                LOG4_WARN("cmd[%u], seq[%lu] oMsgBody.ParseFromArray() error!", oMsgHead.cmd(), oMsgHead.seq());
                return(CODEC_STATUS_ERR);
            }
        }
        else
        {
            pBuff->SetReadIndex(iReadIdx);
            return(CODEC_STATUS_PAUSE);
        }
    }
    else
    {
        return(CODEC_STATUS_PAUSE);
    }
}

E_CODEC_STATUS CustomMsgCodec::Decode(tagConnectionAttr* pConn,MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    E_CODEC_STATUS status = Decode(pConn->pRecvBuff,oMsgHead,oMsgBody);
    if (eConnectStatus_init == pConn->ucConnectStatus)//连接状态处理为解码一个消息成功，则连接状态完成
    {
        if (CODEC_STATUS_OK == status)
        {
            pConn->ucConnectStatus = eConnectStatus_ok;
        }
        else if (CODEC_STATUS_ERR == status)
        {
            LOG4_DEBUG("%s()　CustomMsgCodec　need to init connect status with private pb request",__FUNCTION__);
        }
    }
    return status;
}

} /* namespace thunder */
