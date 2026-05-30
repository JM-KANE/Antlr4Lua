#include "Operator.h"
#include "VirtualMachine.h"
#include "Instruction.h"
#include <iomanip>
#include "stdlib/stdlib.h"
#include "State.h"
#include <format>
#include "../code_gen/CodeGen.h"

using namespace lua;

lua::State::State(VirtualMachine* _vm, Table& reg) : registry(reg), vm(_vm)
{
}

void lua::State::Mark(std::vector<Value>& grey)
{
    if (!color)
    {
        color = 1;
        grey.emplace_back(this);
    }
}

void lua::State::MarkChildren(std::vector<Value>& grey)
{
    registry.Mark(grey);
    for (auto&& stk : stacks)
    {
        stk->Mark(grey);
    }

    // thread
}

std::ostream& lua::State::Out() const
{
    return *vm->out;
}

std::ostream& lua::State::Err() const
{
    return *vm->err;
}

TStatus lua::State::Load(const std::string& data, const std::string& chunkName, std::string_view mode)
{
    auto& p = vm->proto;
    if (false)
    {
        // TODO binary chunk
    }
    else
    {
        CodeGen cg;
        cg.Generate(data, p);
    }
    auto& c = vm->NewLuaClosure(p);
    stack().Push(&c);
    if (!p.Upvalues.empty())
    {
        auto& env = *registry.Get(cv::RIDX_GLOBALS);
        c.upvals.front() = std::make_unique<Upvalue>(env);
    }

    GetGlobal("args");
    auto nArgs = Len2(-1);
    for (int64_t i = 1; i <= nArgs; i++)
    {
        PushInteger(i);
        GetTable(2);
    }
    Remove(2);
    return TStatus::OK;
}

TStatus lua::State::LoadFile(std::string_view filename)
{
    return LoadFileX(filename, "bt");
}

TStatus lua::State::LoadFileX(std::string_view filename, std::string_view mode)
{
    std::ifstream ifs(filename.data(), std::ios_base::binary);
    if (ifs)
    {
        std::string data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return Load(data, "@" + std::string(filename), mode);
    }
    return TStatus::ERRFILE;
}

void lua::State::OpenLibs()
{
    constexpr FuncReg<8> A{
        pair_type{"_G", stdlib::OpenBaseLib}, {"math", stdlib::OpenMathLib},           {"table", stdlib::OpenTableLib},
        {"string", stdlib::OpenStringLib},    {"utf8", stdlib::OpenUTF8Lib},           {"os", stdlib::OpenOSLib},
        {"package", stdlib::OpenPackageLib},  {"coroutine", stdlib::OpenCoroutineLib},
    };

    for (auto&& [name, f] : A)
    {
        RequireF(name, f, true);
        Pop(1);
    }
}

Table* lua::State::GetArgs()
{
    return vm->GetArgs();
}

void lua::State::RequireF(const char* modname, Function openf, bool glb)
{
    GetSubTable(cv::REGISTRYINDEX, "_LOADED");
    GetField(-1, modname);
    if (!ToBoolean(-1))
    {
        Pop(1);
        PushFunction(openf);
        PushString(modname);
        Call(1, 1);
        PushValue(-1);
        SetField(-3, modname);
    }
    Remove(-2);
    if (glb)
    {
        PushValue(-1);
        SetGlobal(modname);
    }
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

void lua::State::PushFunction(Function f)
{
    auto& c = vm->NewFuncClosure(f, 0);
    stack().Push(&c);
}

void lua::State::PushFuncClosure(Function f, int32_t n)
{
    auto& closure = vm->NewFuncClosure(f, n);
    for (auto i = n; i > 0; i--)
    {
        auto val = stack().Pop();
        closure.upvals[i - 1] = std::make_shared<Upvalue>(std::move(val));
    }
    stack().Push(&closure);
}

Stack& lua::State::PushLuaStack(std::unique_ptr<Stack>&& stk)
{
    auto& stackPrev = stack();
    auto& stack = stacks.emplace_back(std::move(stk));
    stack->prev = &stackPrev;
    return *stack;
}

Stack& lua::State::PushLuaStack(size_t size, State* st)
{
    return PushLuaStack(std::make_unique<Stack>(size, st));
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

int32_t lua::State::AbsIndex(int32_t idx)
{
    return (int32_t)stack().AbsIndex(idx);
}

void lua::State::PushNil()
{
    stack().Push(nullptr);
}

void lua::State::PushBoolean(bool b)
{
    stack().Push(b);
}

void lua::State::PushInteger(int64_t n)
{
    stack().Push(n);
}

void lua::State::PushNumber(double f)
{
    stack().Push(f);
}

void lua::State::PushString(std::string s)
{
    stack().Push(std::move(s));
}

void lua::State::PushAny(Value v)
{
    stack().Push(std::move(v));
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

void lua::State::PushGlobalTable()
{
    auto g = registry.Get(cv::RIDX_GLOBALS);
    stack().Push(**g);
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

void lua::State::LoadVararg(int32_t n)
{
    if (n < 0)
        n = (int32_t)stack().varargs.size();
    stack().Check(n);
    stack().PushN(stack().varargs, n);
}

void lua::State::LoadProto(int32_t idx)
{
    auto& stk = stack();
    auto subProto = stk.closure->proto->Protos[(size_t)idx];
    auto& closure = vm->NewLuaClosure(subProto);
    stk.Push(&closure);
    for (size_t i = 0; i < subProto.Upvalues.size(); i++)
    {
        auto& uvInfo = subProto.Upvalues[i];
        auto uvIdx = uvInfo.Idx;
        if (1 == uvInfo.Instack)
        {
            auto [it, ok] = stk.openuvs.try_emplace(uvIdx, nullptr);
            if (ok)
            {
                it->second = std::make_shared<Upvalue>(stk.slots[uvIdx]);
            }
            closure.upvals[i] = it->second;
        }
        else
        {
            closure.upvals[i] = stk.closure->upvals[uvIdx];
        }
    }
}

size_t lua::State::GetTop()
{
    return stack().top;
}

int32_t lua::State::RegisterCount()
{
    return stack().closure->proto->MaxStackSize;
}

uint8_t lua::State::GetTable(int32_t idx)
{
    auto t = stack().Get(idx);
    auto k = stack().Pop();
    return GetTable(t, *k, false);
}

uint8_t lua::State::GetField(int32_t idx, std::string k)
{
    auto t = stack().Get(idx);
    return GetTable(t, k, false);
}

bool lua::State::GetSubTable(int32_t idx, std::string fname)
{
    if (GetField(idx, fname) == type::TABLE)
    {
        return true;
    }
    Pop(1);
    idx = stack().AbsIndex(idx);
    NewTable();
    PushValue(-1);
    SetField(idx, std::move(fname));
    return false;
}

uint8_t lua::State::GetGlobal(const std::string& name)
{
    auto t = registry.Get(cv::RIDX_GLOBALS);
    return GetTable(**t, name, false);
}

uint8_t lua::State::GetMetafield(int32_t idx, const std::string_view& sv)
{
    if (!GetMetatable(idx))
    {
        return type::NIL;
    }
    PushString(std::string(sv));
    auto tt = RawGet(-2);
    tt == type::NIL ? Pop(2) : Remove(-2);
    return tt;
}

bool lua::State::GetMetatable(int32_t idx)
{
    auto val = stack().Get(idx);
    if (auto mt = val.GetMetatable(this))
    {
        stack().Push(mt);
        return true;
    }
    return false;
}

bool lua::State::GetI(int32_t idx, int64_t i)
{
    auto t = stack().Get(idx);
    return GetTable(t, i, false);
}

uint8_t lua::State::RawGet(int32_t idx)
{
    auto t = stack().Get(idx);
    auto k = stack().Pop();
    return GetTable(t, *k, true);
}

void lua::State::SetTable(int32_t idx)
{
    auto t = stack().Get(idx);
    auto v = stack().Pop();
    auto k = stack().Pop();
    SetTable(t, *k, std::move(*v), false);
}

void lua::State::SetField(int32_t idx, std::string k)
{
    auto t = stack().Get(idx);
    auto v = stack().Pop();
    SetTable(t, k, std::move(*v), false);
}

void lua::State::SetMetatable(int32_t idx)
{
    auto val = stack().Get(idx);
    auto mtVal = stack().Pop();
    if (mtVal->IsNil())
    {
        val.SetMetatable(nullptr, this);
    }
    else if (mtVal->IsTable())
    {
        val.SetMetatable(std::get<Table*>(*mtVal), this);
    }
    else
    {
        // TODO error
    }
}

void lua::State::SetGlobal(std::string k)
{
    auto& t = *registry.Get(cv::RIDX_GLOBALS);
    auto v = stack().Pop();
    SetTable(*t, k, std::move(*v), false);
}

void lua::State::SetI(int32_t idx, int64_t i)
{
    auto t = stack().Get(idx);
    auto v = stack().Pop();
    SetTable(t, i, std::move(*v), false);
}

void lua::State::RawSet(int32_t idx)
{
    auto t = stack().Get(idx);
    auto v = stack().Pop();
    auto k = stack().Pop();
    SetTable(t, *k, std::move(*v), true);
}

void lua::State::CreateTable(int32_t nArr, int32_t nRec)
{
    auto& t = vm->NewLuaTable(size_t(nArr), size_t(nRec));
    stack().Push(&t);
}

void lua::State::NewTable()
{
    CreateTable(0, 0);
}

void lua::State::Copy(int32_t fromIdx, int32_t toIdx)
{
    auto val = stack().Get(fromIdx);
    stack().Set(toIdx, std::move(val));
}

void lua::State::Replace(int32_t idx)
{
    stack().Set(idx, std::move(*stack().Pop()));
}

void lua::State::AddPC(int32_t n)
{
    stack().pc += n;
}

void lua::State::CloseUpvalues(int32_t n)
{
    auto& ovs = stack().openuvs;
    for (auto it = ovs.begin(); it != ovs.end();)
    {
        auto&& [i, openuv] = *it;
        if (i + 1 >= n)
        {
            // auto val = std::make_shared<Value>(std::move(*openuv->val));
            // openuv->val = std::move(val);
            vm->AddClosed(openuv->val);
            it = ovs.erase(it);
        }
        else
            ++it;
    }
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
            CallFuncClosure(nArgs, nRes, c);
        }
    }
}

TStatus lua::State::PCall(int32_t nArgs, int32_t nRes, int32_t msgh)
{
    auto& caller = stack();
    auto status = TStatus::ERRRUN;
    try
    {
        Call(nArgs, nRes);
        status = TStatus::OK;
    }
    catch (const std::exception& e)
    {
        if (msgh)
        {
            // TODO Get(msg), handle error
        }
        while (&stack() != &caller)
        {
            PopLuaStack();
        }
        stack().Push(e.what());
    }
    return status;
}

bool lua::State::CallMeta(int obj, std::string_view event)
{
    obj = AbsIndex(obj);
    if (GetMetafield(obj, event) == type::NIL)
    {
        return false;
    }
    PushValue(obj);
    Call(1, 1);
    return true;
}

bool lua::State::RawEqual(int32_t idx1, int32_t idx2)
{
    if (stack().IsValid(idx1) && stack().IsValid(idx2))
    {
        auto a = stack().Get(idx1);
        auto b = stack().Get(idx2);
        return 1 == std::visit(op::Eq(), a, b);
    }
    return false;
}

int32_t lua::State::Error()
{
    auto v = stack().Pop();
    // TODO error
    return 0;
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

int64_t lua::State::Len2(int32_t idx)
{
    Len(idx);
    auto [i, ok] = ToIntegerX(-1);
    if (!ok)
        Error2("object length is not an integer");
    Pop(1);
    return i;
}

size_t lua::State::RawLen(int32_t idx)
{
    auto val = stack().Get(idx);
    if (val.IsString())
    {
        return std::get<std::string>(val).size();
    }
    else if (val.IsTable())
    {
        auto tbl = std::get<Table*>(val);
        return tbl->Len();
    }
    return size_t();
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
            if (auto [res, ok] = CallMetamethod(std::move(*a), std::move(*b), str::CONCAT); ok)
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

bool lua::State::CheckStack(int32_t n)
{
    stack().Check(n);
    return true;
}

void lua::State::CheckStack2(int32_t n, const char* msg)
{
    if (!CheckStack(n))
    {
        if (msg)
        {
            Error2("stack overflow (%s)", msg);
        }
        else
        {
            Error2("stack overflow");
        }
    }
}

void lua::State::SetTop(int32_t idx)
{
    auto newTop = stack().AbsIndex(idx);
    if (newTop < 0)
    {
        // TODO error
    }

    auto n = stack().top - newTop;
    if (n > 0)
    {
        for (size_t i = 0; i < n; i++)
        {
            stack().Pop();
        }
    }
    else if (n < 0)
    {
        for (int32_t i = 0; i > n; i--)
        {
            stack().Push(nullptr);
        }
    }
}

void lua::State::Remove(int32_t idx)
{
    Rotate(idx, -1);
    Pop(1);
}

const char* lua::State::TypeName(uint8_t tp)
{
    switch (tp)
    {
    case type::NIL:
        return "nil";
    case type::BOOLEAN:
        return "boolean";
    case type::NUMBER:
        return "number";
    case type::STRING:
        return "string";
    case type::TABLE:
        return "table";
    case type::FUNCTION:
        return "function";
    case type::THREAD:
        return "thread";
    case type::NONE:
        return "no value";
    default:
        return "userdata";
    }
}

const char* lua::State::TypeName2(int32_t idx)
{
    return TypeName(Type(idx));
}

uint8_t lua::State::Type(int32_t idx)
{
    if (stack().IsValid(idx))
    {
        return stack().Get(idx).TypeOf();
    }

    return type::NONE;
}

bool lua::State::IsNil(int32_t idx)
{
    return Type(idx) == type::NIL;
}

bool lua::State::IsNone(int32_t idx)
{
    return Type(idx) == type::NONE;
}

bool lua::State::IsNoneOrNil(int32_t idx)
{
    auto t = Type(idx);
    return t == type::NIL || t == type::NONE;
}

bool lua::State::IsFloat(int32_t idx)
{
    return ToFloatX(idx).second;
}

bool lua::State::ToBoolean(int32_t idx)
{
    return stack().Get(idx).ConvertToBoolean();
}

int64_t lua::State::ToInteger(int32_t idx)
{
    return ToIntegerX(idx).first;
}

std::pair<int64_t, bool> lua::State::ToIntegerX(int32_t idx)
{
    return stack().Get(idx).ConvertToInteger();
}

double lua::State::ToFloat(int32_t idx)
{
    return ToFloatX(idx).first;
}

std::pair<double, bool> lua::State::ToFloatX(int32_t idx)
{
    return stack().Get(idx).ConvertToFloat();
}

Value lua::State::ToNumber(int32_t idx)
{
    auto val = stack().Get(idx).ConvertToNumber();
    auto i = val.index();
    return 3 == i || 2 == i ? std::move(val) : 0;
}

bool lua::State::StringToNumber(std::string s)
{
    Value val(std::move(s));
    auto newVar = val.ConvertToNumber();
    if (!newVar.IsNil())
    {
        PushAny(std::move(newVar));
        return true;
    }
    return false;
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

std::string lua::State::ToString2(int32_t idx)
{
    if (CallMeta(idx, str::TOSTRING))
    {
        if (!IsString(-1))
        {
            Error2((str::TOSTRING + " must return a string"s).c_str());
        }
    }
    else
        switch (Type(idx))
        {
        case type::STRING:
        case type::NUMBER:
            PushString(std::move(ToStringX(idx).first));
            break;
        case type::BOOLEAN:
            PushString(ToBoolean(idx) ? "true" : "false");
            break;
        case type::NIL:
            PushString("nil");
            break;
        default:
        {
            auto tt = GetMetafield(idx, str::NAME);
            auto kind = type::STRING == tt ? CheckString(-1) : TypeName2(idx);
            PushString(std::format("{0} {1:p}", kind, ToPointer(idx)));
            if (type::STRING != tt)
                Remove(-2);
        }
        break;
        }
    return CheckString(-1);
}

int64_t lua::State::OptInteger(int32_t idx, int64_t def)
{
    return IsNoneOrNil(idx) ? def : CheckInteger(idx);
}

std::string lua::State::OptString(int32_t idx, std::string_view def)
{
    return IsNoneOrNil(idx) ? std::string(def) : CheckString(idx);
}

void* lua::State::ToPointer(int32_t idx)
{
    auto val = stack().Get(idx);
    return std::visit(
        [](auto&& arg) -> void*
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Table*> || std::is_same_v<T, Closure*> || std::is_same_v<T, State*>)
                return arg;
            else
                return &arg;
        },
        val);
}

int64_t lua::State::CheckInteger(int32_t idx)
{
    auto [i, ok] = ToIntegerX(idx);
    if (!ok)
        IntError(idx);
    return i;
}

std::string lua::State::CheckString(int32_t idx)
{
    auto [s, ok] = ToStringX(idx);
    if (!ok)
        TagError(idx, type::STRING);
    return s;
}

void lua::State::CheckAny(int32_t idx)
{
    if (Type(idx) == type::NONE)
    {
        ArgError(idx, "value expected");
    }
}

void lua::State::CheckType(int32_t arg, uint8_t t)
{
    if (Type(arg) != t)
        TagError(arg, t);
}

int32_t lua::State::ArgError(int32_t idx, const std::string_view& msg)
{
    return Error2("bad argument #%d (%s)", idx, msg.data());
}

void lua::State::ArgCheck(bool cond, int32_t arg, const std::string_view& extraMsg)
{
    if (!cond)
        ArgError(arg, extraMsg);
}

bool lua::State::Next(int32_t idx)
{
    auto val = stack().Get(idx);
    if (val.IsTable())
    {
        auto t = std::get<Table*>(val);
        auto k = stack().Pop();
        if (auto nextKey = t->NextKey(*k))
        {
            stack().Push(*nextKey);
            stack().Push(*t->Get(*nextKey));
            return true;
        }
    }
    return false;
}

void lua::State::CollectGarbage()
{
    vm->CollectGarbage();
}

void lua::State::CheckGC()
{
    vm->CheckGC();
}

void lua::State::Barrier(const Value& parent, const Value& child)
{
    if (vm->gc.state == Collector::State::PROPAGATE && 2 == parent.Color() && 0 == child.Color())
    {
        child.Mark(vm->gc.grey);
    }
}

std::pair<ValuePtr, bool> lua::State::CallMetamethod(Value a, Value b, const char* mmName)
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

    auto& oldStack = stack();
    auto newStack = std::make_unique<Stack>(cv::MINSTACK + nRegs, this);
    newStack->closure = c;
    auto funcAndArgs = oldStack.PopN(nArgs + 1);
    newStack->PushN(funcAndArgs, nParams, 1);
    newStack->top = nRegs;  // to max
    if (nArgs > nParams && isVararg)
    {
        newStack->varargs = std::vector(std::make_move_iterator(funcAndArgs.begin() + nParams),
                                        std::make_move_iterator(funcAndArgs.end()));
    }

    PushLuaStack(std::move(newStack));
    RunLuaClosure();
    newStack = PopLuaStack();
    if (nRes)
    {
        auto results = newStack->PopN(newStack->top - nRegs);
        stack().Check(results.size());
        stack().PushN(results, nRes);
    }
}

void lua::State::CallFuncClosure(int32_t nArgs, int32_t nRes, Closure* c)
{
    auto& oldStack = stack();
    auto newStack = std::make_unique<Stack>(cv::MINSTACK + nArgs, this);
    newStack->closure = c;
    if (nArgs > 0)
    {
        auto args = oldStack.PopN(nArgs);
        newStack->PushN(args, nArgs);
    }
    oldStack.Pop();

    PushLuaStack(std::move(newStack));
    auto r = c->func(this);
    newStack = PopLuaStack();
    if (nRes)
    {
        auto results = newStack->PopN(r);
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
        CheckGC();
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
            stack().Push(v ? **v : nullptr);
            return (*v)->TypeOf();
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
            Barrier(t, k);
            Barrier(t, v);
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

void lua::State::IntError(int32_t idx)
{
    if (IsFloat(idx))
        ArgError(idx, "number has no integer representation");
    else
        TagError(idx, type::NUMBER);
}

void lua::State::TagError(int32_t idx, uint8_t tag)
{
    TypeError(idx, TypeName(tag));
}

int32_t lua::State::TypeError(int32_t idx, const std::string_view& tname)
{
    std::string typeArg;
    if (GetMetafield(idx, str::NAME) == type::STRING)
    {
        typeArg = ToString(-1);
    }
    else if (Type(idx) == type::LIGHTUSERDATA)
    {
        typeArg = "light userdata";
    }
    else
    {
        typeArg = TypeName2(idx);
    }
    auto msg = std::string(tname) + (" expected, got " + typeArg);
    auto res = ArgError(idx, msg);
    PushString(std::move(msg));
    return res;
}
