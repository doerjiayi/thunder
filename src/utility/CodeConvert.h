#ifndef __CODE_CONVERT_H__
#define __CODE_CONVERT_H__
#include <cstdio>
#include <stdlib.h>  
#include <locale.h>  
#include <string>

namespace thunder
{

// FUNCTION: gbk2utf8 
// DESCRIPTION: 实现由gbk编码到utf8编码的转换
//  param: strDst 转换后的字符串;
//  param maxStrDstlen utfStr的最大长度
//  param strSrc 待转换的字符串;
// Output: utfStr 
//  Returns: <= 0,fail ; > 0 ,success
inline int gbk2utf8(char *strDst,size_t maxStrDstlen,const char *strSrc)
{  
	if(!strSrc||!strDst)
	{  
		return -1;//Bad Parameter
	} 
	//首先先将gbk编码转换为unicode编码
	if(NULL==setlocale(LC_ALL,"zh_CN.gbk"))//设置转换为unicode前的码,当前为gbk编码
	{  
		return -1;//参数有错误
	}  
	int unicodeLen=mbstowcs(NULL,strSrc,0);//计算转换后的长度
	if(unicodeLen<=0)  
	{  
		return -1;//不能转换
	} 
	wchar_t *strUnicode = (wchar_t *)calloc(sizeof(wchar_t),unicodeLen+1);
	mbstowcs(strUnicode,strSrc,strlen(strSrc));//将gbk转换为unicode
	//将unicode编码转换为utf8编码
	if(NULL==setlocale(LC_ALL,"zh_CN.utf8"))//设置unicode转换后的码,当前为utf8
	{  
		free(strUnicode);
		return -1;//参数有错误
	}  
	int nUnicodeLen = wcstombs(NULL,strUnicode,0);//计算转换后的长度
	if(nUnicodeLen <= 0)
	{  
		free(strUnicode);
		return -1; //不能转换
	}  
	else if(nUnicodeLen >= (int)maxStrDstlen)//判断空间是否足够
	{  
		free(strUnicode);
		return -1;  //Dst Str memory not enough
	}  
	wcstombs(strDst,strUnicode,nUnicodeLen);
	strDst[nUnicodeLen] = 0;//添加结束符
	free(strUnicode);
	return nUnicodeLen;
}


inline int gbk2utf8(std::string& strDst,const char *strSrc)
{
	if (!strSrc)
	{
		return 0;
	}
	int maxDstLen = (int)strlen(strSrc) * 2 + 1;
	char *tmpStrDst = new char[maxDstLen];
	gbk2utf8(tmpStrDst,maxDstLen - 1,strSrc);
	strDst.assign(tmpStrDst);//赋值目标字符串
	delete [] tmpStrDst;
	return strDst.size();
}

}

#endif

