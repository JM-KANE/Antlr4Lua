

#include <alua.hpp>

int32_t l_factorial(int32_t n)
{
    return n > 1 ? n * l_factorial(n - 1) : 1;
}

LUAMOD_API int32_t factorial(lua::State *ls)
{
    auto n = ls->CheckInteger(1);
    ls->PushInteger(l_factorial(n));
    return 1;
}