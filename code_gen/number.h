#ifndef _NUMBER_H
#define _NUMBER_H

#include <string>
#include <charconv>
#include <variant>

namespace lua
{
namespace number
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

}  // namespace number

}  // namespace lua
#endif