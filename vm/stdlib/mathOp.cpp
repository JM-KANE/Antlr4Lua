#include "mathOp.h"
#include "../State.h"
using namespace lua;

namespace
{
template <typename, typename, typename = void>
struct is_callable : std::false_type
{
};
template <typename F, typename... Args>
struct is_callable<F, void(Args...), std::void_t<decltype(std::declval<F>()(std::declval<Args>()...))>> : std::true_type
{
};
template <typename F, typename... Args>
constexpr bool is_callable_v = is_callable<F, void(Args...)>::value;

template <typename F>
struct CallAdapter
{
    F f = F();
    CallAdapter() = default;
    constexpr CallAdapter(F f_) : f(f_)
    {
    }

    template <typename... Args>
    auto operator()(Args&&... args) const
    {
        if constexpr (is_lua_number_v<Args...>)
            return f(std::forward<Args>(args)...);
        else
            return nullptr;
    }
};

struct LogBase
{
    auto operator()(double x, double y) const
    {
        auto dx = double(x);
        auto dy = double(y);
        return 10. == dy ? std::log10(dx) : std::log(dx) / std::log(dy);
    }
};
struct FmodPrecise
{
    auto operator()(int64_t x, int64_t y) const
    {
        return x % y;
    }
    auto operator()(auto x, auto y) const
    {
        return std::fmod(double(x), double(y));
    }
};
struct ModfPrecise
{
    std::pair<int64_t, double> operator()(int64_t x) const
    {
        return {x, 0};
    }
    std::pair<int64_t, double> operator()(auto x) const
    {
        auto dx = double(x);
        double i;
        auto frac = std::modf(dx, &i);
        return {int64_t(i), frac};
    }
};

template <bool LT>
void Minmax(lua::State* ls)
{
    auto n = ls->GetTop();
    int32_t idx = 1;
    ls->ArgCheck(n, 1, "value expected");
    for (size_t i = 1; i < n; i++)
    {
        auto newIdx = int32_t(i) + 1;
        if constexpr (LT)
        {
            if (ls->Compare<lua::op::Lt>(newIdx, idx))
                idx = newIdx;
        }
        else if (ls->Compare<op::Lt>(idx, newIdx))
            idx = newIdx;
    }
    ls->PushValue(idx);
}

}  // namespace

namespace lua
{
namespace stdlib
{
namespace math
{
int32_t Random(State* ls)
{
    auto nArg = ls->GetTop();
    switch (nArg)
    {
    case 1:
    case 2:
    {
        auto m = ls->CheckInteger(1);
        int64_t n = 1;
        if (1 == nArg)
            std::swap(m, n);
        else
            n = ls->CheckInteger(2);
        auto res = ls->RandomRange(m, n);
        ls->PushInteger(res);
    }
    break;
    default:
    {
        auto res = ls->RandomDefault();
        ls->PushNumber(res);
    }
    break;
    }
    return 1;
}
int32_t RandomSeed(State* ls)
{
    auto n = ls->CheckInteger(1);
    ls->SetSeed(seed_type(n));
    return 0;
}
int32_t Max(State* ls)
{
    Minmax<false>(ls);
    return 1;
}
int32_t Min(State* ls)
{
    Minmax<true>(ls);
    return 1;
}
int32_t Exp(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter([](auto x) { return std::exp(double(x)); }), 1);
    return 1;
}
int32_t Log(State* ls)
{
    ls->ArgToNumber(1);
    if (ls->IsNoneOrNil(2))
        ls->PushTransform(CallAdapter([](auto x) { return std::log(double(x)); }), 1);
    else
    {
        ls->ArgToNumber(2);
        ls->PushTransform(CallAdapter(LogBase{}), 1, 2);
    }
    return 1;
}
int32_t Deg(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter([](auto x) { return x * 180. / std::numbers::pi; }), 1);
    return 1;
}
int32_t Rad(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter([](auto x) { return x * std::numbers::pi / 180.; }), 1);
    return 1;
}
int32_t Sin(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter([](auto x) { return std::sin(double(x)); }), 1);
    return 1;
}
int32_t Cos(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter([](auto x) { return std::cos(double(x)); }), 1);
    return 1;
}
int32_t Tan(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter([](auto x) { return std::tan(double(x)); }), 1);
    return 1;
}
int32_t Asin(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter([](auto x) { return std::asin(double(x)); }), 1);
    return 1;
}
int32_t Acos(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter([](auto x) { return std::acos(double(x)); }), 1);
    return 1;
}
int32_t Atan(State* ls)
{
    ls->ArgToNumber(1);
    if (ls->IsNoneOrNil(2))
        ls->PushTransform(CallAdapter([](auto x) { return std::atan(double(x)); }), 1);
    else
    {
        ls->ArgToNumber(2);
        ls->PushTransform(CallAdapter([](auto y, auto x) { return std::atan2(double(y), double(x)); }), 1, 2);
    }
    return 1;
}
int32_t Ceil(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter(
                          [](auto x) -> int64_t
                          {
                              if constexpr (std::is_same_v<decltype(x), double>)
                                  return (int64_t)std::ceil(x);
                              else
                                  return x;
                          }),
                      1);
    return 1;
}
int32_t Floor(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter(
                          [](auto x) -> int64_t
                          {
                              if constexpr (std::is_same_v<decltype(x), double>)
                                  return (int64_t)std::floor(x);
                              else
                                  return x;
                          }),
                      1);
    return 1;
}
int32_t Fmod(State* ls)
{
    ls->ArgToNumber(1);
    ls->ArgToNumber(2);
    if (ls->IsPureInteger(1) && ls->IsPureInteger(2))
    {
        auto d = ls->ToInteger(2);
        if (d == 0 || d == -1)
        {
            ls->ArgCheck(!d, 2, "zero");
            ls->PushInteger(0);
            return 1;
        }
    }
    ls->PushTransform(CallAdapter(FmodPrecise{}), 1, 2);
    return 1;
}
int32_t Modf(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter(ModfPrecise{}), 1);
    return 2;
}
int32_t Abs(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter([](auto x) -> decltype(std::abs(x)) { return std::abs(x); }), 1);
    return 1;
}

int32_t Sqrt(State* ls)
{
    ls->ArgToNumber(1);
    ls->PushTransform(CallAdapter([](auto x) { return std::sqrt(double(x)); }), 1);
    return 1;
}
int32_t Ult(State* ls)
{
    auto m = static_cast<uint64_t>(ls->CheckInteger(1));
    auto n = static_cast<uint64_t>(ls->CheckInteger(2));
    ls->PushBoolean(m < n);
    return 1;
}
int32_t ToInt(State* ls)
{
    auto [x, ok] = ls->ToIntegerX(1);
    if (ok)
        ls->PushInteger(x);
    else
        ls->PushNil();
    return 1;
}
int32_t Type(State* ls)
{
    auto n = ls->Type(1);
    if (n != cv::type::LUA_TNUMBER)
        ls->PushString("nil");
    else
        ls->PushString(ls->ToIntegerX(1).second ? "integer" : "float");
    return 1;
}

}  // namespace math

}  // namespace stdlib

}  // namespace lua

using namespace stdlib;

constexpr FuncReg<27> mathFuncs{
    Reg{"random", math::Random}, {"randomseed", math::RandomSeed},
    {"max", math::Max},          {"min", math::Min},
    {"exp", math::Exp},          {"log", math::Log},
    {"deg", math::Deg},          {"rad", math::Rad},
    {"sin", math::Sin},          {"cos", math::Cos},
    {"tan", math::Tan},          {"asin", math::Asin},
    {"acos", math::Acos},        {"atan", math::Atan},
    {"ceil", math::Ceil},        {"floor", math::Floor},
    {"fmod", math::Fmod},        {"modf", math::Modf},
    {"abs", math::Abs},          {"sqrt", math::Sqrt},
    {"ult", math::Ult},          {"tointeger", math::ToInt},
    {"type", math::Type},        {"pi", nullptr},
    {"huge", nullptr},           {"maxinteger", nullptr},
    {"mininteger", nullptr},
};

int32_t lua::stdlib::OpenMathLib(State* ls)
{
    ls->NewLib(mathFuncs);
    ls->PushNumber(std::numbers::pi);
    ls->SetField(-2, "pi");
    ls->PushNumber(std::numeric_limits<double>::infinity());
    ls->SetField(-2, "huge");
    ls->PushInteger(std::numeric_limits<int64_t>::max());
    ls->SetField(-2, "maxinteger");
    ls->PushInteger(std::numeric_limits<int64_t>::min());
    ls->SetField(-2, "mininteger");
    return 1;
}