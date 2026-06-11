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

std::istream& lua::State::In() const
{
    return *vm->in;
}

TStatus lua::State::Load(const std::string& data, std::string chunkName, std::string_view mode)
{
    auto topInfo = std::make_unique<TopPrototype>();
    auto& p = *topInfo;
    p.Source = std::move(chunkName);
    TStatus st{};
    if (false)
    {
        // TODO binary chunk
    }
    else
    {
        CodeGen cg;
        st = cg.Generate(data, p);
    }
    if (st != TStatus{})
    {
        if (TStatus::LUA_ERRSYNTAX == st)
        {
            MakeError<SyntaxException>(&p, std::move(*p.err));
        }
        return st;
    }

    auto& c = vm->NewLuaClosure(p);
    c.topInfo = std::move(topInfo);
    stack().Push(&c);
    return st;
}

std::pair<TStatus, bool> lua::State::LoadStream(const std::string& data)
{
    auto topInfo = std::make_unique<TopPrototype>();
    auto& p = *topInfo;
    p.Source = "=stdin";
    CodeGen cg;
    auto st = cg.GenerateREPL(data, p);
    if (st.first != TStatus{})
    {
        if (TStatus::LUA_ERRSYNTAX == st.first)
        {
            MakeError<SyntaxException>(&p, std::move(*p.err));
        }
        return st;
    }
    auto& c = vm->NewLuaClosure(p);
    c.topInfo = std::move(topInfo);
    stack().Push(&c);
    return st;
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
    MakeError<FileException>(std::string(filename));
    return TStatus::LUA_ERRFILE;
}

void lua::State::SetEnv(int32_t idx)
{
    auto c = ToFunction(-1);
    if (!c->proto->Upvalues.empty())
    {
        ValuePtr valPtr;
        if (0 == idx)
        {
            valPtr = *registry.Get(cv::LUA_RIDX_GLOBALS);
        }
        else
        {
            auto val = stack().Get(idx);
            valPtr = std::make_shared<Value>(std::move(val));
        }
        c->upvals.front() = std::make_unique<Upvalue>(std::move(valPtr));
        c->upvals.front()->closed = idx;
    }
}

void lua::State::OpenLibs()
{
    constexpr FuncReg<8> A{
        Reg{"_G", stdlib::OpenBaseLib},      {"math", stdlib::OpenMathLib},           {"table", stdlib::OpenTableLib},
        {"string", stdlib::OpenStringLib},   {"utf8", stdlib::OpenUTF8Lib},           {"os", stdlib::OpenOSLib},
        {"package", stdlib::OpenPackageLib}, {"coroutine", stdlib::OpenCoroutineLib},
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

void lua::State::RequireF(const char* modname, Function* openf, bool glb)
{
    GetSubTable(cv::LUA_REGISTRYINDEX, str::LUA_LOADED_TABLE);
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
    auto top = stack().top;
    if (top <= 1)
        return;
    auto t = stack().top - 1;
    auto p = stack().AbsIndex(idx) - 1;
    int64_t m = n >= 0 ? t - n : p - n - 1;
    stack().Reverse(p, m);
    stack().Reverse(m + 1, t);
    stack().Reverse(p, t);
}

void lua::State::Insert(int64_t idx)
{
    Rotate(idx, 1);
}

void lua::State::PushFunction(Function* f)
{
    auto& c = vm->NewFuncClosure(f, 0);
    stack().Push(&c);
}

void lua::State::PushFuncClosure(Function* f, int32_t n)
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
    auto stackPrev = stacks.empty() ? nullptr : &stack();
    auto& stack = stacks.emplace_back(std::move(stk));
    stack->prev = stackPrev;
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

std::pair<uint32_t, bool> lua::State::CurrentLine(uint32_t level) const
{
    if (level)
        return {};

    auto stk = &stack();
    size_t i = 1;
    for (; i < level && stk->prev; i++)
    {
        stk = stk->prev;
    }
    if (i < level)
        return {};

    return {stk->CurrentLine(), true};
}

std::pair<uint32_t, const TopPrototype*> lua::State::Where(size_t level) const
{
    // if (!level)
    //     return {};
    auto stk = &stack();
    const Prototype* proto{};
    uint32_t line{};
    size_t i = 0;
    while (1)
    {
        if (stk->closure)
        {
            if (auto protoC = stk->closure->proto)
            {
                ++i;
                proto = protoC;
                if (i == level)
                {
                    line = stk->CurrentLine();
                    break;
                }
                else if (i > level)  // level == 0
                {
                    break;
                }
            }
        }
        if (auto prev = stk->prev)
        {
            stk = prev;
            continue;
        }
        else
            break;
    }
    if (!proto)
    {
        throw "internal error";
    }
    return {line, proto->Top()};
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
    auto g = registry.Get(cv::LUA_RIDX_GLOBALS);
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
    const auto& vars = stack().varargs;
    stack().PushN(vars, n);
}

void lua::State::LoadProto(int32_t idx, int32_t r)
{
    auto& stk = stack();
    auto& subProto = stk.closure->proto->Protos[(size_t)idx];
    auto& closure = vm->NewLuaClosure(subProto);
    stk.Push(&closure);
    Replace(r);
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
    return stack().RegisterCount();
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
    if (GetField(idx, fname) == cv::type::LUA_TTABLE)
    {
        return true;
    }
    Pop(1);
    idx = (int32_t)stack().AbsIndex(idx);
    NewTable();
    PushValue(-1);
    SetField(idx, std::move(fname));
    return false;
}

uint8_t lua::State::GetGlobal(const std::string& name)
{
    auto t = registry.Get(cv::LUA_RIDX_GLOBALS);
    return GetTable(**t, name, false);
}

uint8_t lua::State::GetMetafield(int32_t idx, const std::string_view& sv)
{
    if (!GetMetatable(idx))
    {
        return cv::type::LUA_TNIL;
    }
    PushString(std::string(sv));
    auto tt = RawGet(-2);
    tt == cv::type::LUA_TNIL ? Pop(2) : Remove(-2);
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

uint8_t lua::State::RawGetI(int32_t idx, int64_t i)
{
    auto t = stack().Get(idx);
    return GetTable(t, i, true);
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
    else if (!exception)
    {
        Error2("attempt to set metatable of a %s value", TypeName(val.TypeOf()));
    }
}

void lua::State::SetGlobal(std::string k)
{
    auto& t = *registry.Get(cv::LUA_RIDX_GLOBALS);
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

void lua::State::RawSetI(int32_t idx, int64_t i)
{
    auto t = stack().Get(idx);
    auto v = stack().Pop();
    SetTable(t, i, std::move(*v), true);
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
            auto newVar = std::make_unique<Value>(std::move(*openuv->val));
            openuv->closed = true;
            openuv->val = std::move(newVar);
            it = ovs.erase(it);
        }
        else
            ++it;
    }
}

void lua::State::Call(int32_t nArgs, int32_t nRes)
{
    DoCall(nArgs, nRes, false);
}

void lua::State::TailCall(int32_t nArgs)
{
    DoCall(nArgs, -1, true);
}

void lua::State::DoCall(int32_t nArgs, int32_t nRes, bool tail)
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

    if (!c)
    {
        Error2("attempt to call a %s value", TypeName(val.TypeOf()));
        Pop(nArgs + 1);
        if (nRes > 0)
        {
            stack().Check(nRes);
            stack().RepairNullptr(nRes);
        }
    }
    else if (c->proto)
    {
        CallLuaClosure(nArgs, nRes, c, tail);
    }
    else
    {
        CallFuncClosure(nArgs, nRes, c, tail);
    }
}

TStatus lua::State::PCall(int32_t nArgs, int32_t nRes, int32_t msgh)
{
    auto& caller = stack();
    auto c = msgh ? stack().Pop() : nullptr;
    Call(nArgs, nRes);
    std::ostringstream os;
    auto st = Catch(os);
    if (st != TStatus::LUA_OK)
    {
        // while (&stack() != &caller)
        // {
        //     PopLuaStack();
        // }

        auto msg = os.str();
        if (msgh)
        {
            PushAny(std::move((*c)));
            Insert(1);
            SetTop(1);
            PushString(std::move(msg));
            Call(1, -1);
            if (exception && exception->Status() != TStatus::LUA_ERRERR)
            {
                MakeError<ErrorException>(exception->ToString());
            }
        }
        else
        {
            SetTop(1);
            stack().Set(1, std::move(msg));
        }
    }
    return st;
}

bool lua::State::CallMeta(int obj, std::string_view event)
{
    obj = AbsIndex(obj);
    if (GetMetafield(obj, event) == cv::type::LUA_TNIL)
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

void lua::State::Throw()
{
    if (exception)
    {
        if (!vm->REPL())
            Err() << vm->argv[0] << ": ";
        status = Catch(Err());
        Err() << std::endl;
    }
}

TStatus lua::State::Catch(std::ostream& os)
{
    if (exception)
    {
        auto st = exception->Status();
        os << exception->ToString();
        exception.reset();
        return st;
    }
    return status;
}

int32_t lua::State::Error()
{
    if (exception)
    {
        return 0;
    }

    auto lv = (size_t)ToInteger(-1);
    Pop(1);
    auto msg = ToString(-1);
    Pop(1);

    auto [line, proto] = Where(lv);
    MakeError<RunException>(proto, line, std::move(msg));
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
            stack().Push(int64_t(-1));
            Error2("attempt to get length of a %s value", TypeName(val.TypeOf()));
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
                stack().Push("");
                if (auto bt = b->TypeOf(); bt != cv::type::LUA_TSTRING)
                {
                    Error2("attempt to concatenate a %s value", TypeName(bt));
                }
                else if (auto at = a->TypeOf(); at != cv::type::LUA_TSTRING)
                {
                    Error2("attempt to concatenate a %s value", TypeName(at));
                }
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
        throw "invalid stack top size";
    }

    auto top = stack().top;
    if (top > newTop)
    {
        for (size_t i = 0; i < top - newTop; i++)
        {
            stack().Pop();
        }
    }
    else if (top < newTop)
    {
        for (size_t i = 0; i < newTop - top; i++)
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
    case cv::type::LUA_TNIL:
        return "nil";
    case cv::type::LUA_TBOOLEAN:
        return "boolean";
    case cv::type::LUA_TNUMBER:
        return "number";
    case cv::type::LUA_TSTRING:
        return "string";
    case cv::type::LUA_TTABLE:
        return "table";
    case cv::type::LUA_TFUNCTION:
        return "function";
    case cv::type::LUA_TUSERDATA:
        return "thread";
    case cv::type::LUA_TNONE:
        return "no value";
    default:
        return "userdata";
    }
}

const char* lua::State::TypeName2(int32_t idx)
{
    return TypeName(Type(idx));
}

uint8_t lua::State::Type(int32_t idx) const
{
    if (stack().IsValid(idx))
    {
        return stack().Get(idx).TypeOf();
    }

    return cv::type::LUA_TNONE;
}

bool lua::State::IsNil(int32_t idx)
{
    return Type(idx) == cv::type::LUA_TNIL;
}

bool lua::State::IsNone(int32_t idx)
{
    return Type(idx) == cv::type::LUA_TNONE;
}

bool lua::State::IsNoneOrNil(int32_t idx)
{
    auto t = Type(idx);
    return t == cv::type::LUA_TNIL || t == cv::type::LUA_TNONE;
}

bool lua::State::IsFloat(int32_t idx)
{
    return ToFloatX(idx).second;
}

bool lua::State::IsBoolean(int32_t idx)
{
    return Type(idx) == cv::type::LUA_TBOOLEAN;
}

bool lua::State::IsFunction(int32_t idx) const
{
    return Type(idx) == cv::type::LUA_TFUNCTION;
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
    return cv::type::LUA_TSTRING == t || cv::type::LUA_TNUMBER == t;
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
            else if constexpr (std::is_same_v<T, double>)
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
        case cv::type::LUA_TSTRING:
        case cv::type::LUA_TNUMBER:
            PushString(std::move(ToStringX(idx).first));
            break;
        case cv::type::LUA_TBOOLEAN:
            PushString(ToBoolean(idx) ? "true" : "false");
            break;
        case cv::type::LUA_TNIL:
            PushString("nil");
            break;
        default:
        {
            auto tt = GetMetafield(idx, str::NAME);
            auto kind = cv::type::LUA_TSTRING == tt ? CheckString(-1) : TypeName2(idx);
            PushString(std::format("{0}: {1:p}", kind, ToPointer(idx)));
            if (cv::type::LUA_TSTRING != tt)
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
                return nullptr;
        },
        val);
}

Closure* lua::State::ToFunction(int32_t idx) const
{
    auto val = stack().Get(idx);
    return std::visit(
        [](auto&& arg) -> Closure*
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Closure*>)
                return arg;
            else
                return nullptr;
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
        TagError(idx, cv::type::LUA_TSTRING);
    return s;
}

void lua::State::CheckAny(int32_t idx)
{
    if (Type(idx) == cv::type::LUA_TNONE)
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
        if (auto [nextKey, ok] = t->NextKey(*k); ok)
        {
            if (nextKey)
            {
                stack().Push(*nextKey);
                stack().Push(*t->Get(*nextKey));
                return true;
            }
        }
        else
            Error2("invalid key to 'next'");
    }
    else if (!exception)
    {
        Error2("attempt to get next key of a %s value", TypeName(val.TypeOf()));
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

void lua::State::CallLuaClosure(int32_t nArgs, int32_t nRes, Closure* c, bool tail)
{
    auto nRegs = c->proto->MaxStackSize;
    auto nParams = c->proto->NumParams;
    bool isVararg = c->proto->IsVararg;
    auto& oldStack = stack();
    if (tail)
    {
        oldStack.CloseUpvalues();
        if (nArgs > nParams && isVararg)
        {
            oldStack.varargs = oldStack.PopN(nArgs - nParams);
        }
        Rotate(1, nParams);
        oldStack.top = 0;
        CheckStack(cv::LUA_MINSTACK + nRegs);
        oldStack.closure = c;
        oldStack.top = nRegs;
        oldStack.pc = 0;
        RunLuaClosure();
        return;
    }

    auto newStack = std::make_unique<Stack>(cv::LUA_MINSTACK + nRegs, this);
    auto funcAndArgs = oldStack.PopN(nArgs + 1);
    if (nArgs > nParams && isVararg)
    {
        newStack->varargs = std::vector(std::make_move_iterator(funcAndArgs.begin() + nParams + 1),
                                        std::make_move_iterator(funcAndArgs.end()));
    }
    newStack->PushN(funcAndArgs, nParams, 1);
    newStack->closure = c;
    newStack->top = nRegs;

    PushLuaStack(std::move(newStack));
    RunLuaClosure();

    bool changeToFunctionTail = !stack().closure->proto;
    auto newRegs = changeToFunctionTail ? 0 : RegisterCount();
    newStack = PopLuaStack();
    if (nRes)
    {
        std::vector<ValuePtr> results;
        if (changeToFunctionTail)
        {
            auto total = newStack->Pop();
            auto nTotal = total->ConvertToInteger().first;
            results = newStack->PopN(nTotal);
        }
        else
        {
            results = newStack->PopN(newStack->top - newRegs);
        }
        stack().Check(results.size());
        stack().PushN(results, nRes);
    }
}

void lua::State::CallFuncClosure(int32_t nArgs, int32_t nRes, Closure* c, bool tail)
{
    auto& oldStack = stack();
    if (tail)
    {
        oldStack.CloseUpvalues();
        Rotate(1, nArgs);
        oldStack.top = 0;
        CheckStack(cv::LUA_MINSTACK + nArgs);
        oldStack.closure = c;
        oldStack.top = nArgs;
        oldStack.pc = 0;
        auto r = c->func(this);
        oldStack.Push(r);
        return;
    }
    auto newStack = std::make_unique<Stack>(cv::LUA_MINSTACK + nArgs, this);
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

TStatus lua::State::RunLuaClosure()
{
    while (1)
    {
        // auto line = CurrentLine();
        Instruction inst = Fetch();
        inst.Execute(this);
        if (exception)
        {
            return exception->Status();
        }
        CheckGC();
        if (exception)
        {
            return exception->Status();
        }
        if (inst.Opcode() == Op::RETURN || inst.Opcode() == Op::TAILCALL)
        {
            return TStatus::LUA_OK;
        }
    }
}

uint8_t lua::State::GetTable(const Value& t, const Value& k, bool raw)
{
    auto isTable = t.IsTable();
    if (isTable)
    {
        auto tbl = std::get<Table*>(t);
        auto v = tbl->Get(k);
        if (raw || v || !tbl->HasMetafield(str::INDEX))
        {
            auto val = v && *v ? **v : Value{};
            auto t = val.TypeOf();
            stack().Push(std::move(val));
            return t;
        }
    }
    if (!raw)
    {
        if (auto mf = t.GetMetafield(str::INDEX, this))
        {
            return std::visit(
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
                        stack().Push(nullptr);
                        return cv::type::LUA_TNIL;
                    }
                },
                *mf);
        }
    }
    if (!isTable)
    {
        Error2("attempt to index a %s value", TypeName(t.TypeOf()));
    }
    PushNil();
    return 0;
}

void lua::State::SetTable(const Value& t, const Value& k, Value v, bool raw)
{
    auto isTable = t.IsTable();
    if (isTable)
    {
        auto tbl = std::get<Table*>(t);
        auto old = tbl->Get(k);
        if (raw || old || !tbl->HasMetafield(str::NEWINDEX))
        {
            Barrier(t, k);
            Barrier(t, v);
            if (auto err = tbl->Put(Value{k}, std::move(v)))
            {
                auto msg = 1 == err ? "table index is nil" : "table index is NaN";
                Error2(msg);
            }
            return;
        }
    }
    if (!raw)
    {
        if (auto mf = t.GetMetafield(str::NEWINDEX, this))
        {
            std::visit(
                [&](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, Closure*>)
                    {
                        stack().Push(*mf);
                        stack().Push(t);
                        stack().Push(k);
                        stack().Push(std::move(v));
                        Call(3, 0);
                    }
                    else
                        SetTable(*mf, k, std::move(v), false);
                },
                *mf);
            return;
        }
    }

    if (!isTable)
    {
        Error2("attempt to index a %s value", TypeName(t.TypeOf()));
    }
}

bool lua::State::Warn() const
{
    return _warn;
}

void lua::State::SetWarn(bool b)
{
    _warn = b;
}

void lua::State::IntError(int32_t idx)
{
    if (IsFloat(idx))
        ArgError(idx, "number has no integer representation");
    else
        TagError(idx, cv::type::LUA_TNUMBER);
}

void lua::State::TagError(int32_t idx, uint8_t tag)
{
    TypeError(idx, TypeName(tag));
}

int32_t lua::State::TypeError(int32_t idx, const std::string_view& tname)
{
    std::string typeArg;
    if (GetMetafield(idx, str::NAME) == cv::type::LUA_TSTRING)
    {
        typeArg = ToString(-1);
    }
    else if (Type(idx) == cv::type::LUA_TLIGHTUSERDATA)
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

int32_t lua::State::FileResult(int stat, const char* fname)
{
    auto en = errno;
    if (stat)
    {
        PushBoolean(true);
        return 1;
    }
    PushNil();
    auto msg = en ? strerror(en) : "(no extra info)";
    PushString(fname ? std::format("{}: {}", fname, msg) : msg);
    PushInteger(en);
    return 3;
}

int32_t lua::State::ExecResult(int stat)
{
    if (stat && errno)
    {
        return FileResult(stat, nullptr);
    }
    auto what = "exit";
#ifndef _WIN32
    if (WIFEXITED(stat))
    {
        stat = WEXITSTATUS(stat);
    }
    else if (WIFSIGNALED(stat))
    {
        stat = WTERMSIG(stat);
        what = "signal";
    }
#endif
    if (*what == 'e' && !stat)
        PushBoolean(true);
    else
        PushNil();
    PushString(what);
    PushInteger(stat);
    return 3;
}
