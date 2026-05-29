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
    uint8_t color{};
    const Prototype* proto{};
    Function func{};
    std::vector<UpvaluePtr> upvals;

    Closure(const Prototype* p);
    Closure(Function f, size_t nUpvals);

    void Mark(std::vector<Value>& grey);
    void MarkChildren(std::vector<Value>& grey);
};

struct Table
{
private:
    static int64_t KeyToInt(const Value& key);
    void ShrinkArray();
    void ExpandArray();

public:
    uint8_t color{};
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

    void Mark(std::vector<Value>& grey);
    void MarkChildren(std::vector<Value>& grey);
};

}  // namespace lua

#endif