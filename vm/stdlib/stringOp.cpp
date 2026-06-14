#include "stringOp.h"
#include "../State.h"
using namespace lua;

namespace
{
size_t posrelatI(int64_t pos, size_t len)
{
    if (pos >= 0 || pos < -(int64_t)len)
        return (size_t)std::max(int64_t(1), pos);
    else
        return len + size_t(pos) + 1;
}
size_t getendpos(State* ls, int32_t idx, int64_t def, size_t len)
{
    auto pos = ls->OptInteger(idx, def);
    if (pos >= 0)
        return std::min(size_t(pos), len);
    else if (pos < -(int64_t)len)
        return 0;
    else
        return len + size_t(pos) + 1;
}

}  // namespace

namespace lua
{
namespace stdlib
{
namespace str
{
int32_t byte(State* ls)
{
    auto str = ls->CheckString(1);
    auto len = str.size();
    auto start = ls->OptInteger(2, 1);
    auto i = posrelatI(start, len);
    auto j = getendpos(ls, 3, start, len);
    if (i > j)
        return 0;
    --i;
    auto n = j - i;
    ls->CheckStack2(int32_t(n), "string slice too long");
    while (i < j)
    {
        auto c = (unsigned char)str[i];
        ls->PushInteger(int64_t(c));
        ++i;
    }
    return n;
}

int32_t char_(State* ls)
{
    auto nArg = ls->GetTop();
    std::string s;
    s.reserve(nArg);
    for (size_t i = 0; i < nArg; i++)
    {
        auto idx = int32_t(i) + 1;
        auto n = ls->CheckInteger(idx);
        ls->ArgCheck(n >= 0 && n < 256, idx, "value out of range");
        s.push_back(char(n));
    }
    ls->PushString(std::move(s));
    return 1;
}

int32_t dump(State* ls)
{
    // TODO binary loadable
    return 0;
}

int32_t find(State* ls)
{
    return 0;
}

int32_t format(State* ls)
{
    return 0;
}

int32_t gmatch(State* ls)
{
    return 0;
}
int32_t gsub(State* ls)
{
    return 0;
}

int32_t len(State* ls)
{
    return 0;
}

int32_t lower(State* ls)
{
    return 0;
}
int32_t match(State* ls)
{
    return 0;
}
int32_t rep(State* ls)
{
    return 0;
}

int32_t reverse(State* ls)
{
    return 0;
}

int32_t sub(State* ls)
{
    return 0;
}

int32_t upper(State* ls)
{
    return 0;
}

int32_t pack(State* ls)
{
    return 0;
}

int32_t packsize(State* ls)
{
    return 0;
}

int32_t unpack(State* ls)
{
    return 0;
}

}  // namespace str

}  // namespace stdlib
}  // namespace lua

using namespace stdlib;

namespace
{
constexpr FuncReg<17> strFuncs{
    Reg{"byte", stdlib::str::byte},      {"char", stdlib::str::char_},    {"dump", stdlib::str::dump},
    {"find", stdlib::str::find},         {"format", stdlib::str::format}, {"gmatch", stdlib::str::gmatch},
    {"gsub", stdlib::str::gsub},         {"len", stdlib::str::len},       {"lower", stdlib::str::lower},
    {"match", stdlib::str::match},       {"rep", stdlib::str::rep},       {"reverse", stdlib::str::reverse},
    {"sub", stdlib::str::sub},           {"upper", stdlib::str::upper},   {"pack", stdlib::str::pack},
    {"packsize", stdlib::str::packsize}, {"unpack", stdlib::str::unpack},
};

void createMetatable(State* ls)
{
    ls->CreateTable(0, 1);
    ls->PushString("");
    ls->PushValue(-2);
    ls->SetMetatable(-2);
    ls->Pop(1);
    ls->PushValue(-2);
    ls->SetField(-2, lua::str::INDEX);
    ls->Pop(1);
}

}  // namespace

int32_t lua::stdlib::OpenStringLib(State* ls)
{
    ls->NewLib(strFuncs);
    createMetatable(ls);
    return 1;
}