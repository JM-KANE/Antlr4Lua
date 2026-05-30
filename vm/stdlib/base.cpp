#include "base.h"
#include "table.h"
#include "../VirtualMachine.h"
#include "../code_gen/number.h"
using namespace lua;
using namespace stdlib;
namespace
{
constexpr FuncReg<26> baseFuncs{
    pair_type{"print", base::Print},
    {"assert", base::Assert},
    {"error", base::Error},
    {"warn", base::Warn},
    {"select", base::Select},
    {"ipairs", base::IPairs},
    {"pairs", base::Pairs},
    {"next", base::Next},
    {"load", base::Load},
    {"loadfile", base::LoadFile},
    {"dofile", base::DoFile},
    {"pcall", base::PCall},
    {"xpcall", base::XPCall},
    {"getmetatable", base::GetMetatable},
    {"setmetatable", base::SetMetatable},
    {"rawequal", base::RawEqual},
    {"rawlen", base::RawLen},
    {"rawget", base::RawGet},
    {"rawset", base::RawSet},
    {"type", base::Type},
    {"tostring", base::ToString},
    {"tonumber", base::ToNumber},
    {"collectgarbage", base::CollectGarbage},
    {"unpack", table::Unpack},

    {"_G", nullptr},
    {"_VERSION", nullptr},
    // {"arg", nullptr},
};

int32_t iPairsAux(State* ls)
{
    auto i = ls->CheckInteger(2) + 1;
    ls->PushInteger(i);
    return ls->GetI(1, i) == type::NIL ? 1 : 2;
}

int32_t outStream(State* ls, std::ostream& os, const char* seg)
{
    auto n = ls->GetTop();
    ls->GetGlobal("tostring");
    for (size_t i = 1; i <= n; i++)
    {
        ls->PushValue(-1);
        ls->PushValue(1);
        ls->Call(1, 1);
        auto [s, ok] = ls->ToStringX(-1);
        if (!ok)
        {
            return ls->Error2("'tostring' must return a string");
        }
        if (i > 1)
            os << seg;
        os << s;
        ls->Pop(1);
    }
    os << std::endl;
    return 0;
}

int32_t loadAux(State* ls, TStatus status, int32_t envIdx)
{
    if (TStatus::OK == status)
    {
        if (envIdx)
        {
            // TODO env
        }
        return 1;
    }
    else
    {
        ls->PushNil();
        ls->Insert(-2);
        return 2;
    }
}

}  // namespace

int32_t lua::stdlib::OpenBaseLib(State* ls)
{
    ls->PushGlobalTable();
    ls->SetFuncs(baseFuncs, 0);
    ls->PushValue(-1);
    ls->SetField(-2, "_G");
    ls->PushString("Lua 5.3");
    ls->SetField(-2, "_VERSION");
    //ls->PushAny(ls->GetArgs());
    //ls->SetField(-2, "arg");
    return 1;
}

int32_t lua::stdlib::base::Print(State* ls)
{
    return outStream(ls, ls->Out(), "\t");
}

int32_t lua::stdlib::base::Assert(State* ls)
{
    if (ls->ToBoolean(1))
        return (int32_t)ls->GetTop();
    ls->CheckAny(1);
    ls->Remove(1);
    ls->PushString("assertion failed!");
    ls->SetTop(1);
    return base::Error(ls);
}

int32_t lua::stdlib::base::Error(State* ls)
{
    auto level = int32_t(ls->OptInteger(2, 1));
    ls->SetTop(1);
    if (ls->Type(1) == type::STRING && level > 0)
    {
        // ls->Where(level) /* add extra information */
        // ls->PushValue(1)
        // ls->Concat(2)
    }
    return ls->Error();
}

int32_t lua::stdlib::base::Warn(State* ls)
{
    return outStream(ls, ls->Err(), " ");
}

int32_t lua::stdlib::base::Select(State* ls)
{
    auto n = int64_t(ls->GetTop());
    if (ls->Type(1) == type::STRING && ls->CheckString(1) == "#")
    {
        ls->PushInteger(n - 1);
        return 1;
    }
    else
    {
        auto i = ls->CheckInteger(1);
        if (i < 0)
        {
            i = n + i;
        }
        else if (i > n)
        {
            i = n;
        }
        ls->ArgCheck(1 <= i, 1, "index out of range");
        return int(n - i);
    }
}

int32_t lua::stdlib::base::IPairs(State* ls)
{
    ls->CheckAny(1);
    ls->PushFunction(iPairsAux);
    ls->PushValue(1);
    ls->PushInteger(0);
    return 3;
}

int32_t lua::stdlib::base::Pairs(State* ls)
{
    ls->CheckAny(1);
    if (ls->GetMetafield(1, str::PAIRS) == type::NIL)
    {
        ls->PushFunction(base::Next);
        ls->PushValue(1);
        ls->PushNil();
    }
    else
    {
        ls->PushValue(1);
        ls->Call(1, 3);
    }
    return 3;
}

int32_t lua::stdlib::base::Next(State* ls)
{
    ls->CheckType(1, type::TABLE);
    ls->SetTop(2);
    if (ls->Next(1))
    {
        return 2;
    }
    else
    {
        ls->PushNil();
        return 1;
    }
}

int32_t lua::stdlib::base::Load(State* ls)
{
    TStatus status{TStatus::ERRFILE};
    auto [chunk, isStr] = ls->ToStringX(1);
    auto mode = ls->OptString(3, "bt");
    auto env = 0;
    if (!ls->IsNone(4))
    {
        env = 4;
    }
    if (isStr)
    {
        auto chunkname = ls->OptString(2, chunk);
        status = ls->Load(chunk, chunkname, mode);
    }
    else
    {
        // TODO loading from a reader function
    }
    return loadAux(ls, status, env);
}

int32_t lua::stdlib::base::LoadFile(State* ls)
{
    auto fname = ls->OptString(1, "");
    auto mode = ls->OptString(1, "bt");
    auto env = 0;
    if (!ls->IsNone(3))
    {
        env = 3;
    }
    auto status = ls->LoadFileX(fname, mode);
    return loadAux(ls, status, env);
}

int32_t lua::stdlib::base::DoFile(State* ls)
{
    auto fname = ls->OptString(1, "bt");
    ls->SetTop(1);
    if (ls->LoadFile(fname) != TStatus::OK)
    {
        return ls->Error();
    }
    ls->Call(0, cv::RIDX_GLOBALS);
    return (int32_t)ls->GetTop() - 1;
}

int32_t lua::stdlib::base::PCall(State* ls)
{
    auto nArgs = ls->GetTop() - 1;
    auto status = ls->PCall((int32_t)nArgs, -1, 0);
    ls->PushBoolean(status == TStatus::OK);
    ls->Insert(1);
    return (int32_t)ls->GetTop();
}

int32_t lua::stdlib::base::XPCall(State* ls)
{
    auto nArgs = ls->GetTop() - 2;
    ls->Rotate(2, -1);
    auto i = (int32_t)ls->CheckInteger(-1);  // TODO
    ls->Pop(1);
    auto status = ls->PCall((int32_t)nArgs, -1, i);
    ls->PushBoolean(status == TStatus::OK);
    ls->Insert(1);
    return (int32_t)ls->GetTop();
}

int32_t lua::stdlib::base::GetMetatable(State* ls)
{
    ls->CheckAny(1);
    if (!ls->GetMetatable(1))
    {
        ls->PushNil();
        return 1;
    }
    ls->GetMetafield(1, str::METATABLE);
    return 1;
}

int32_t lua::stdlib::base::SetMetatable(State* ls)
{
    auto t = ls->Type(2);
    ls->CheckType(1, type::TABLE);
    ls->ArgCheck(t == type::NIL || t == type::TABLE, 2, "nil or table expected");
    if (ls->GetMetafield(1, str::METATABLE) != type::NIL)
    {
        return ls->Error2("cannot change a protected metatable");
    }
    ls->SetTop(2);
    ls->SetMetatable(1);
    return 1;
}

int32_t lua::stdlib::base::RawEqual(State* ls)
{
    ls->CheckAny(1);
    ls->CheckAny(2);
    ls->PushBoolean(ls->RawEqual(1, 2));
    return 1;
}

int32_t lua::stdlib::base::RawLen(State* ls)
{
    auto t = ls->Type(1);
    ls->ArgCheck(t == type::TABLE || t == type::STRING, 1, "table or string expected");
    ls->PushInteger(int64_t(ls->RawLen(1)));
    return 1;
}

int32_t lua::stdlib::base::RawGet(State* ls)
{
    ls->CheckType(1, type::TABLE);
    ls->CheckAny(2);
    ls->SetTop(2);
    ls->RawGet(1);
    return 1;
}

int32_t lua::stdlib::base::RawSet(State* ls)
{
    ls->CheckType(1, type::TABLE);
    ls->CheckAny(2);
    ls->CheckAny(3);
    ls->SetTop(3);
    ls->RawSet(1);
    return 1;
}

int32_t lua::stdlib::base::Type(State* ls)
{
    auto t = ls->Type(1);
    ls->ArgCheck(t != type::NONE, 1, "value expected");
    ls->PushString(ls->TypeName(t));
    return 1;
}

int32_t lua::stdlib::base::ToString(State* ls)
{
    ls->CheckAny(1);
    ls->ToString2(1);
    return 1;
}

int32_t lua::stdlib::base::ToNumber(State* ls)
{
    if (ls->IsNoneOrNil(2))
    {
        ls->CheckAny(1);
        if (ls->Type(1) == type::NUMBER)
        {
            ls->SetTop(1);
            return 1;
        }
        else if (auto [s, ok] = ls->ToStringX(1); ok && ls->StringToNumber(std::move(s)))
        {
            return 1;
        }
    }
    else
    {
        ls->CheckType(1, type::STRING);
        auto s = ls->ToString(1);
        auto base = int(ls->CheckInteger(2));
        ls->ArgCheck(2 <= base && base <= 36, 2, "base out of range");
        auto str = number::Trim(s);
        auto cs = str.c_str();
        int64_t val;
        if (std::from_chars(cs, cs + str.size(), val, base).ec == std::errc{})
        {
            ls->PushInteger(val);
            return 1;
        }
    }
    ls->PushNil();
    return 1;
}

int32_t lua::stdlib::base::CollectGarbage(State* ls)
{
    // TODO
    return 0;
}
