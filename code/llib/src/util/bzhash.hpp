#ifndef _bzHash_HPP_
#define _bzHash_HPP_

namespace llib
{
// hash值
// 从字符串数组中查找匹配的字符串最合适的算法自然是使用哈希表。所谓Hash，一般是一个整数，通过某种算法，
// 可以把一个字符串"压缩" 成一个整数，这个数称为Hash，当然，无论如何，一个32位整数是无法对应回一个字符串的，
// 但如果有合适的校验方式，两个字符串计算出的Hash值相等的可能非常小。

// 假如说两个不同的字符串经过一个哈希算法得到的入口点一致有可能，但用三个不同的哈希算法算出的入口点都一致，
// 几率是1:18889465931478580854784，大概是10的 22.3次方分之一(使用3次单向散列函数运算)

void initCryptrTable();

//字符串，随机种子
unsigned long bzhashstr(const char *str, unsigned long seed); 


//字符串哈希为64位长整形
unsigned long long HashStrToUint64(const char *str, unsigned int length);
 

}

#endif
