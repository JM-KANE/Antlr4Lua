#include "State.h"
#include "Stack.h"

using namespace lua;

lua::Stack::Stack(size_t size, State* st) : slots(size), state{st}
{
}

void lua::Stack::Check(size_t n)
{
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
        // TODO error
    }
    state->Barrier(state, *val);
    slots[top] = std::move(val);
    ++top;
    return *slots[top];
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
        // TODO error
    }

    --top;
    auto v = std::move(slots.back());
    slots.back() = Value::Nil();
    return v;
}

void lua::Stack::PushN(std::vector<ValuePtr>& vals, int64_t n, size_t start)
{
    auto nV = vals.size();
    if (n < 0)
    {
        n = nV;
    }
    for (size_t i = start; i < n; i++)
    {
        Push(i < nV ? std::move(vals[i]) : Value::Nil());
    }
}

std::vector<ValuePtr> lua::Stack::PopN(int64_t n)
{
    std::vector<ValuePtr> v;
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
        return *closure->upvals[uvIdx]->val;
    }
    if (idx == cv::REGISTRYINDEX)
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
    if (idx < cv::REGISTRYINDEX)
    {
        auto uvIdx = cv::REGISTRYINDEX - idx - 1;
        if (closure && uvIdx < closure->upvals.size())
        {
            *closure->upvals[uvIdx]->val = std::move(val);
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
        state->Barrier(state, val);
        slots[absIdx - 1] = std::make_unique<Value>(std::move(val));
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
