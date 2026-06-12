#ifndef _VALUE_H
#define _VALUE_H

// #include "../code_gen/FuncInfo.h"
#include "stdlib/libtype.h"
#include <variant>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace lua
{

struct State;
struct Table;
struct Closure;
using BaseValue = std::variant<std::nullptr_t, bool, int64_t, double, std::string, Table*, Closure*, State*>;

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
    bool IsFloat() const
    {
        return index() == 3;
    }

    static std::unique_ptr<Value> Nil();

    uint8_t TypeOf() const;

    Value ConvertToNumber() const;
    std::pair<int64_t, bool> ConvertToInteger(bool considerStr = true) const;
    std::pair<double, bool> ConvertToFloat() const;
    bool ConvertToBoolean() const;

    Table* GetMetatable(State* ls) const;
    Value* GetMetafield(const std::string& fieldName, State* ls) const;
    void SetMetatable(Table* mt, State* ls);

    void Mark(std::vector<Value>& grey) const;
    void MarkChildren(std::vector<Value>& grey);
    void SetBlack();
    void SetColor(uint8_t c);
    uint8_t Color() const;
};

using ValuePtr = std::shared_ptr<Value>;
}  // namespace lua
namespace std
{
template <>
struct hash<lua::Value> : hash<lua::BaseValue>
{
};

}  // namespace std
#endif
