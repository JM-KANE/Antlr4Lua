#ifndef _STRUCT_H
#define _STRUCT_H

#include "Value.h"
#include "stdlib/libtype.h"

namespace lua
{
struct Upvalue
{
    ValuePtr val;
};

using UpvaluePtr = std::shared_ptr<Upvalue>;

struct Closure
{
    const Prototype* proto{};
    Function func{};
    std::vector<UpvaluePtr> upvals;

    Closure(const Prototype* p);
    Closure(Function f, size_t nUpvals);
};

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
    template <typename T>
    auto& Push(T&& v)
    {
        if (top == slots.size())
        {
            // TODO error
        }
        slots[top] = std::make_unique<Value>(std::forward<T>(v));
        ++top;
        return *slots[top];
    }
    ValuePtr Pop();

    void PushN(std::vector<ValuePtr>& vals, int64_t n, size_t start = 0);
    std::vector<ValuePtr> PopN(int64_t n);
    size_t AbsIndex(int64_t idx) const;
    Value Get(int64_t idx) const;
    void Set(int64_t idx, Value val);
    void Reverse(size_t from, size_t to);

    bool IsValid(int32_t idx) const;
};

struct Table
{
private:
    static int64_t KeyToInt(const Value& key);
    void ShrinkArray();
    void ExpandArray();

public:
    Table* metatable{};
    std::vector<ValuePtr> arr;
    std::unordered_map<Value, ValuePtr> map;
    std::unordered_map<Value, const Value*> keys;
    uint8_t changed{1};

    Table() = default;
    Table(size_t nArr, size_t nRec);

    bool HasMetafield(const std::string& fieldName) const;
    size_t Len() const;

    const ValuePtr* Get(const Value& key);

    void Put(Value&& key, Value&& val);
    const Value* NextKey(const Value& key);
    void InitKeys();
};

}  // namespace lua

#endif