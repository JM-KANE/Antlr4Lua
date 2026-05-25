#include "State.h"
#include "Struct.h"

using namespace lua;

uint8_t lua::Value::TypeOf() const
{
    return std::visit(
        [](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, bool>)
                return type::BOOLEAN;
            else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, double>)
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

Table* lua::Value::GetMetatable(State* ls)
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

Value* lua::Value::GetMetafield(const std::string& fieldName, State* ls)
{
    if (auto mt = GetMetatable(ls))
    {
        mt->Get(fieldName);
    }

    return nullptr;
}

lua::Closure::Closure(Prototype* p) : proto{p}, upvals(p->Upvalues.size())
{
}

lua::Stack::Stack(size_t size, State* st) : slots(size), state{st}
{
}

size_t lua::Stack::AbsIndex(int64_t idx)
{
    if (idx >= 0 || idx <= cv::REGISTRYINDEX)
    {
        return idx;
    }
    return idx + top + 1;
}

Value lua::Stack::Get(int64_t idx)
{
    if (idx < cv::REGISTRYINDEX)
    {
        auto uvIdx = cv::REGISTRYINDEX - idx - 1;
        if (!closure || uvIdx >= closure->upvals.size())
        {
            return nullptr;
        }
        return closure->upvals[uvIdx];
    }
    if (idx == cv::REGISTRYINDEX)
    {
        return &state->registry;
    }
    auto absIdx = AbsIndex(idx);
    if (absIdx > 0 && absIdx <= top)
    {
        return slots[absIdx - 1];
    }
    return nullptr;
}

int64_t lua::Table::KeyToInt(const Value& key)
{
    auto idx = key.index();
    if (2 == idx)
    {
        return std::get<int64_t>(key);
    }
    else if (3 == idx)
    {
        return (int64_t)std::get<double>(key);
    }
    return -1;
}

lua::Table::Table(size_t nArr) : arr(nArr)
{
}

Value* lua::Table::Get(const Value& key)
{
    int64_t idx = KeyToInt(key);
    if (idx >= 1 && idx <= arr.size())
    {
        return &arr[idx - 1];
    }

    auto it = map.find(key);

    return it == map.end() ? nullptr : &it->second;
}

void lua::Table::Put(Value&& key, Value&& val)
{
}

void lua::Table::InitKeys()
{
    if (!changed)
    {
        return;
    }

    changed = 0;
    for (int64_t i = 0; i < arr.size(); i++)
    {
        keys[i] = arr[i].index() ? &arr[i] : nullptr;
    }
    for (auto&& [k, v] : map)
    {
        keys[k] = v.index() ? &v : nullptr;
    }
    if (keys.empty())
    {
        return;
    }

    auto itB = keys.begin();
    auto it = std::next(itB);
    for (; it != keys.end(); ++it, ++itB)
    {
        itB->second = &it->first;
    }
    auto n = keys.extract(itB);
    n.mapped() = &keys.begin()->first;
    n.key() = nullptr;
    keys.insert(std::move(n));
}