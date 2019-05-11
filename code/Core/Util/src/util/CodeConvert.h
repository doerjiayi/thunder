#ifndef __CODE_CONVERT_H__
#define __CODE_CONVERT_H__
#include <cstdio>
#include <stdlib.h>  
#include <locale.h>  
#include <string>

namespace util
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

struct IgnoreChars
{
//	"ignore_chars_brief": "字符忽略列表，以逗号分隔，用ASCII码表示的十进制数,参考ASCII表:空字符(0),水平制表符(9),换行键(10),垂直制表符(11),回车键(13),空格(32),叹号(33),双引号(34),井号(35),闭单引号(39),星号(42),加号(43),分号(59),电子邮件符号(64),脱字符(94),波浪号(126)",
//	"ignore_chars": "0,9,10,11,13,32,33,34,35,39,42,43,59,64,94,126"
	std::vector<unsigned char> m_ignoreCharsVec;
	IgnoreChars()
	{
		LoadLetters("0,9,10,11,13,32,33,34,35,39,42,43,59,64,94,126");
	}
	void RemoveFlag(std::string &str,char flag = ' ')const
	{
		std::string::iterator it = std::remove(str.begin(), str.end(), flag);
		str.erase(it, str.end());
	}
	void LoadLetters(const std::string &ignore_chars)
	{
		if (ignore_chars.size() > 0)
		{
			int s = ignore_chars.length();
			char *tmpChars = new char[s + 1];
			snprintf(tmpChars,s + 1,ignore_chars.c_str());
			int j(0);
			int ascii(0);
			for(int i = 0;i <= s;)
			{
				if (',' == tmpChars[i])//逗号分隔
				{
					tmpChars[i] = 0;
					ascii = atoi(&tmpChars[j]);
					++i;
					j = i;
					m_ignoreCharsVec.push_back((unsigned char)ascii);
				}
				else if(0 == tmpChars[i])
				{
					ascii = atoi(&tmpChars[j]);
					m_ignoreCharsVec.push_back((unsigned char)ascii);
					break;
				}
				else
				{
					++i;
				}
			}
			delete [] tmpChars;
		}
	}
	void SkipNonsenseLetters(std::string& word)const
	{
		for(int i = m_ignoreCharsVec.size() -1;i > -1; --i)
		{
			RemoveFlag(word,m_ignoreCharsVec[i]);
		}
	}
};


}

#endif

