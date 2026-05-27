#ifndef _STATE_H
#define _STATE_H

#include "Struct.h"
namespace lua
{
namespace op
{
struct Le;
}  // namespace op

struct State
{
    class VirtualMachine* vm{};
    std::vector<std::unique_ptr<Stack>> stacks;
    Table registry;
    Table globals;

    State();

    void OpenLibs();
    void RequireF(const char* modname, Function openf, bool glb);

    auto& stack() const
    {
        return *stacks.back();
    }

    void Rotate(int64_t idx, int64_t n);
    void Insert(int64_t idx);

    void PushFunction(Function f);
    void PushFuncClosure(Function f, int32_t n);
    Stack& PushLuaStack(size_t size, State* st);
    std::unique_ptr<Stack> PopLuaStack();

    uint32_t Fetch() const;

    void PushNil();
    void PushBoolean(bool b);
    void PushInteger(int64_t n);
    void PushNumber(double f);
    void PushString(std::string s);
    void PushAny(Value v);
    void PushValue(int32_t idx);
    void Pop(size_t n);
    void PushGlobalTable();

    void GetConst(int32_t idx);
    void GetRK(int32_t idx);
    void LoadVararg(int32_t n);
    void LoadProto(int32_t idx);
    size_t GetTop();
    int32_t RegisterCount();
    uint8_t GetTable(int32_t idx);
    uint8_t GetField(int32_t idx, std::string k);
    bool GetSubTable(int32_t idx, std::string fname);
    template <size_t N>
    void SetFuncs(const FuncReg<N>& l, size_t nup)
    {
        for (auto&& [name, fun] : l)
        {
            for (size_t i = 0; i < nup; i++)
            {
                PushValue(-nup);
            }
            PushFuncClosure(fun, nup);
            SetField(-nup - 2, name);
        }
        Pop(nup);
    }
    void SetTable(int32_t idx);
    void SetField(int32_t idx, std::string k);
    void SetGlobal(std::string k);
    void SetI(int32_t idx, int64_t i);
    void CreateTable(int32_t nArr, int32_t nRec);
    void NewTable();
    void Copy(int32_t fromIdx, int32_t toIdx);
    void Replace(int32_t idx);
    void AddPC(int32_t n);
    void CloseUpvalues(int32_t n);
    void Call(int32_t nArgs, int32_t nRes);
    template <typename OP>
    void Arith()
    {
        auto b = std::move(*stack().Pop());
        Value a;
        if constexpr (OP::num_param == 2)
        {
            a = std::move(*stack().Pop());
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
    template <typename OP>
    bool Compare(int32_t idx1, int32_t idx2)
    {
        if (stack().IsValid(idx1) && stack().IsValid(idx2))
        {
            auto a = stack().Get(idx1);
            auto b = stack().Get(idx2);

            auto state = std::visit(OP(), a, b);
            if (-1 == state)
            {
                // try meta
                if (auto [res, ok] = CallMetamethod(a, b, OP::field_name); ok)
                {
                    return res->ConvertToBoolean();
                }
                if constexpr (std::is_same_v<op::Le, OP>)
                {
                    if (auto [res, ok] = CallMetamethod(b, a, str::LT); ok)
                    {
                        return !res->ConvertToBoolean();
                    }
                }
            }
            else
            {
                return state;
            }
        }
        return false;
    }

    void Len(int32_t idx);
    void Concat(int32_t n);
    void CheckStack(int32_t n);
    void SetTop(int32_t idx);
    void Remove(int32_t idx);

    uint8_t Type(int32_t idx);
    bool IsNil(int32_t idx);
    bool ToBoolean(int32_t idx);
    int64_t ToInteger(int32_t idx);
    std::pair<int64_t, bool> ToIntegerX(int32_t idx);
    double ToFloat(int32_t idx);
    std::pair<double, bool> ToFloatX(int32_t idx);
    Value ToNumber(int32_t idx);
    bool IsString(int32_t idx);
    std::string ToString(int32_t idx);
    std::pair<std::string, bool> ToStringX(int32_t idx);

    std::pair<ValuePtr, bool> CallMetamethod(Value a, Value b, const char* mmName);
    void CallLuaClosure(int32_t nArgs, int32_t nRes, Closure* c);
    void CallFuncClosure(int32_t nArgs, int32_t nRes, Closure* c);
    void RunLuaClosure();
    uint8_t GetTable(const Value& t, const Value& k, bool raw);
    void SetTable(const Value& t, const Value& k, Value v, bool raw);
};

}  // namespace lua
#endif