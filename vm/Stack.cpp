#include "State.h"
#include "Stack.h"
#include "../code_gen/FuncInfo.h"
#include <algorithm>

using namespace lua;

lua::Stack::Stack(size_t size, State* st) : slots(size), state{st}
{
}

lua::Stack::~Stack()
{
    CloseUpvalues();
}

uint32_t lua::Stack::CurrentLine() const
{
    return closure->proto->LineInfo[pc - 1];
}

void lua::Stack::Check(size_t n)
{
    if (top + n > cv::LUAI_MAXSTACK)
    {
        state->Error2("stack overflow!");
    }
    auto free = slots.size() - top;
    for (size_t i = free; i < n; i++)
    {
        slots.emplace_back(Value::Nil());
    }
}

Value& lua::Stack::Push(ValuePtr&& val)
{
    if (top == slots.size())
    {
        state->Error2("stack overflow!");
    }
    state->Barrier(state, *val);
    slots[top] = std::move(val);
    auto& valEnd = slots[top];
    ++top;
    return *valEnd;
}

Value& lua::Stack::Push(const ValuePtr& val)
{
    auto v = val;
    return Push(std::move(v));
}

ValuePtr lua::Stack::Pop()
{
    if (top < 1)
    {
        return nullptr;
    }

    --top;
    auto v = std::move(slots[top]);
    slots[top] = Value::Nil();
    return v;
}

void lua::Stack::PushN(std::vector<ValuePtr>& vals, int64_t n, size_t start)
{
    auto nV = vals.size();
    if (n < 0)
    {
        n = nV;
    }
    for (size_t i = 0; i < size_t(n); i++)
    {
        auto j = i + start;
        Push(j < nV ? std::move(vals[j]) : Value::Nil());
    }
}

void lua::Stack::PushN(const std::vector<ValuePtr>& vals, int64_t n, size_t start)
{
    auto nV = vals.size();
    if (n < 0)
    {
        n = nV;
    }
    for (size_t i = 0; i < size_t(n); i++)
    {
        auto j = i + start;
        Push(j < nV ? std::make_unique<Value>(*vals[j]) : Value::Nil());
    }
}

std::vector<ValuePtr> lua::Stack::PopN(int64_t n)
{
    std::vector<ValuePtr> v;
    if (n > 0)
    {
        v.resize(n);
        for (size_t i = 0; i < size_t(n); i++)
        {
            auto val = Pop();
            *(v.rbegin() + i) = std::move(val);
        }
    }
    return v;
}

size_t lua::Stack::AbsIndex(int64_t idx) const
{
    if (idx >= 0 || idx <= cv::LUA_REGISTRYINDEX)
    {
        return idx;
    }
    return idx + top + 1;
}

Value lua::Stack::Get(int64_t idx) const
{
    if (idx < cv::LUA_REGISTRYINDEX)
    {
        auto uvIdx = LuaUpvalueIndex((int32_t)idx + 1);
        if (!closure || uvIdx >= (int64_t)closure->upvals.size())
        {
            return nullptr;
        }
        return *closure->upvals[uvIdx]->val;
    }
    if (idx == cv::LUA_REGISTRYINDEX)
    {
        return &state->registry;
    }
    auto absIdx = AbsIndex(idx);
    if (absIdx > 0 && absIdx <= top)
    {
        return *slots[absIdx - 1];
    }
    return nullptr;
}

void lua::Stack::Set(int64_t idx, Value val)
{
    if (idx < cv::LUA_REGISTRYINDEX)
    {
        auto uvIdx = LuaUpvalueIndex((int32_t)idx + 1);
        if (closure && uvIdx < (int64_t)closure->upvals.size())
        {
            *closure->upvals[uvIdx]->val = std::move(val);
        }
        return;
    }
    if (idx == cv::LUA_REGISTRYINDEX)
    {
        state->Error2("modifying the registry is forbidden");
        return;
    }
    auto absIdx = AbsIndex(idx);
    if (absIdx > 0 && absIdx <= top)
    {
        state->Barrier(state, val);
        auto& slot = slots[absIdx - 1];
        if (slot)
            *slot = std::move(val);
        else
            slot = std::make_unique<Value>(std::move(val));
    }
    return;
}

void lua::Stack::Reverse(size_t from, size_t to)
{
    if (to > from)
    {
        std::reverse(slots.begin() + from, slots.begin() + to + 1);
    }
}

bool lua::Stack::IsValid(int32_t idx) const
{
    if (idx < cv::LUA_REGISTRYINDEX)
    {
        auto uvIdx = cv::LUA_REGISTRYINDEX - idx - 1;
        return closure && uvIdx < closure->upvals.size();
    }
    if (idx == cv::LUA_REGISTRYINDEX)
    {
        return true;
    }
    auto absIdx = AbsIndex(idx);
    return absIdx > 0 && absIdx <= top;
}

void lua::Stack::CloseUpvalues()
{
    if (!openuvs.empty())
    {
        for (auto& [i, uv] : openuvs)
        {
            uv->closed = true;
        }
        openuvs.clear();
    }
    for (size_t i = 0; i < RegisterCount(); i++)
    {
        slots[i] = nullptr;
    }
}

int32_t lua::Stack::RegisterCount() const
{
    return closure && closure->proto ? closure->proto->MaxStackSize : 0;
}

void lua::Stack::Mark(std::vector<Value>& grey)
{
    for (auto&& st : slots)
    {
        st->Mark(grey);
    }
    for (auto&& st : varargs)
    {
        st->Mark(grey);
    }
}
