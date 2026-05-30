#ifndef _OPERATOR_H
#define _OPERATOR_H

#include "Value.h"
#include <utility>
#include <cmath>
#include <functional>
namespace lua
{

namespace op
{

struct divides
{
    auto operator()(auto a, auto b) const
    {
        return double(a) / double(b);
    }
};
struct idivides
{
    int64_t operator()(int64_t a, int64_t b) const
    {
        if (a > 0 && b > 0 || a < 0 && b < 0 || a % b == 0)
            return a / b;
        return a / b - 1;
    }
    auto operator()(auto a, auto b) const
    {
        return std::floor(double(a) / double(b));
    }
};
struct modulus
{
    auto operator()(auto a, auto b) const
    {
        auto d = idivides()(a, b);
        return a - d * b;
    }
};
struct power
{
    auto operator()(auto a, auto b) const
    {
        return std::pow(double(a), double(b));
    }
};
template <typename COP>
struct bitwise
{
    int64_t operator()(int64_t a, int64_t b) const
    {
        return COP()(a, b);
    }
    int64_t operator()(int64_t a) const
    {
        return COP()(a);
    }
    template <typename... Ts>
    std::nullptr_t operator()(Ts...) const
    {
        return nullptr;
    }
};
struct bit_and : bitwise<std::bit_and<>>
{
};
struct bit_or : bitwise<std::bit_or<>>
{
};
struct bit_xor : bitwise<std::bit_xor<>>
{
};
struct bit_not : bitwise<std::bit_not<>>
{
};
struct shift_left
{
    int64_t operator()(int64_t a, int64_t b) const
    {
        return b >= 0 ? a << b : int64_t(uint64_t(a) >> -b);
    }
    std::nullptr_t operator()(auto, auto) const
    {
        return nullptr;
    }
};
struct shift_right
{
    auto operator()(auto a, auto b) const
    {
        return shift_left()(a, -b);
    }
};

//

template <size_t N, typename COP>
struct ArithOperator
{
    static constexpr auto num_param = N;

    template <typename... Ts>
        requires(N == sizeof...(Ts))
    Value operator()(Ts... args) const
    {
        if constexpr (is_lua_number_v<Ts...>)
            return COP()(args...);
        else
            return {};
    }
};

struct Add : ArithOperator<2, std::plus<>>
{
    static constexpr auto field_name = str::ADD;
};
struct Sub : ArithOperator<2, std::minus<>>
{
    static constexpr auto field_name = str::SUB;
};
struct Mul : ArithOperator<2, std::multiplies<>>
{
    static constexpr auto field_name = str::MUL;
};
struct Mod : ArithOperator<2, modulus>
{
    static constexpr auto field_name = str::MOD;
};
struct Pow : ArithOperator<2, power>
{
    static constexpr auto field_name = str::POW;
};
struct Div : ArithOperator<2, divides>
{
    static constexpr auto field_name = str::DIV;
};
struct IDiv : ArithOperator<2, idivides>
{
    static constexpr auto field_name = str::IDIV;
};
struct BAnd : ArithOperator<2, bit_and>
{
    static constexpr auto field_name = str::BAND;
};
struct BOr : ArithOperator<2, bit_or>
{
    static constexpr auto field_name = str::BOR;
};
struct BXor : ArithOperator<2, bit_xor>
{
    static constexpr auto field_name = str::BXOR;
};
struct ShL : ArithOperator<2, shift_left>
{
    static constexpr auto field_name = str::SHL;
};
struct ShR : ArithOperator<2, shift_right>
{
    static constexpr auto field_name = str::SHR;
};
struct UnM : ArithOperator<1, std::negate<>>
{
    static constexpr auto field_name = str::UNM;
};
struct BNot : ArithOperator<1, bit_not>
{
    static constexpr auto field_name = str::BNOT;
};

/* compare */
struct CompareOperator
{
};
struct Eq : CompareOperator
{
    static constexpr auto field_name = str::EQ;
    template <typename T1, typename T2>
    int8_t operator()(T1&& a, T2&& b) const
    {
        using DT1 = std::decay_t<T1>;
        using DT2 = std::decay_t<T2>;
        if constexpr (std::is_same_v<DT1, DT2> || is_lua_number_v<DT1, DT2>)
        {
            if (a == b)
                return 1;
            if constexpr (std::is_same_v<DT1, Table*>)
            {
                return -1;
            }
        }
        return 0;
    }
};
struct Lt : CompareOperator
{
    static constexpr auto field_name = str::LT;
    template <typename T1, typename T2>
    int8_t operator()(T1&& a, T2&& b) const
    {
        using DT1 = std::decay_t<T1>;
        using DT2 = std::decay_t<T2>;
        if constexpr (std::is_same_v<DT1, std::string> && std::is_same_v<DT2, std::string> || is_lua_number_v<DT1, DT2>)
            return a < b;
        else
            return -1;
    }
};
struct Le : CompareOperator
{
    static constexpr auto field_name = str::LE;
    template <typename T1, typename T2>
    int8_t operator()(T1&& a, T2&& b) const
    {
        using DT1 = std::decay_t<T1>;
        using DT2 = std::decay_t<T2>;
        if constexpr (std::is_same_v<DT1, std::string> && std::is_same_v<DT2, std::string> || is_lua_number_v<DT1, DT2>)
            return a <= b;
        else
            return -1;
    }
};

}  // namespace op

}  // namespace lua

#endif