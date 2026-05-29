#include "base.h"
#include "../State.h"
using namespace lua;
using namespace stdlib;
namespace
{
constexpr FuncReg<32> funcs{
    pair_type{"print", base::Print},
    {"assert", base::Assert},
    {"error", base::Error},
    {"warn", nullptr},
    {"select", base::Select},
    {"ipairs", base::IPairs},
    {"pairs", base::Pairs},
    {"next", base::Next},
    {"load", base::Load},
    {"loadfile", base::LoadFile},
    {"loadstring", nullptr},
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
    {"collectgarbage", nullptr},
    {"getfenv", nullptr},
    {"setfenv", nullptr},
    {"newproxy", nullptr},
    {"module", nullptr},
    {"unpack", nullptr},

    {"_G", nullptr},
    {"_VERSION", nullptr},
    {"arg", nullptr},
};
}  // namespace

int32_t lua::stdlib::OpenBaseLib(State* ls)
{
    ls->PushGlobalTable();
    ls->SetFuncs(funcs, 0);
    ls->PushValue(-1);
    ls->SetField(-2, "_G");
    ls->PushString("Lua 5.3");
    ls->SetField(-2, "_VERSION");
    ls->PushAny(ls->GetArgs());
    ls->SetField(-2, "arg");
    return 1;
}

int32_t lua::stdlib::base::Print(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::Assert(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::Error(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::Select(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::IPairs(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::Pairs(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::Next(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::Load(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::LoadFile(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::DoFile(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::PCall(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::XPCall(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::GetMetatable(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::SetMetatable(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::RawEqual(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::RawLen(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::RawGet(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::RawSet(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::Type(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::ToString(State* ls)
{
    return 0;
}

int32_t lua::stdlib::base::ToNumber(State* ls)
{
    return 0;
}
