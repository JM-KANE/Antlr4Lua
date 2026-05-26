#include "aux.h"
#include <algorithm>

namespace lua
{
namespace aux
{
std::string Trim(const std::string& s)
{
    auto it = std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); });
    auto str = s.substr(it - s.begin());
    auto itR = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) { return !std::isspace(ch); });
    str.erase(itR.base(), str.end());
    return str;
}

std::variant<std::nullptr_t, int64_t, double> ToNum(const std::string& s)
{
    std::variant<std::nullptr_t, int64_t, double> num;
    auto str = Trim(s);
    bool isHex = str.size() > 2;
    if (isHex)
    {
        isHex = '0' == str.front() && ('x' == str[1] || 'X' == str[1]);
    }

    std::from_chars_result res;
    if (isHex)
    {
        auto i = ToHex(str, &res);
        if (res.ec == std::errc{})
        {
            return i;
        }
        auto d = ToHexFloat(str, &res);
        if (res.ec == std::errc{})
        {
            return d;
        }
    }
    else
    {
        auto i = ToInt(str, &res);
        if (res.ec == std::errc{})
        {
            return i;
        }
        auto d = ToFloat(str, &res);
        if (res.ec == std::errc{})
        {
            return d;
        }
    }
    return nullptr;
}

int64_t ToInt(const std::string& str, std::from_chars_result* res)
{
    auto cs = str.c_str();
    int64_t val;
    std::from_chars(cs, cs + str.size(), val);
    return val;
}

int64_t ToHex(const std::string& str, std::from_chars_result* res)
{
    auto cs = str.c_str();
    int64_t val;
    std::from_chars(cs + 2, cs + str.size(), val, 16);
    return val;
}

double ToFloat(const std::string& str, std::from_chars_result* res)
{
    auto cs = str.c_str();
    double val;
    std::from_chars(cs, cs + str.size(), val);
    return val;
}

double ToHexFloat(const std::string& str, std::from_chars_result* res)
{
    auto cs = str.c_str();
    double val;
    std::from_chars(cs + 2, cs + str.size(), val, std::chars_format::hex);
    return val;
}

}  // namespace aux
}  // namespace lua