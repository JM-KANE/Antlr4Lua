#ifndef _STATE_H
#define _STATE_H

#include "Struct.h"
namespace lua
{
struct State
{
    class VirtualMachine* vm{};
    std::vector<std::unique_ptr<Stack>> stacks;
    Table registry;
    Table globals;

    State();

    auto& stack() const
    {
        return *stacks.back();
    }

    void Rotate(int64_t idx, int64_t n);
    void Insert(int64_t idx);

    Stack& PushLuaStack(size_t size, State* st);
    std::unique_ptr<Stack> PopLuaStack();

    uint32_t Fetch() const;

    void PushNil();
    void PushBoolean(bool b);
    void PushValue(int32_t idx);
    void Pop(size_t n);
    void GetConst(int32_t idx);
    void GetRK(int32_t idx);
    uint8_t GetTable(int32_t idx);
    void SetTable(int32_t idx);
    void CreateTable(int32_t nArr, int32_t nRec);
    void Copy(int32_t fromIdx, int32_t toIdx);
    void Replace(int32_t idx);
    void AddPC(int32_t n);
    void CloseUpvalues(int32_t n);
    void Call(int32_t nArgs, int32_t nRes);
    template <typename OP>
    void Arith()
    {
        auto b = stack().Pop();
        Value a;
        if constexpr (OP::num_param == 2)
        {
            a = stack().Pop();
        }
        else
        {
            a = std::move(b);
        }
        a = a.ConvertToNumber();
        Value res;
        if constexpr (OP::num_param == 2)
        {
            b = b.ConvertToNumber();
            res = std::visit(OP(), a, b);
        }
        else
            res = std::visit(OP(), a);
        if (!res.IsNil())
        {
            stack().Push(std::move(res));
            return;
        }
        constexpr auto mm = OP::field_name;
        auto [resM, ok] = CallMetamethod(std::move(a), std::move(b), mm);
        if (ok)
        {
            stack().Push(std::move(resM));
            return;
        }

        // TODO error;
    }
    void Len(int32_t idx);
    void Concat(int32_t n);
    bool ToBoolean(int32_t idx);
    void CheckStack(int32_t n);

    uint8_t Type(int32_t idx);
    bool IsString(int32_t idx);
    std::string ToString(int32_t idx);
    std::pair<std::string, bool> ToStringX(int32_t idx);

    std::pair<Value, bool> CallMetamethod(Value a, Value b, const char* mmName);
    void CallLuaClosure(int32_t nArgs, int32_t nRes, Closure* c);
    void RunLuaClosure();
    uint8_t GetTable(const Value& t, const Value& k, bool raw);
    void SetTable(const Value& t, const Value& k, Value v, bool raw);
};

}  // namespace lua
#endif