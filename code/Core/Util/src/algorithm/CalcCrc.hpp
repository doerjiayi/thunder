#ifndef CODE_ALGORITHM_SRC_CALCCRC_HPP_
#define CODE_ALGORITHM_SRC_CALCCRC_HPP_
#include <string>

namespace util
{
//冗余校验函数

inline unsigned char CalcCrc(const std::string &strData)
{
    const unsigned char* pb=(const unsigned char*)strData.c_str();//从消息体开始验证
    unsigned int uiSize= strData.length();//长度
    unsigned char ret = 0xBC;//特殊的key(简易冗余校验码只有一个字节)
    while (uiSize > 0)
    {
        ret ^= *pb;
        uiSize--;
        pb++;
    }
    return ret;
}

inline unsigned char CalcCrc(const char *data,unsigned int uiSize)
{
    const unsigned char* pb=(const unsigned char*)data;//从消息体开始验证
    unsigned char ret = 0xBC;//特殊的key(简易冗余校验码只有一个字节)
    while (uiSize > 0)
    {
        ret ^= *pb;
        uiSize--;
        pb++;
    }
    return ret;
}

}

#endif
