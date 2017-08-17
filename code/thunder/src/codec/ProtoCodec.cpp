/*******************************************************************************
 * Project:  Thunder
 * @file     ProtoCodec.cpp
 * @brief 
 * @author   cjy
 * @date:    2017年10月6日
 * @note
 * Modify history:
 ******************************************************************************/
#include "ProtoCodec.hpp"

namespace thunder
{

ProtoCodec::ProtoCodec(llib::E_CODEC_TYPE eCodecType, const std::string& strKey)
    : ThunderCodec(eCodecType, strKey)
{
}

ProtoCodec::~ProtoCodec()
{
}

E_CODEC_STATUS ProtoCodec::Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, llib::CBuffer* pBuff)
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
    MsgHead oSwitchMsgHead;//在Encode中保证oSwitchMsgHead.ByteSize() == 15需要处理最高位
    oSwitchMsgHead.set_cmd(oMsgHead.cmd() | 0x80000000);
    oSwitchMsgHead.set_msgbody_len(oMsgHead.msgbody_len() | 0x80000000);
    oSwitchMsgHead.set_seq(oMsgHead.seq() | 0x80000000);
    //不需要checksum
    int iNeedWriteLen = oSwitchMsgHead.ByteSize();
    iWriteLen = pBuff->Write(oSwitchMsgHead.SerializeAsString().c_str(), oSwitchMsgHead.ByteSize());
    if (iWriteLen != iNeedWriteLen)
    {
        LOG4_ERROR("buff write head iWriteLen != iNeedWriteLen!");
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
        return(CODEC_STATUS_ERR);
    }
    LOG4_DEBUG("buff write head iWriteLen(%d),oSwitchMsgHead(%s,%u),oMsgHead(%s,%u)",
                    iWriteLen,oSwitchMsgHead.DebugString().c_str(),oSwitchMsgHead.ByteSize(),
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

E_CODEC_STATUS ProtoCodec::Decode(llib::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE("%s() pBuff->ReadableBytes()=%d, pBuff->GetReadIndex()=%d",
                    __FUNCTION__, pBuff->ReadableBytes(), pBuff->GetReadIndex());
    if ((int)(pBuff->ReadableBytes()) >= gc_uiMsgHeadSize)
    {
        MsgHead oSwitchMsgHead;
        bool bResult = oSwitchMsgHead.ParseFromArray(pBuff->GetRawReadBuffer(), gc_uiMsgHeadSize);
        if(bResult)
        {
            oMsgHead.Clear();
            //在Encode中保证oMsgHead.ByteSize() == 15需要处理最高位
            LOG4_DEBUG("%s() recv oSwitchMsgHead(%s,%u)",
                                        __FUNCTION__,oSwitchMsgHead.DebugString().c_str(),oSwitchMsgHead.ByteSize());
            oMsgHead.set_cmd(oSwitchMsgHead.cmd() & 0x7FFFFFFF);
            oMsgHead.set_msgbody_len(oSwitchMsgHead.msgbody_len() & 0x7FFFFFFF);
            oMsgHead.set_seq(oSwitchMsgHead.seq() & 0x7FFFFFFF);
            LOG4_DEBUG("%s() data oMsgHead(%s,%u)",
                            __FUNCTION__,oMsgHead.DebugString().c_str(),oMsgHead.ByteSize());
        }
        if (bResult)
        {
            if (0 == oMsgHead.msgbody_len())      // 无包体（心跳包等）
            {
                pBuff->SkipBytes(oSwitchMsgHead.ByteSize());
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

} /* namespace thunder */
