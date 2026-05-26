#ifndef _STRUCT_H
#define _STRUCT_H

#include "Value.h"

namespace lua
{
struct Closure
{
    Prototype* proto{};
    std::vector<Value*> upvals;

    Closure(Prototype* p);
};

struct Stack
{
    std::vector<Value> slots;
    size_t top{};
    State* state{};
    Closure* closure{};
    std::vector<Value> varargs;
    std::unordered_map<size_t, Value*> openuvs;
    size_t pc{};
    Stack* prev{};

    Stack(size_t size, State* st);

    void Check(size_t n);
    template <typename T>
    auto& Push(T&& v)
    {
        if (top == slots.size())
        {
            // TODO error
        }
        slots[top] = std::forward<T>(v);
        ++top;
        return slots[top];
    }
    Value Pop();

    void PushN(std::vector<Value>& vals, int64_t n, size_t start = 0);
    std::vector<Value> PopN(int64_t n);
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
    std::vector<Value> arr;
    std::unordered_map<Value, Value> map;
    std::unordered_map<Value, const Value*> keys;
    uint8_t changed{1};

    Table() = default;
    Table(size_t nArr, size_t nRec);

    bool HasMetafield(const std::string& fieldName) const;
    size_t Len() const;

    Value* Get(const Value& key);

    void Put(Value&& key, Value&& val);
    const Value* NextKey(const Value& key);
    void InitKeys();
};

}  // namespace lua

#endif