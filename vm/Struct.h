#ifndef _STRUCT_H
#define _STRUCT_H

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

    uint8_t TypeOf() const;

    Table* GetMetatable(State* ls);
    Value* GetMetafield(const std::string& fieldName, State* ls);
};
}  // namespace lua
namespace std
{
template <>
struct hash<lua::Value> : hash<lua::BaseValue>
{
};

}  // namespace std

namespace lua
{
struct Closure
{
    Prototype* proto{};
    std::vector<Value*> upvals;

    Closure(Prototype* p);
};

struct Stack
{
    std::vector<Value> slots;
    size_t top{};
    State* state{};
    Closure* closure{};
    Stack* prev{};

    Stack(size_t size, State* st);

    template <typename T>
    auto& Push(T&& v)
    {
        return slots.emplace_back(std::forward<T>(v));
    }
    size_t AbsIndex(int64_t idx);
    Value Get(int64_t idx);
};

struct Table
{
private:
    static int64_t KeyToInt(const Value& key);

public:
    Table* metatable{};
    std::vector<Value> arr;
    std::unordered_map<Value, Value> map;
    std::unordered_map<Value, const Value*> keys;
    uint8_t changed{1};

    Table() = default;
    Table(size_t nArr);

    Value* Get(const Value& key);

    void Put(Value&& key, Value&& val);

    void InitKeys();
};

}  // namespace lua

#endif