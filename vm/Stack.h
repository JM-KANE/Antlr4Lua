#ifndef _STACK_H
#define _STACK_H

#include "Struct.h"

namespace lua
{
struct Stack
{
    std::vector<ValuePtr> slots;
    size_t top{};
    State* state{};
    Closure* closure{};
    std::vector<ValuePtr> varargs;
    std::unordered_map<size_t, UpvaluePtr> openuvs;
    size_t pc{};
    Stack* prev{};

    Stack(size_t size, State* st);

    void Check(size_t n);
    Value& Push(ValuePtr&& val);
    Value& Push(const ValuePtr& val);
    template <typename T>
        requires(!std::is_assignable_v<ValuePtr, T>)
    Value& Push(T&& v)
    {
        return Push(std::make_shared<Value>(std::forward<T>(v)));
    }
    ValuePtr Pop();

    void PushN(std::vector<ValuePtr>& vals, int64_t n, size_t start = 0);
    std::vector<ValuePtr> PopN(int64_t n);
    size_t AbsIndex(int64_t idx) const;
    Value Get(int64_t idx) const;
    void Set(int64_t idx, Value val);
    void Reverse(size_t from, size_t to);

    bool IsValid(int32_t idx) const;

    void Mark(std::vector<Value>& grey);
};

}  // namespace lua

#endif