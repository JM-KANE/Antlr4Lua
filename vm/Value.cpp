#include "State.h"
#include "Value.h"
#include "../code_gen/aux.h"

using namespace lua;

std::unique_ptr<Value> lua::Value::Nil()
{
    return std::make_unique<Value>();
}

uint8_t lua::Value::TypeOf() const
{
    return std::visit(
        [](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, bool>)
                return type::BOOLEAN;
            else if constexpr (is_lua_number_v<T>)
                return type::NUMBER;
            else if constexpr (std::is_same_v<T, std::string>)
                return type::STRING;
            else if constexpr (std::is_same_v<T, Table*>)
                return type::TABLE;
            else
                return type::NIL;
        },
        *this);
}

Value lua::Value::ConvertToNumber() const
{
    return std::visit(
        [this](auto&& arg) -> Value
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (is_lua_number_v<T>)
            {
                return *this;
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                auto v = aux::ToNum(arg);
                return std::visit([](auto& arg_) -> Value { return arg_; }, v);
            }
            else
                return {};
        },
        *this);
}

std::pair<int64_t, bool> lua::Value::ConvertToInteger() const
{
    auto v = ConvertToNumber();
    return std::visit(
        [](auto&& arg) -> std::pair<int64_t, bool>
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int64_t>)
                return {arg, true};
            else if constexpr (std::is_same_v<T, double>)
                return aux::FloatToInteger(arg);
            else
                return {};
        },
        v);
}

std::pair<double, bool> lua::Value::ConvertToFloat() const
{
    auto v = ConvertToNumber();
    return std::visit(
        [](auto&& arg) -> std::pair<int64_t, bool>
        {
            if constexpr (is_lua_number_v<decltype(arg)>)
                return {arg, true};
            else
                return {};
        },
        v);
}

bool lua::Value::ConvertToBoolean() const
{
    return std::visit(
        [](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, bool>)
                return arg;
            else if constexpr (std::is_same_v<T, std::nullptr_t>)
                return false;
            else
                return true;
        },
        *this);
}

Table* lua::Value::GetMetatable(State* ls) const
{
    if (IsTable())
    {
        return std::get<Table*>(*this)->metatable;
    }
    auto key = "_MT" + std::to_string(TypeOf());
    if (auto& mt = *ls->registry.Get(key); mt->IsTable())
    {
        return std::get<Table*>(*mt)->metatable;
    }
    return nullptr;
}

Value* lua::Value::GetMetafield(const std::string& fieldName, State* ls) const
{
    if (auto mt = GetMetatable(ls))
    {
        if (auto pv = mt->Get(fieldName))
            return pv->get();
    }

    return nullptr;
}

void lua::Value::Mark(std::vector<Value>& grey) const
{
    std::visit(
        [&](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Table*> || std::is_same_v<T, Closure*> || std::is_same_v<T, State*>)
                arg->Mark(grey);
        },
        *this);
}

void lua::Value::MarkChildren(std::vector<Value>& grey)
{
    std::visit(
        [&](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Table*> || std::is_same_v<T, Closure*> || std::is_same_v<T, State*>)
                arg->MarkChildren(grey);
        },
        *this);
}

void lua::Value::SetBlack()
{
    SetColor(2);
}

void lua::Value::SetColor(uint8_t c)
{
    std::visit(
        [&](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Table*> || std::is_same_v<T, Closure*> || std::is_same_v<T, State*>)
                arg->color = c;
        },
        *this);
}

uint8_t lua::Value::Color() const
{
    return std::visit(
        [&](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Table*> || std::is_same_v<T, Closure*> || std::is_same_v<T, State*>)
                return arg->color;
            else
                return uint8_t();
        },
        *this);
}
