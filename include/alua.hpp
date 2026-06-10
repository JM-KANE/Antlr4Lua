#ifndef ALUA_HPP
#define ALUA_HPP

#include "../vm/VirtualMachine.h"

/* LUA_API */
#ifdef LUA_BUILD_AS_DLL
#if defined(LUA_CORE) || defined(LUA_LIB)
#define LUA_API __declspec(dllexport)
#else
#define LUA_API __declspec(dllimport)
#endif
#else
#define LUA_API extern
#endif

/* LUAMOD_API */
#if defined(__cplusplus)
#define LUAMOD_API extern "C"
#else
#define LUAMOD_API LUA_API
#endif

#endif