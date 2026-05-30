// #include "State.h"
#include "Struct.h"

using namespace lua;

lua::Closure::Closure(const Prototype* p) : proto{p}, upvals(p->Upvalues.size())
{
}

lua::Closure::Closure(Function f, size_t nUpvals) : func(f), upvals(nUpvals)
{
}

void lua::Closure::Mark(std::vector<Value>& grey)
{
    if (!color)
    {
        color = 1;
        grey.emplace_back(this);
    }
}

void lua::Closure::MarkChildren(std::vector<Value>& grey)
{
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

void lua::Table::ShrinkArray()
{
    while (arr.back()->IsNil())
    {
        map.emplace(int64_t(arr.size()), std::move(arr.back()));
        arr.pop_back();
    }
}

void lua::Table::ExpandArray()
{
    for (size_t i = arr.size() + 1;; ++i)
    {
        auto it = map.find(int64_t(i));
        if (it != map.end() && !it->second->IsNil())
        {
            auto n = map.extract(it);
            arr.emplace_back(std::move(n.mapped()));
        }
        else
        {
            break;
        }
    }
}

lua::Table::Table(size_t nArr, size_t nRec) : arr(nArr)
{
    map.reserve(nRec);
}

bool lua::Table::HasMetafield(const std::string& fieldName) const
{
    return metatable && metatable->Get(fieldName);
}

size_t lua::Table::Len() const
{
    return arr.size();
}

const ValuePtr* lua::Table::Get(const Value& key)
{
    int64_t idx = KeyToInt(key);
    if (idx >= 1 && idx <= (int64_t)arr.size())
    {
        return &arr[idx - 1];
    }

    auto it = map.find(key);

    return it == map.end() || it->second->IsNil() ? nullptr : &it->second;
}

void lua::Table::Put(Value&& key, Value&& val)
{
    // TODO key error

    changed = 1;
    int64_t idx = KeyToInt(key);
    auto valPtr = std::make_unique<Value>(std::move(val));
    if (idx >= 1)
    {
        auto arrLen = arr.size();
        if (idx <= (int64_t)arrLen)
        {
            bool isNil = valPtr->IsNil();
            arr[idx - 1] = std::move(valPtr);
            if (idx == arrLen && isNil)
            {
                // tombstone
                ShrinkArray();
            }
            return;
        }
        if (idx == arrLen + 1)
        {
            map.erase(key);
            arr.emplace_back(std::move(valPtr));
            ExpandArray();
            return;
        }
    }
    map.insert_or_assign(std::move(key), std::move(valPtr));
}

const Value* lua::Table::NextKey(const Value& key)
{
    InitKeys();
    auto it = keys.find(key);
    return it == keys.end() ? nullptr : it->second;
}

void lua::Table::InitKeys()
{
    if (!changed)
    {
        return;
    }

    changed = 0;
    for (size_t i = 0; i < arr.size(); i++)
    {
        keys[(int64_t)i] = arr[i]->index() ? arr[i].get() : nullptr;
    }
    for (auto&& [k, v] : map)
    {
        keys[k] = v->index() ? v.get() : nullptr;
    }
    if (keys.empty())
    {
        return;
    }

    using iterator = decltype(keys)::iterator;

    std::pair<iterator, iterator> befores;
    const Value* first{};
    for (auto it = keys.begin(); it != keys.end(); ++it)
    {
        if (it->second)
        {
            auto curr = &it->first;
            if (!first)
            {
                first = curr;
            }
            auto end = std::next(befores.second);
            for (auto it = befores.first; it != end; ++it)
            {
                it->second = curr;
            }

            befores.first = it;
        }
        befores.second = it;
    }
    if (!first)
    {
        keys.clear();
    }
    else
    {
        auto next = std::next(befores.first);
        auto n = keys.extract(befores.first);
        for (auto it = next; it != keys.end();)
        {
            it = keys.erase(it);
        }
        n.mapped() = first;
        n.key() = nullptr;
    }
}

void lua::Table::Mark(std::vector<Value>& grey)
{
    if (!color)
    {
        color = 1;
        grey.emplace_back(this);
    }
}

void lua::Table::MarkChildren(std::vector<Value>& grey)
{
    for (auto&& v : arr)
    {
        v->Mark(grey);
    }
    for (auto&& [k, v] : map)
    {
        v->Mark(grey);
    }
    if (metatable)
    {
        metatable->Mark(grey);
    }
}
