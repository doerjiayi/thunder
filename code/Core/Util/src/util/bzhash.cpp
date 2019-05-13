#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <ctype.h>
#include <stdio.h>
#include "bzhash.hpp"

namespace util
{
// hash值
// 从字符串数组中查找匹配的字符串最合适的算法自然是使用哈希表。所谓Hash，一般是一个整数，通过某种算法，
// 可以把一个字符串"压缩" 成一个整数，这个数称为Hash，当然，无论如何，一个32位整数是无法对应回一个字符串的，
// 但如果有合适的校验方式，两个字符串计算出的Hash值相等的可能非常小。

// 假如说两个不同的字符串经过一个哈希算法得到的入口点一致有可能，但用三个不同的哈希算法算出的入口点都一致，
// 几率是1:18889465931478580854784，大概是10的 22.3次方分之一(使用3次单向散列函数运算)

// 哈希运算种子表是用来计算字符串对应的哈希整数的。
// 加密表
static unsigned long cryptTable[0x500];
// 哈希种子表是否初始化
static int cryptTableInited = 0;

void initCryptrTable()
{      
    unsigned long seed = 0x00100001, index1 = 0, index2 = 0, i;    
  
    for( index1 = 0; index1 < 0x100; index1++ )     
    {      
        for( index2 = index1, i = 0; i < 5; i++, index2 += 0x100 )     
        {      
            unsigned long temp1, temp2;     
            seed = (seed * 125 + 3) % 0x2AAAAB;     
            temp1 = (seed & 0xFFFF) << 0x10;     
            seed = (seed * 125 + 3) % 0x2AAAAB;     
            temp2 = (seed & 0xFFFF);     
            cryptTable[index2] = ( temp1 | temp2 );    //初始化加密表
        }      
    }      
}    

unsigned long bzhashstr(const char *str,int length,unsigned long seed)   //字符串，随机种子
{  
    unsigned char *key = (unsigned char *)str;     
    unsigned long seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE;//种子1,2
    int ch;     
  
    if (!cryptTableInited)  
    {  
        initCryptrTable();  
        cryptTableInited = 1;  
    }  
  
    while(length > 0)
    {  
        --length;
        ch = *key++;  
        ch = toupper(ch);   
  
        seed1 = cryptTable[(seed << 8) + ch] ^ (seed1 + seed2);//由加密表计算出的数
        seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3; //修改种子2
    }     
    return seed1;      
}   

unsigned long long HashStrToUint64(const char *str,unsigned int length)
{
    unsigned int hash1 = bzhashstr(str,length,0);
    unsigned int hash2 = bzhashstr(str,length,1);
    return ((unsigned long long)(unsigned int)(hash1) | ((unsigned long long)(unsigned int)(hash2) << 32));
}

void HashStrToString(const char *str,unsigned int length,char *dstStr,unsigned int dstLen)
{
    unsigned int hash1 = bzhashstr(str,length,0);
    unsigned int hash2 = bzhashstr(str,length,1);
    unsigned int hash3 = bzhashstr(str,length,2);
    snprintf(dstStr,dstLen,"%u%u%u",hash1,hash2,hash3);
}


}

