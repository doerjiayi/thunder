/*******************************************************************************
 * Project:  Net
 * @file     ProtoCodec.cpp
 * @brief 
 * @author   cjy
 * @date:    2015年10月6日
 * @note
 * Modify history:
 ******************************************************************************/
#include "ProtoCodec.hpp"

namespace net
{

ProtoCodec::ProtoCodec(util::E_CODEC_TYPE eCodecType, const std::string& strKey)
    : StarshipCodec(eCodecType, strKey)
{
}

ProtoCodec::~ProtoCodec()
{
}

E_CODEC_STATUS ProtoCodec::Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, util::CBuffer* pBuff)
{
    LOG4_TRACE("%s() pBuff->ReadableBytes()=%u", __FUNCTION__, pBuff->ReadableBytes());
    if (oMsgBody.ByteSize() > 64000000) // pb 最大限制
    {
        LOG4_ERROR("oHttpMsg.ByteSize() > 64000000");
        return (CODEC_STATUS_ERR);
    }
    int iErrno = 0;
    int iHadWriteLen = 0;
    int iWriteLen = 0;
//    MsgHead oSwitchMsgHead;//在Encode中保证oSwitchMsgHead.ByteSize() == 15需要处理最高位
//    oSwitchMsgHead.set_cmd(oMsgHead.cmd() | 0x80000000);
//    oSwitchMsgHead.set_msgbody_len(oMsgHead.msgbody_len() | 0x80000000);
//    oSwitchMsgHead.set_seq(oMsgHead.seq() | 0x80000000);
    //不需要checksum
    int iNeedWriteLen = oMsgHead.ByteSize();
    iWriteLen = pBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
    if (iWriteLen != iNeedWriteLen)
    {
        LOG4_ERROR("buff write head iWriteLen != iNeedWriteLen!");
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
        return(CODEC_STATUS_ERR);
    }
    LOG4_DEBUG("buff write head iWriteLen(%d),oMsgHead(%s,%u),oMsgHead(%s,%u)",
                    iWriteLen,oMsgHead.DebugString().c_str(),oMsgHead.ByteSize(),
                    oMsgHead.DebugString().c_str(),oMsgHead.ByteSize());
    iHadWriteLen += iWriteLen;
    if (oMsgHead.msgbody_len() == 0)    // 无包体（心跳包等）
    {
//        pBuff->Compact(8192);
        return(CODEC_STATUS_OK);
    }
    iNeedWriteLen = oMsgBody.ByteSize();
    iWriteLen = pBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
    if (iWriteLen == iNeedWriteLen)
    {
        LOG4_DEBUG("buff write msgbody iWriteLen(%d)",iWriteLen);
        LOG4_TRACE("pBuff->ReadableBytes()=%u", pBuff->ReadableBytes());
//        pBuff->Compact(8192);
        return(CODEC_STATUS_OK);
    }
    else
    {
        LOG4_ERROR("buff write body iWriteLen != iNeedWriteLen!");
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
        return(CODEC_STATUS_ERR);
    }
}

E_CODEC_STATUS ProtoCodec::Decode(tagConnectionAttr* pConn,MsgHead& oMsgHead, MsgBody& oMsgBody)
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

E_CODEC_STATUS ProtoCodec::Decode(util::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE("%s() pBuff->ReadableBytes()=%d, pBuff->GetReadIndex()=%d",
                    __FUNCTION__, pBuff->ReadableBytes(), pBuff->GetReadIndex());
    if ((int)(pBuff->ReadableBytes()) >= gc_uiMsgHeadSize)
    {
    	oMsgHead.Clear();
        bool bResult = oMsgHead.ParseFromArray(pBuff->GetRawReadBuffer(), gc_uiMsgHeadSize);
        if (bResult)
        {
            if (0 == oMsgHead.msgbody_len())      // 无包体（心跳包等）
            {
                pBuff->SkipBytes(oMsgHead.ByteSize());
//                pBuff->Compact(8192);
                return(CODEC_STATUS_OK);
            }
            if (pBuff->ReadableBytes() >= gc_uiMsgHeadSize + oMsgHead.msgbody_len())
            {
                bResult = oMsgBody.ParseFromArray(
                                pBuff->GetRawReadBuffer() + gc_uiMsgHeadSize, oMsgHead.msgbody_len());
                if (bResult)
                {
                    pBuff->SkipBytes(gc_uiMsgHeadSize + oMsgBody.ByteSize());
//                    pBuff->Compact(8192);
                    return(CODEC_STATUS_OK);
                }
                else
                {
                    LOG4_ERROR("cmd[%u], seq[%lu] oMsgBody.ParseFromArray() error!", oMsgHead.cmd(), oMsgHead.seq());
                    return(CODEC_STATUS_ERR);
                }
            }
            else
            {
                return(CODEC_STATUS_PAUSE);
            }
        }
        else
        {
            oMsgHead.Clear();
            LOG4_ERROR("oMsgHead.ParseFromArray() error!");
            return(CODEC_STATUS_ERR);
        }
    }
    else
    {
        return(CODEC_STATUS_PAUSE);
    }
}

} /* namespace net */
