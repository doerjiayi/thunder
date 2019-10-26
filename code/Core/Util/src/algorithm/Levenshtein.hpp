/*
 * Levenshtein.hpp
 *
 *  Created on: 2017年5月17日
 *      Author: chen
 */

#ifndef CODE_ALGORITHM_SRC_LEVENSHTEIN_HPP_
#define CODE_ALGORITHM_SRC_LEVENSHTEIN_HPP_
#include <string>
#include <algorithm>
#include <iostream>

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
namespace util
{

int Levenshtein(const std::string& str1,const std::string& str2);

}

#endif /* CODE_ROBOTSERVER_SRC_LEVENSHTEIN_HPP_ */
