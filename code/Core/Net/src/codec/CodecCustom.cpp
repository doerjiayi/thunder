/*******************************************************************************
 * Project:  Nebula
 * @file     CodecCustom.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月11日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CodecCustom.hpp"

namespace net
{


CodecCustom::CodecCustom(util::E_CODEC_TYPE eCodecType, const std::string& strKey)
    : StarshipCodec(eCodecType, strKey)
{
}

CodecCustom::~CodecCustom()
{
}

E_CODEC_STATUS CodecCustom::Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, util::CBuffer* pBuff)
{
    LOG4_TRACE("pBuff->ReadableBytes()=%u, oMsgHead.ByteSize() = %d", pBuff->ReadableBytes(), oMsgHead.ByteSize());
    int iHadWriteLen = 0;
    int iWriteLen = 0;
    int iNeedWriteLen = sizeof(clientMsgHead);
    clientMsgHead cMsgHead;
    cMsgHead.body_len = 2; 
    cMsgHead.seq = oMsgHead.seq();

    cMsgHead.body_len = htons(cMsgHead.body_len);
    cMsgHead.seq = htonl(cMsgHead.seq);
    iWriteLen = pBuff->Write((char*)&cMsgHead, iNeedWriteLen);
    if (iWriteLen != iNeedWriteLen)
    {
        LOG4_ERROR("buff write head iWriteLen != iNeedWriteLen!");
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
        return(CODEC_STATUS_ERR);
    }
    iHadWriteLen += iWriteLen;

    std::string rsponeMsg("ok");
    iNeedWriteLen = rsponeMsg.size();
    iWriteLen = pBuff->Write(rsponeMsg.c_str(), rsponeMsg.size());
    if (iWriteLen == iNeedWriteLen)
    {
        return(CODEC_STATUS_OK);
    }
    else
    {
        LOG4_ERROR("buff write body iWriteLen != iNeedWriteLen!");
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
        return(CODEC_STATUS_ERR);
    }
}

E_CODEC_STATUS CodecCustom::Decode(util::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE("pBuff->ReadableBytes() = %u", pBuff->ReadableBytes());
    size_t uiHeadSize = sizeof(clientMsgHead);
    if (pBuff->ReadableBytes() >= uiHeadSize)
    {
        clientMsgHead stMsgHead;
        int iReadIdx = pBuff->GetReadIndex();
        pBuff->Read((char*)&stMsgHead, uiHeadSize);
        LOG4_TRACE("=======packet len = %u, packet seq = %u==========", stMsgHead.body_len, stMsgHead.seq);
        stMsgHead.body_len = ntohs(stMsgHead.body_len);
        stMsgHead.seq = ntohl(stMsgHead.seq);
        LOG4_TRACE("seq %u, len %u, pBuff->ReadableBytes() %u",
                        stMsgHead.seq, stMsgHead.body_len,
                        pBuff->ReadableBytes());
        oMsgHead.set_msgbody_len(stMsgHead.body_len);
        oMsgHead.set_seq(stMsgHead.seq);
        oMsgHead.set_cmd(100001);//这是自己设定的，用于测试回调业务
        if (0 == stMsgHead.body_len)      // 心跳包无包体
        {
            return(CODEC_STATUS_OK);
        }
        if (pBuff->ReadableBytes() >= stMsgHead.body_len)
        {
            std::string strRawData;
			strRawData.assign((const char*)pBuff->GetRawReadBuffer(), stMsgHead.body_len);
			oMsgBody.set_body(strRawData);
            LOG4_TRACE("pBuff->ReadableBytes()=%d, body_len()=%d, BodyContent=%s", pBuff->ReadableBytes(), stMsgHead.body_len, strRawData.c_str());
			pBuff->SkipBytes(stMsgHead.body_len);
			return(CODEC_STATUS_OK);
        }
        else
        {
            return(CODEC_STATUS_PAUSE);
        }
    }
    else
    {
        return(CODEC_STATUS_PAUSE);
    }
}

E_CODEC_STATUS CodecCustom::Decode(tagConnectionAttr* pConn,MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    E_CODEC_STATUS eCodecStatus = Decode(pConn->pRecvBuff, oMsgHead, oMsgBody);
    if (CODEC_STATUS_OK == eCodecStatus)//连接状态处理
    {
        if(eConnectStatus_ok != pConn->ucConnectStatus)// 连接尚未完成
        {
            LOG4_DEBUG("oInMsgHead.cmd(%u),ucConnectStatus(%u)",
                            oMsgHead.cmd(),pConn->ucConnectStatus);
            if (CMD_RSP_TELL_WORKER == oMsgHead.cmd())//连接完成（回应自己的节点的数据）
            {
                pConn->ucConnectStatus = eConnectStatus_ok;
            }
            else if (CMD_RSP_TELL_WORKER > oMsgHead.cmd())
            {
                pConn->ucConnectStatus = eConnectStatus_connecting;
            }
        }
    }
    return eCodecStatus;
}

} /* namespace neb */
