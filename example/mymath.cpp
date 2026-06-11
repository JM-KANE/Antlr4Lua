#ifdef _WIN32
#define LUA_BUILD_AS_DLL
#define LUA_LIB
#endif

#include <alua.hpp>

static int32_t l_double(lua::State *L)
{
    auto x = L->CheckInteger(1);
    L->PushInteger(2 * x);
    return 1;
}

static constexpr lua::FuncReg<2> funcs = {
    lua::Reg{"double", l_double},
    {NULL, NULL}};

LUAMOD_API int32_t luaopen_mymath(lua::State *L)
{
    L->NewLib(funcs);
    return 1;
}