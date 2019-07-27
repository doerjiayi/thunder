/*
 * Similarity.hpp
 *
 *  Created on: 2017年5月17日
 *      Author: chen
 */
#include "Similarity.hpp"

namespace util
{

//最长公共子序列（Longest Common Subsequence），sphinx的评分标准之一
int LongestCommonSubsequence_length(const std::string &str1, const std::string &str2) {
    if (str1.size() == 0 || str2.size() == 0) return 0;
    std::vector<std::vector<int>> lcs_dp(str1.length() + 1, std::vector<int>(str2.length() + 1,0));//动规状态保存
    for (int i = 1; i <= str1.length(); i++) {
        for (int j = 1; j <= str2.length(); j++) {
            if (str1[i - 1] == str2[j - 1]) {//相同字符的则加1
            	lcs_dp[i][j] = lcs_dp[i - 1][j - 1] + 1;
            }
            else {//不同字符的为较大的左或上的格子的长度
            	lcs_dp[i][j] = std::max(lcs_dp[i - 1][j],lcs_dp[i][j - 1]);
            }
        }
    }
    return lcs_dp[str1.length()][str2.length()];
}

//最长公共子串（Longest Common Substring）
int LongestCommonSubstring_length(const std::string &str1, const std::string &str2) {
	if (str1.size() == 0 || str2.size() == 0) return 0;
	int length = 0;
	std::vector<std::vector<int>> lcs_dp(str1.length() + 1, std::vector<int>(str2.length() + 1,0));//动规状态保存
	for (int i = 1; i <= str1.length(); i++)
	{
		for (int j = 1; j <= str2.length(); j++)
		{
			if (str1[i - 1] == str2[j - 1])
			{
				lcs_dp[i][j] = lcs_dp[i - 1][j - 1] + 1;
				if (lcs_dp[i][j] > length)length = lcs_dp[i][j];
			}
			//else lcs_dp[i][j] = 0;可以看出，求最长子串和求最长子序列差别仅仅在这里
		}
	}
	return length;
}



//编辑距离概念描述：
//编辑距离，又称Levenshtein距离，是指两个字串之间，由一个转成另一个所需的最少编辑操作次数。许可的编辑操作包括将一个字符替换成另一个字符，插入一个字符，删除一个字符。
//例如将kitten一字转成sitting：
//sitten （k→s）
//sittin （e→i）
//sitting （→g）
//俄罗斯科学家Vladimir Levenshtein在1965年提出这个概念。
// 
//问题：找出字符串的编辑距离，即把一个字符串s1最少经过多少步操作变成编程字符串s2，操作有三种，添加一个字符，删除一个字符，修改一个字符
// 
//解析：
//首先定义这样一个函数——edit(i, j)，它表示第一个字符串的长度为i的子串到第二个字符串的长度为j的子串的编辑距离。
//显然可以有如下动态规划公式：
//if i == 0 且 j == 0，edit(i, j) = 0
//if i == 0 且 j > 0，edit(i, j) = j
//if i > 0 且j == 0，edit(i, j) = i
//if i ≥ 1  且 j ≥ 1 ，edit(i, j) == min{ edit(i-1, j) + 1, edit(i, j-1) + 1, edit(i-1, j-1) + f(i, j) }，当第一个字符串的第i个字符不等于第二个字符串的第j个字符时，f(i, j) = 1；否则，f(i, j) = 0。
int Levenshtein(const std::string& word1,const std::string& word2) {
	if (word1.size() == 0 && word2.size() == 0)return 0;
	int size1 = word1.size(), size2 = word2.size();
	std::vector<std::vector<int>> dp(size1 + 1, std::vector<int>(size2 + 1, 0));
	for (int i = 0; i <= size1; i++) dp[i][0] = i;//边际条件
	for (int j = 0; j <= size2; j++) dp[0][j] = j;
	for (int i = 1; i <= size1; i++) {
		for (int j = 1; j <= size2; j++) {
			int replace = word1[i - 1] == word2[j - 1] ? dp[i - 1][j - 1] : dp[i - 1][j - 1] + 1;//替换或相同
			int ins_del = std::min(dp[i][j - 1] + 1, dp[i - 1][j] + 1);//插入或者删除
			dp[i][j] = std::min(replace, ins_del);
		}
	}
	return dp[size1][size2];
}


std::vector<int> KmpGetNext(const std::string &str)
{
	std::vector<int> next({-1});//以-1开始
   for(int i=1;i<= str.size();i++)
   {
       int maxLen=0;
//       std::string tmp = str.substr(0,i);//子串计算
       for(int k = 1;k < i;++k)//计算前缀后缀的值
       {
           if(str.substr(0,k)==str.substr(i-k,k))//前缀后缀
           {
               maxLen=k;
           }
       }
       next.push_back(maxLen);
   }
   next.pop_back();
   return next;//ABCDABD    -1 0 0 0 0 1 2；
}

int KmpSearch(const std::string& s, const std::string& p)
{
    int sLen = s.size();
    int pLen = p.size();
    if (sLen == 0 || pLen == 0)return -1;
    std::vector<int> next = KmpGetNext(p);
    int i = 0;
    int j = 0;
    while (i < sLen && j < pLen)
    {
        ////j == -1,kmp匹配失败，跳过本字节，下字节开始继续匹配；
        //j >= 0，本字节开始匹配，模式串调到j开始
        if (j == -1 || s[i] == p[j])
        {
            i++;
            j++;
        }
        else
        {//从模式串的第几个字节失败则跳到指定位置继续尝试，如在第7字节失败，则跳到第三字节开始（下标为前面子串最大前后匹配长度）
            j = next[j];//ABCDABD    -1 0 0 0 0 1 2；
        }
    }
    if (j == pLen) return i - j;//返回第一个完全匹配位置

    return -1;
}

/*
输入:
s = "aa"
p = "a*"
输出: true
输入:
s = "aab"
p = "c*a*b"
输出: true
 * */
bool SimpleRegExMatchDfs(const std::string& s,const std::string& p,int i,int j)
{
   if(j >= p.size()) return i == s.size();
   bool j_match = i < s.size() && (s[i]==p[j] || p[j]=='.');
   if(j + 1 < p.size() && p[j + 1]=='*'){
	   return SimpleRegExMatchDfs(s,p,i,j+2)||(j_match && SimpleRegExMatchDfs(s,p,i+1,j));//匹配*，匹配0 个 或者  1个以上
   }
   return j_match && SimpleRegExMatchDfs(s,p,i+1,j+1);//匹配一个
}

bool SimpleRegExMatch(const std::string& str, const std::string& pattern)
{
	return SimpleRegExMatchDfs(str,pattern,0,0);
}

}
