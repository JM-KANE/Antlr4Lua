#ifndef _AUX_H
#define _AUX_H

#include <string>
#include <charconv>
#include <variant>

namespace lua
{
namespace aux
{

enum class NumType
{
    NONE,
    INT,
    HEX,
    FLOAT,
    HEXFLOAT,
};

std::string Trim(const std::string& s);
std::variant<std::nullptr_t, int64_t, double> ToNum(const std::string& s);

int64_t ToInt(const std::string& str, std::from_chars_result* res = {});
int64_t ToHex(const std::string& str, std::from_chars_result* res = {});
double ToFloat(const std::string& str, std::from_chars_result* res = {});
double ToHexFloat(const std::string& str, std::from_chars_result* res = {});

std::pair<int64_t, bool> FloatToInteger(double f);

}  // namespace aux

}  // namespace lua
#endif