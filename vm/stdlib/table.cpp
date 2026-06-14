#include "table.h"
#include "../State.h"
using namespace lua;
using namespace stdlib;

namespace lua::stdlib::table
{
constexpr uint8_t R = 1;
constexpr uint8_t W = 2;
constexpr uint8_t L = 4;
constexpr uint8_t RW = R | W;

}  // namespace lua::stdlib::table

namespace
{
constexpr FuncReg<7> tabFuncs{Reg{"move", table::Move}, {"insert", table::Insert}, {"remove", table::Remove},
                              {"sort", table::Sort},    {"concat", table::Concat}, {"pack", table::Pack},
                              {"unpack", table::Unpack}};

constexpr size_t MAX_LEN = 1'000'000;  // ?

bool checkField(State* ls, const char* key, int n)
{
    ls->PushString(key);
    return ls->RawGet(-n) != cv::type::LUA_TNIL;
}

void checkTab(State* ls, int32_t arg, int what)
{
    if (!ls->IsTable(arg))
    {
        int32_t n = 1;
        if (auto mt = ls->GetMetatable(arg))
        {
            if ((!(what & table::R) || checkField(ls, str::INDEX, ++n))
                && (!(what & table::W) || checkField(ls, str::NEWINDEX, ++n))
                && (!(what & table::L) || ls->IsString(arg) || checkField(ls, str::LEN, ++n)))
            {
                ls->Pop(n);
                return;
            }
        }
    }
    ls->CheckType(arg, cv::type::LUA_TTABLE);
}

}  // namespace

int32_t lua::stdlib::OpenTableLib(State* ls)
{
    ls->NewLib(tabFuncs);
    return 1;
}

int32_t lua::stdlib::table::Move(State* ls)
{
    auto f = ls->CheckInteger(2);
    auto e = ls->CheckInteger(3);
    auto t = ls->CheckInteger(4);
    int32_t tt = 1;
    bool hasDest = !ls->IsNoneOrNil(5);
    if (hasDest)
    {
        tt = 5;
    }
    checkTab(ls, 1, R);
    checkTab(ls, tt, W);
    if (e >= f)
    {
        ls->ArgCheck(f > 0 || e < cv::LUA_MAXINTEGER + f, 3, "too many elements to move");
        auto n = e - f + 1;
        ls->ArgCheck(t <= cv::LUA_MAXINTEGER - n + 1, 4, "destination wrap around");
        for (auto i = n - 1; i >= 0; i--)
        {
            ls->GetI(1, f + i);
            ls->SetI(tt, t + i);
        }
    }
    if (!hasDest)
    {
        ls->PushValue(tt);
    }

    return 1;
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
