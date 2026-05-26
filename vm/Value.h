#ifndef _VALUE_H
#define _VALUE_H

#include "../code_gen/FuncInfo.h"

namespace lua
{

namespace cv
{
constexpr auto MINSTACK = 20;
constexpr auto MAXSTACK = 1'000'000'000;
constexpr auto REGISTRYINDEX = -MAXSTACK - 1000;
constexpr auto RIDX_MAINTHREAD = 1;
constexpr auto RIDX_GLOBALS = 2;
}  // namespace cv

struct State;
struct Table;
using BaseValue =
    std::variant<std::nullptr_t, bool, int64_t, double, std::string, struct Table*, struct Closure*, void*>;

template <typename... Ts>
using is_lua_number = std::conjunction<
    std::disjunction<std::is_same<std::decay_t<Ts>, int64_t>, std::is_same<std::decay_t<Ts>, double>>...>;
template <typename... Ts>
static constexpr bool is_lua_number_v = is_lua_number<Ts...>::value;

namespace type
{
constexpr uint8_t NIL = 0;
constexpr uint8_t BOOLEAN = 1;
constexpr uint8_t LIGHTUSERDATA = 2;
constexpr uint8_t NUMBER = 3;
constexpr uint8_t STRING = 4;
constexpr uint8_t TABLE = 5;
constexpr uint8_t FUNCTION = 6;
constexpr uint8_t USERDATA = 7;
constexpr uint8_t THREAD = 8;
constexpr uint8_t NONE = -1;
}  // namespace type

struct Value : public BaseValue
{
    using BaseValue::BaseValue;

    bool IsTable() const
    {
        return index() == 5;
    }
    bool IsClosure() const
    {
        return index() == 6;
    }
    bool IsNil() const
    {
        return index() == 0;
    }
    bool IsString() const
    {
        return index() == 4;
    }

    uint8_t TypeOf() const;

    Value ConvertToNumber() const;
    bool ConvertToBoolean() const;

    Table* GetMetatable(State* ls) const;
    Value* GetMetafield(const std::string& fieldName, State* ls) const;
};
}  // namespace lua
namespace std
{
template <>
struct hash<lua::Value> : hash<lua::BaseValue>
{
};

}  // namespace std
#endif
