/*
 * Levenshtein.hpp
 *
 *  Created on: 2017年5月17日
 *      Author: chen
 */

#ifndef CODE_ALGORITHM_SRC_Similarity_HPP_
#define CODE_ALGORITHM_SRC_Similarity_HPP_
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>

namespace util
{
//最长公共子序列长度，子序列不需要是连续的
int LongestCommonSubsequence_length(const std::string& str1,const std::string& str2);
//最长公共子串长度，子串需要是连续的
int LongestCommonSubstring_length(const std::string &str1, const std::string &str2);

//编辑距离，用于计算两字符串之间的最小编辑次数(添加一个字符，删除一个字符，修改一个字符)
int Levenshtein(const std::string& word1,const std::string& word2);

//kmp模式匹配算法
int KmpSearch(const std::string& str, const std::string& pattern);


/*
 * 简化正则任意匹配
给定一个字符串 (s) 和一个字符模式 (p)。实现支持 '.' 和 '*' 的正则表达式匹配。
'.' 匹配任意单个字符。
'*' 匹配零个或多个前面的元素。
匹配应该覆盖整个字符串 (s) ，而不是部分字符串。
 * */
bool SimpleRegExMatch(const std::string& str, const std::string& pattern);

}

#endif /* CODE_ALGORITHM_SRC_Similarity_HPP_ */
