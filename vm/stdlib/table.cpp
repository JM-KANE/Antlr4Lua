#include "table.h"
#include "../State.h"
using namespace lua;
using namespace stdlib;

namespace
{
constexpr FuncReg<7> tabFuncs{pair_type{"move", table::Move}, {"insert", table::Insert}, {"remove", table::Remove},
                              {"sort", table::Sort},          {"concat", table::Concat}, {"pack", table::Pack},
                              {"unpack", table::Unpack}};

constexpr size_t MAX_LEN = 1'000'000;  // ?
}  // namespace

int32_t lua::stdlib::OpenTableLib(State* ls)
{
    ls->NewLib(tabFuncs);
    return 1;
}

int32_t lua::stdlib::table::Move(State* ls)
{
    return 0;
}

int32_t lua::stdlib::table::Insert(State* ls)
{
    return 0;
}

int32_t lua::stdlib::table::Remove(State* ls)
{
    return 0;
}

int32_t lua::stdlib::table::Sort(State* ls)
{
    return 0;
}

int32_t lua::stdlib::table::Concat(State* ls)
{
    return 0;
}

int32_t lua::stdlib::table::Pack(State* ls)
{
    return 0;
}

int32_t lua::stdlib::table::Unpack(State* ls)
{
    auto i = ls->OptInteger(2, 1);
    auto e = ls->OptInteger(3, ls->Len2(1));
    if (i > e)
    {
        return 0;
    }

    auto n = int(e - i + 1);
    if (n <= 0 || n >= MAX_LEN || !ls->CheckStack(n))
    {
        return ls->Error2("too many results to unpack");
    }

    for (; i < e; i++)
    {
        ls->GetI(1, i);
    }
    ls->GetI(1, e);
    return n;
}
