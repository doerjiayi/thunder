/*******************************************************************************
* Project:  DataAnalysis
* File:     LogBase.hpp
* Description: 写日志基类
* Author:        bwarliao
* Created date:  2010-12-23
* Modify history:
*******************************************************************************/

#ifndef LOGBASE_HPP_
#define LOGBASE_HPP_

#include "LogLevel.hpp"

namespace util
{

const unsigned int gc_uiMaxLogFileSize = 2048000;
const unsigned int gc_uiMaxRollLogFileIndex = 9;

class CLogBase
{
public:
    CLogBase(){};
    virtual ~CLogBase(){};
    virtual int WriteLog(int iLev = INFO, const char* chLogStr = "info", ...) = 0;
    virtual int SetLogLevel(int iLev)
    {
        return 0;
    }
};

}

#endif /* LOGBASE_HPP_ */
