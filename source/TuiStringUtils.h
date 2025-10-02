

#ifndef TuiStringUtils_h
#define TuiStringUtils_h

#include <stdarg.h>  // for va_start, etc
#include <memory>    // for std::unique_ptr
#include <algorithm>
#include <string>
#include <cstdio>
#include <sstream>
#include <vector>
#include <iostream>
#include <ctype.h>
#include <iomanip>
#include <limits>
#include "TuiSha1.h"

#include <stdexcept>


namespace Tui {

inline std::string string_format(const std::string fmt, ...) {
    int size = ((int)fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
    std::string str;
    va_list ap;
    while (1) {     // Maximum two passes on a POSIX system...
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
        va_end(ap);
        if (n > -1 && n < size) {  // Everything worked
            str.resize(n);
            return str;
        }
        if (n > -1)  // Needed size returned
            size = n + 1;   // For null char
        else
            size *= 2;      // Guess at a larger size (OS specific)
    }
    return str;
}

//on mac: Cannot pass object of non-trivial type 'std::string' through variadic function; call will abort at runtime


//void printBits(size_t const size, void const * const ptr);

#ifdef _MSC_VER
#define _WINSOCKAPI_    // stops windows.h including winsock.h
#define NOMINMAX
#include <windows.h>

inline std::wstring convertUtf8ToWide(const std::string& str)
{
    int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wstr[0], count);
    return wstr;
}

inline std::string convertWideToUtf8(const std::wstring& wstr)
{
    int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), NULL, 0, NULL, NULL);
    std::string str(count, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
    return str;
}
#else

inline std::string convertUtf8ToWide(const std::string& str)
{
    return str;
}

inline std::string convertWideToUtf8(const std::string& wstr)
{
    return wstr;
}
#endif


//#define Tui::string_format(__fmt__, ...) fmt::sprintf(__fmt__, ##__VA_ARGS__)

inline std::string stringByReplacingString(const std::string& input, const std::string& search, const std::string& replace)
{
    std::string result = input;
    size_t pos = result.find(search);
    
    while(pos != std::string::npos)
    {
        result.replace(pos, search.size(), replace);
        pos = result.find(search, pos + search.size());
    }
    
    return result;
}

inline std::vector<std::string> splitString(const std::string& input, char delim)
{
    std::vector<std::string> result;
    
    std::string foundString;
    std::istringstream inputStringStream(input);
    
    while (std::getline(inputStringStream, foundString, delim)) {
        result.push_back(foundString);
    }
    
    return result;
}

#include <string>

static const unsigned char base64_table[65] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or empty string on failure
 */
inline std::string base64_encode(const unsigned char *src, size_t len)
{
    unsigned char *out, *pos;
    const unsigned char *end, *in;
    
    size_t olen;
    
    olen = 4*((len + 2) / 3); /* 3-byte blocks to 4-byte */
    
    if (olen < len)
        return std::string(); /* integer overflow */
    
    std::string outStr;
    outStr.resize(olen);
    out = (unsigned char*)&outStr[0];
    
    end = src + len;
    in = src;
    pos = out;
    while (end - in >= 3) {
        *pos++ = base64_table[in[0] >> 2];
        *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = base64_table[in[2] & 0x3f];
        in += 3;
    }
    
    if (end - in) {
        *pos++ = base64_table[in[0] >> 2];
        if (end - in == 1) {
            *pos++ = base64_table[(in[0] & 0x03) << 4];
            *pos++ = '=';
        }
        else {
            *pos++ = base64_table[((in[0] & 0x03) << 4) |
                                  (in[1] >> 4)];
            *pos++ = base64_table[(in[1] & 0x0f) << 2];
        }
        *pos++ = '=';
    }
    
    return outStr;
}

static const int B64index[256] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
    7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,
    0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

inline std::string b64decode(const void* data, const size_t len)
{
    unsigned char* p = (unsigned char*)data;
    int pad = len > 0 && (len % 4 || p[len - 1] == '=');
    const size_t L = ((len + 3) / 4 - pad) * 4;
    std::string str(L / 4 * 3 + pad, '\0');
    
    for (size_t i = 0, j = 0; i < L; i += 4)
    {
        int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 | B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
        str[j++] = n >> 16;
        str[j++] = n >> 8 & 0xFF;
        str[j++] = n & 0xFF;
    }
    if (pad)
    {
        int n = B64index[p[L]] << 18 | B64index[p[L + 1]] << 12;
        str[str.size() - 1] = n >> 16;
        
        if (len > L + 2 && p[L + 2] != '=')
        {
            n |= B64index[p[L + 2]] << 6;
            str.push_back(n >> 8 & 0xFF);
        }
    }
    return str;
}


static const unsigned char base16_table[17] = "0123456789abcdef";

inline std::string base16_encode(std::string src)
{
    
    unsigned char* in = (unsigned char*)&src[0];
    
    size_t len = src.length();
    std::string outStr;
    outStr.resize(len * 2);
    unsigned char* out = (unsigned char*)&outStr[0];
    
    for(int i = 0; i < len; i++)
    {
        out[i * 2 + 0] = base16_table[(in[i]/0x10) & 0xf];
        out[i * 2 + 1] = base16_table[(in[i]) & 0xf];
    }
    
    return outStr;
}

inline bool isNumber(std::string s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

inline uint64_t playerIDFromString(std::string playerID)
{
    if(isNumber(playerID) && playerID.size() < 22)
    {
        try
        {
            uint64_t result = std::stoull(playerID, 0, 10);
            return result;
        }
        catch(...){}
    }
    
    try
    {
        uint64_t result = std::stoull(playerID.substr(0, 16), 0, 16);
        return result;
    }
    catch(...){}
    
    return std::stoull(TuiSHA1::sha1(playerID).substr(0, 16), 0, 16);
}

inline std::string doubleToString(double d)
{
    std::stringstream ss;
    ss << std::setprecision( std::numeric_limits<double>::max_digits10);
    ss << d;
    return ss.str();
}

}

#endif
