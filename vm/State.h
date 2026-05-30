#ifndef _STATE_H
#define _STATE_H

#include "Stack.h"
namespace lua
{
namespace op
{
struct Le;
}  // namespace op
class VirtualMachine;
struct State
{
    uint8_t color{};
    VirtualMachine* vm{};
    Table& registry;
    std::vector<std::unique_ptr<Stack>> stacks;

    State(VirtualMachine* _vm, Table& reg);
    void Mark(std::vector<Value>& grey);
    void MarkChildren(std::vector<Value>& grey);

    std::ostream& Out() const;
    std::ostream& Err() const;

    TStatus Load(const std::string& data, const std::string& chunkName, std::string_view mode);
    TStatus LoadFile(std::string_view filename);
    TStatus LoadFileX(std::string_view filename, std::string_view mode);

    template <size_t N>
    void SetFuncs(const FuncReg<N>& l, size_t nup)
    {
        auto snup = (int32_t)nup;
        CheckStack2(snup, "too many upvalues");
        for (auto&& [name, fun] : l)
        {
            for (size_t i = 0; i < nup; i++)
            {
                PushValue(-snup);
            }
            if (fun)
                PushFuncClosure(fun, snup);
            else
                PushNil();
            SetField(-snup - 2, name);
        }
        Pop(nup);
    }

    void OpenLibs();
    template <size_t N>
    void NewLib(const FuncReg<N>& l)
    {
        NewLibTable(l);
        SetFuncs(l, 0);
    }
    template <size_t N>
    void NewLibTable(const FuncReg<N>& l)
    {
        CreateTable(0, N);
    }
    Table* GetArgs();

    void RequireF(const char* modname, Function openf, bool glb);

    auto& stack() const
    {
        return *stacks.back();
    }

    void Rotate(int64_t idx, int64_t n);
    void Insert(int64_t idx);

    void PushFunction(Function f);
    void PushFuncClosure(Function f, int32_t n);
    Stack& PushLuaStack(std::unique_ptr<Stack>&& stk);
    Stack& PushLuaStack(size_t size, State* st);
    std::unique_ptr<Stack> PopLuaStack();

    uint32_t Fetch() const;
    int32_t AbsIndex(int32_t idx);

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
    uint8_t GetGlobal(const std::string& name);
    uint8_t GetMetafield(int32_t idx, const std::string_view& sv);
    bool GetMetatable(int32_t idx);
    bool GetI(int32_t idx, int64_t i);
    uint8_t RawGet(int32_t idx);
    void SetTable(int32_t idx);
    void SetField(int32_t idx, std::string k);
    void SetMetatable(int32_t idx);
    void SetGlobal(std::string k);
    void SetI(int32_t idx, int64_t i);
    void RawSet(int32_t idx);
    void CreateTable(int32_t nArr, int32_t nRec);
    void NewTable();
    void Copy(int32_t fromIdx, int32_t toIdx);
    void Replace(int32_t idx);
    void AddPC(int32_t n);
    void CloseUpvalues(int32_t n);
    void Call(int32_t nArgs, int32_t nRes);
    TStatus PCall(int32_t nArgs, int32_t nRes, int32_t msgh);
    bool CallMeta(int obj, std::string_view event);
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
        if constexpr (OP::num_param == 1)
            b = Value{};
        constexpr auto mm = OP::field_name;
        auto [resM, ok] = CallMetamethod(std::move(a), std::move(b), mm);  // unary op
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
    bool RawEqual(int32_t idx1, int32_t idx2);

    int32_t Error();
    template <typename... Ts>
    int32_t Error2(const char* fmt, Ts... args)
    {
        std::string msg;
        if constexpr (sizeof...(args))
        {
            char buffer[128];
            int n = std::snprintf(buffer, sizeof(buffer), fmt, args...);
            msg = std::string(buffer, n);
        }
        else
        {
            msg = fmt;
        }

        PushString(std::move(msg));
        return Error();
    }

    void Len(int32_t idx);
    int64_t Len2(int32_t idx);
    size_t RawLen(int32_t idx);
    void Concat(int32_t n);
    bool CheckStack(int32_t n);
    void CheckStack2(int32_t n, const char* msg);
    void SetTop(int32_t idx);
    void Remove(int32_t idx);

    const char* TypeName(uint8_t tp);
    const char* TypeName2(int32_t idx);
    uint8_t Type(int32_t idx);
    bool IsNil(int32_t idx);
    bool IsNone(int32_t idx);
    bool IsNoneOrNil(int32_t idx);
    bool IsFloat(int32_t idx);
    bool ToBoolean(int32_t idx);
    int64_t ToInteger(int32_t idx);
    std::pair<int64_t, bool> ToIntegerX(int32_t idx);
    double ToFloat(int32_t idx);
    std::pair<double, bool> ToFloatX(int32_t idx);
    Value ToNumber(int32_t idx);
    bool StringToNumber(std::string s);
    bool IsString(int32_t idx);
    std::string ToString(int32_t idx);
    std::pair<std::string, bool> ToStringX(int32_t idx);
    std::string ToString2(int32_t idx);
    int64_t OptInteger(int32_t idx, int64_t def);
    std::string OptString(int32_t idx, std::string_view def);
    void* ToPointer(int32_t idx);

    int64_t CheckInteger(int32_t idx);
    std::string CheckString(int32_t idx);

    void CheckAny(int32_t idx);
    void CheckType(int32_t arg, uint8_t t);
    int32_t ArgError(int32_t idx, const std::string_view& msg);
    void ArgCheck(bool cond, int32_t arg, const std::string_view& extraMsg);

    bool Next(int32_t idx);

    void CollectGarbage();
    void CheckGC();
    void Barrier(const Value& parent, const Value& child);

    std::pair<ValuePtr, bool> CallMetamethod(Value a, Value b, const char* mmName);
    void CallLuaClosure(int32_t nArgs, int32_t nRes, Closure* c);
    void CallFuncClosure(int32_t nArgs, int32_t nRes, Closure* c);
    void RunLuaClosure();
    uint8_t GetTable(const Value& t, const Value& k, bool raw);
    void SetTable(const Value& t, const Value& k, Value v, bool raw);

    void IntError(int32_t idx);
    void TagError(int32_t idx, uint8_t tag);
    int32_t TypeError(int32_t idx, const std::string_view& tname);
};

}  // namespace lua
#endif