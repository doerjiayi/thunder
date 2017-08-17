/*
 * HttpUrl.h
 *
 *  Created on: 2017年12月23日
 *      Author: chen
 */

#ifndef CODE_INTERFACESERVER_SRC_HTTPURLCODER_H_
#define CODE_INTERFACESERVER_SRC_HTTPURLCODER_H_
#include <string>
#include <cstdio>
#include <vector>
#include <algorithm>
namespace llib
{
    char *url_encode(const char *s, int len, int *new_length);
    void url_encode(std::string &s);
    int url_decode(char *str, int len);
    void url_decode(std::string &str);
    void url_decode_original(std::string &str);
}
;

#endif /* CODE_INTERFACESERVER_SRC_HTTPURLCODER_H_ */
