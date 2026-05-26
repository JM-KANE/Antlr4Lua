#include "State.h"
#include "Value.h"
#include "../code_gen/aux.h"

using namespace lua;

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
    if (auto mt = ls->registry.Get(key); mt->IsTable())
    {
        return std::get<Table*>(*mt)->metatable;
    }
    return nullptr;
}

Value* lua::Value::GetMetafield(const std::string& fieldName, State* ls) const
{
    if (auto mt = GetMetatable(ls))
    {
        mt->Get(fieldName);
    }

    return nullptr;
}