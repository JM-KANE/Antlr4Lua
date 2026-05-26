#include "VirtualMachine.h"
#include "Instruction.h"
#include "State.h"
#include <iomanip>
using namespace lua;

lua::State::State() : registry(8, 0)
{
    registry.Put(cv::RIDX_MAINTHREAD, this);
    globals.map.reserve(20);
    registry.Put(cv::RIDX_GLOBALS, &globals);
    PushLuaStack(cv::MINSTACK, this);
}

void lua::State::Rotate(int64_t idx, int64_t n)
{
    auto t = stack().top - 1;
    auto p = stack().AbsIndex(idx) - 1;
    int32_t m = n >= 0 ? t - n : p - n - 1;
    stack().Reverse(p, m);
    stack().Reverse(m + 1, t);
    stack().Reverse(p, t);
}

void lua::State::Insert(int64_t idx)
{
    Rotate(idx, 1);
}

Stack& lua::State::PushLuaStack(size_t size, State* st)
{
    auto& stackPrev = stack();
    auto& stack = stacks.emplace_back(std::make_unique<Stack>(size, st));
    stack->prev = &stackPrev;
    return *stack;
}

std::unique_ptr<Stack> lua::State::PopLuaStack()
{
    auto stk = std::move(stacks.back());
    stk->prev = {};
    stacks.pop_back();
    return stk;
}

uint32_t lua::State::Fetch() const
{
    auto i = stack().closure->proto->Code[stack().pc];
    ++stack().pc;
    return i;
}

void lua::State::PushNil()
{
    stack().Push(nullptr);
}

void lua::State::PushBoolean(bool b)
{
    stack().Push(b);
}

void lua::State::PushValue(int32_t idx)
{
    stack().Push(stack().Get(idx));
}

void lua::State::Pop(size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        stack().Pop();
    }
}

void lua::State::GetConst(int32_t idx)
{
    auto& val = stack().closure->proto->Constants[idx];
    std::visit([this](auto&& arg) { stack().Push(arg); }, val);
}

void lua::State::GetRK(int32_t idx)
{
    if (idx > 0xff)
        GetConst(0xff & idx);
    else
        PushValue(idx + 1);
}

uint8_t lua::State::GetTable(int32_t idx)
{
    auto t = stack().Get(idx);
    auto k = stack().Pop();
    return GetTable(t, k, false);
}

void lua::State::SetTable(int32_t idx)
{
    auto t = stack().Get(idx);
    auto v = stack().Pop();
    auto k = stack().Pop();
    SetTable(t, k, std::move(v), false);
}

void lua::State::CreateTable(int32_t nArr, int32_t nRec)
{
    auto& t = vm->NewLuaTable(size_t(nArr), size_t(nRec));
    stack().Push(&t);
}

void lua::State::Copy(int32_t fromIdx, int32_t toIdx)
{
    auto val = stack().Get(fromIdx);
    stack().Set(toIdx, std::move(val));
}

void lua::State::Replace(int32_t idx)
{
    stack().Set(idx, stack().Pop());
}

void lua::State::AddPC(int32_t n)
{
    stack().pc += n;
}

void lua::State::CloseUpvalues(int32_t n)
{
}

void lua::State::Call(int32_t nArgs, int32_t nRes)
{
    auto val = stack().Get(-(nArgs + 1));

    Closure* c{};
    if (val.IsClosure())
    {
        c = std::get<Closure*>(val);
    }
    else if (auto mf = val.GetMetafield(str::CALL, this))
    {
        if (mf->IsClosure())
        {
            c = std::get<Closure*>(*mf);
            stack().Push(val);
            Insert(-(nArgs + 2));
            nArgs += 1;
        }
    }

    if (c)
    {
        if (c->proto)
        {
            CallLuaClosure(nArgs, nRes, c);
        }
        else
        {
            // TODO no proto
        }
    }
}

void lua::State::Len(int32_t idx)
{
    auto val = stack().Get(idx);
    if (val.IsString())
    {
        stack().Push(int64_t(std::get<std::string>(val).size()));
    }
    else
    {
        auto [v, ok] = CallMetamethod(val, val, str::LEN);
        if (ok)
        {
            stack().Push(std::move(v));
        }
        else if (val.IsTable())
        {
            auto tbl = std::get<Table*>(val);
            stack().Push(int64_t(tbl->Len()));
        }
        else
        {
            // TODO error
        }
    }
}

void lua::State::Concat(int32_t n)
{
    for (size_t i = 1; i < n; i++)
    {
        if (IsString(-1) && IsString(-2))
        {
            auto s2 = ToString(-1);
            auto s1 = ToString(-2);
            stack().Pop();
            stack().Pop();
            stack().Push(s1 + s2);
        }
        else
        {
            auto b = stack().Pop();
            auto a = stack().Pop();
            if (auto [res, ok] = CallMetamethod(a, b, str::CONCAT); ok)
            {
                stack().Push(std::move(res));
            }
            else
            {
                // TODO error
            }
        }
    }
}

bool lua::State::ToBoolean(int32_t idx)
{
    return stack().Get(idx).ConvertToBoolean();
}

void lua::State::CheckStack(int32_t n)
{
    stack().Check(n);
}

uint8_t lua::State::Type(int32_t idx)
{
    if (stack().IsValid(idx))
    {
        return stack().Get(idx).TypeOf();
    }

    return type::NONE;
}

bool lua::State::IsString(int32_t idx)
{
    auto t = Type(idx);
    return type::STRING == t || type::NUMBER == t;
}

std::string lua::State::ToString(int32_t idx)
{
    return std::move(ToStringX(idx).first);
}

std::pair<std::string, bool> lua::State::ToStringX(int32_t idx)
{
    auto val = stack().Get(idx);
    return std::visit(
        [](auto&& arg) -> std::pair<std::string, bool>
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int64_t>)
                return {std::to_string(arg), true};
            else if constexpr (std::is_same_v<T, std::string>)
            {
                std::ostringstream os;
                os << std::setprecision(15) << arg;
                auto s = os.str();
                if (std::find_if(s.begin(), s.end(), [](char c) { return c == '.' || c == 'e'; }) == s.end())
                {
                    s += ".0";
                }
                return {std::move(s), true};
            }
            else if constexpr (std::is_same_v<T, std::string>)
                return {std::move(arg), true};
            else
                return {};
        },
        val);
}

std::pair<Value, bool> lua::State::CallMetamethod(Value a, Value b, const char* mmName)
{
    auto mm = a.GetMetafield(mmName, this);
    if (!mm)
    {
        mm = b.GetMetafield(mmName, this);
        if (!mm)
        {
            return {};
        }
    }
    stack().Check(4);
    stack().Push(std::move(*mm));
    stack().Push(std::move(a));
    stack().Push(std::move(b));
    Call(2, 1);
    return {stack().Pop(), true};
}

void lua::State::CallLuaClosure(int32_t nArgs, int32_t nRes, Closure* c)
{
    auto nRegs = c->proto->MaxStackSize;
    auto nParams = c->proto->NumParams;
    bool isVararg = c->proto->IsVararg;

    auto& newStack = PushLuaStack(cv::MINSTACK + nRegs, this);
    newStack.closure = c;
    auto funcAndArgs = stack().PopN(nArgs + 1);
    newStack.PushN(funcAndArgs, nParams, 1);
    newStack.top = nRegs;  // to max
    if (nArgs > nParams && isVararg)
    {
        newStack.varargs = std::vector(std::make_move_iterator(funcAndArgs.begin() + nParams),
                                       std::make_move_iterator(funcAndArgs.end()));
    }

    RunLuaClosure();
    auto res = PopLuaStack();
    if (nRes)
    {
        auto results = newStack.PopN(newStack.top - nRegs);
        stack().Check(results.size());
        stack().PushN(results, nRes);
    }
}

void lua::State::RunLuaClosure()
{
    while (1)
    {
        Instruction inst = Fetch();
        inst.Execute(this);
        if (inst.Opcode() == Op::RETURN)
        {
            return;
        }
    }
}

uint8_t lua::State::GetTable(const Value& t, const Value& k, bool raw)
{
    if (t.IsTable())
    {
        auto tbl = std::get<Table*>(t);
        auto v = tbl->Get(k);
        if (raw || v || !tbl->HasMetafield(str::INDEX))
        {
            stack().Push(v ? *v : nullptr);
            return v->TypeOf();
        }
    }
    if (!raw)
    {
        if (auto mf = t.GetMetafield(str::INDEX, this))
        {
            bool right = true;
            uint8_t res = std::visit(
                [&](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, Table*>)
                        return GetTable(*mf, k, false);
                    else if constexpr (std::is_same_v<T, Closure*>)
                    {
                        stack().Push(*mf);
                        stack().Push(t);
                        stack().Push(k);
                        Call(2, 1);
                        auto v = stack().Get(-1);
                        return v.TypeOf();
                    }
                    else
                    {
                        right = false;
                        return uint8_t{};
                    }
                },
                *mf);
            if (right)
            {
                return res;
            }
        }
    }

    // TODO error
    return 0;
}

void lua::State::SetTable(const Value& t, const Value& k, Value v, bool raw)
{
    if (t.IsTable())
    {
        auto tbl = std::get<Table*>(t);
        auto old = tbl->Get(k);
        if (raw || old || !tbl->HasMetafield(str::NEWINDEX))
        {
            tbl->Put(Value{k}, std::move(v));
            return;
        }
    }
    if (!raw)
    {
        if (auto mf = t.GetMetafield(str::NEWINDEX, this))
        {
            bool right = true;
            std::visit(
                [&](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, Table*>)
                        SetTable(*mf, k, std::move(v), false);
                    else if constexpr (std::is_same_v<T, Closure*>)
                    {
                        stack().Push(*mf);
                        stack().Push(t);
                        stack().Push(k);
                        stack().Push(std::move(v));
                        Call(3, 0);
                    }
                    else
                    {
                        right = false;
                    }
                },
                *mf);
            if (right)
            {
                return;
            }
        }
    }

    // TODO error
}
