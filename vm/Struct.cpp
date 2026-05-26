#include "State.h"
#include "Struct.h"

using namespace lua;

lua::Closure::Closure(Prototype* p) : proto{p}, upvals(p->Upvalues.size())
{
}

lua::Stack::Stack(size_t size, State* st) : slots(size), state{st}
{
}

void lua::Stack::Check(size_t n)
{
    auto free = slots.size() - top;
    for (size_t i = free; i < n; i++)
    {
        slots.emplace_back();
    }
}

Value lua::Stack::Pop()
{
    if (top < 1)
    {
        // TODO error
    }

    --top;
    auto v = std::move(slots.back());
    slots.back() = nullptr;
    return v;
}

void lua::Stack::PushN(std::vector<Value>& vals, int64_t n, size_t start)
{
    auto nV = vals.size();
    if (n < 0)
    {
        n = nV;
    }
    for (size_t i = start; i < n; i++)
    {
        Push(i < nV ? std::move(vals[i]) : nullptr);
    }
}

std::vector<Value> lua::Stack::PopN(int64_t n)
{
    std::vector<Value> v;
    if (n > 0)
    {
        v.resize(n);
        for (size_t i = 0; i < n; i++)
        {
            auto val = Pop();
            *(v.rbegin() + i) = std::move(val);
        }
    }
    return v;
}

size_t lua::Stack::AbsIndex(int64_t idx) const
{
    if (idx >= 0 || idx <= cv::REGISTRYINDEX)
    {
        return idx;
    }
    return idx + top + 1;
}

Value lua::Stack::Get(int64_t idx) const
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

void lua::Stack::Set(int64_t idx, Value val)
{
    if (idx < cv::REGISTRYINDEX)
    {
        auto uvIdx = cv::REGISTRYINDEX - idx - 1;
        if (closure && uvIdx < closure->upvals.size())
        {
            *closure->upvals[uvIdx] = std::move(val);
        }
        return;
    }
    if (idx == cv::REGISTRYINDEX)
    {
        // TODO error
        return;
    }
    auto absIdx = AbsIndex(idx);
    if (absIdx > 0 && absIdx <= top)
    {
        slots[absIdx - 1] = std::move(val);
    }
    return;
}

void lua::Stack::Reverse(size_t from, size_t to)
{
    if (to > from)
    {
        std::reverse(slots.begin() + from, slots.end() + to + 1);
    }
}

bool lua::Stack::IsValid(int32_t idx) const
{
    if (idx < cv::REGISTRYINDEX)
    {
        auto uvIdx = cv::REGISTRYINDEX - idx - 1;
        return closure && uvIdx < closure->upvals.size();
    }
    if (idx == cv::REGISTRYINDEX)
    {
        return true;
    }
    auto absIdx = AbsIndex(idx);
    return absIdx > 0 && absIdx <= top;
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
    while (arr.back().IsNil())
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
        if (it != map.end() && !it->second.IsNil())
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

Value* lua::Table::Get(const Value& key)
{
    int64_t idx = KeyToInt(key);
    if (idx >= 1 && idx <= arr.size())
    {
        return &arr[idx - 1];
    }

    auto it = map.find(key);

    return it == map.end() || it->second.IsNil() ? nullptr : &it->second;
}

void lua::Table::Put(Value&& key, Value&& val)
{
    // TODO key error

    changed = 1;
    int64_t idx = KeyToInt(key);
    if (idx >= 1)
    {
        auto arrLen = arr.size();
        if (idx <= arrLen)
        {
            bool isNil = val.IsNil();
            arr[idx - 1] = std::move(val);
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
            arr.emplace_back(std::move(val));
            ExpandArray();
            return;
        }
    }
    map.insert_or_assign(std::move(key), std::move(val));
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