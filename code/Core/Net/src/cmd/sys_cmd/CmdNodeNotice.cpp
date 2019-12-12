/*******************************************************************************
 * Project:  Net
 * @file     CmdNodeNotice.cpp
 * @brief 
 * @author   cjy
 * @date:    2019年8月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include <cmd/sys_cmd/CmdNodeNotice.hpp>
#include "util/json/CJsonObject.hpp"
#include <step/sys_step/StepNodeNotice.hpp>
#include <iostream>
using namespace std;
namespace net
{

CmdNodeNotice::CmdNodeNotice()
{
}

CmdNodeNotice::~CmdNodeNotice()
{

}

bool CmdNodeNotice::AnyMessage(
                const tagMsgShell& stMsgShell,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    bool bResult = false;
    util::CBuffer oBuff;
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;

    util::CJsonObject jObj;
    int iRet = 0;
    if (GetCmd() == (int)oInMsgHead.cmd())
    {
        if (jObj.Parse(oInMsgBody.body()))
        {
            bResult = true;
            LOG4_DEBUG("CmdNodeNotice seq[%llu] jsonbuf[%s] Parse is ok",
                oInMsgHead.seq(),oInMsgBody.body().c_str());

            Step* pStep = new StepNodeNotice(stMsgShell, oInMsgHead, oInMsgBody);
            if (pStep == NULL)
            {
                LOG4_ERROR("error %d: new StepNodeNotice() error!", ERR_NEW);
                return(STATUS_CMD_FAULT);
            }
            if (RegisterCallback(pStep))
            {
                pStep->Emit(ERR_OK);
                return(STATUS_CMD_COMPLETED);
            }
            else
            {
                delete pStep;
            }

            //STEP不需要回调
            DeleteCallback(pStep);

            return(bResult);
        }
        else
        {
            iRet = 1;
            bResult = false;
            LOG4_ERROR("error jsonParse error! json[%s]", oInMsgBody.body().c_str());
        }
    }

    return(bResult);
}

} /* namespace net */
