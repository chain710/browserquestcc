#ifndef _BASE64_CODEC_H_
#define _BASE64_CODEC_H_

#ifndef _BASE64_NONUSE_STL_

#include <string>

std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);
#else

// encode输出字符串(会写\0), 注意输出的outlen不包含\0, 相当于是strlen(out)的结果
// outlen要求输入out buffer的长度
void base64_encode(unsigned char const* , unsigned int len, unsigned char *out, unsigned int& outlen);
// decode输出buffer, outlen输出解码后的长度
// outlen要求输入out buffer的长度
void base64_decode(unsigned char const* , unsigned int len, unsigned char *out, unsigned int& outlen);

#endif

#endif

