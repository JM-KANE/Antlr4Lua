#ifndef ALUA_HPP
#define ALUA_HPP

#include "../vm/VirtualMachine.h"


/*
@@ LUA_API is a mark for all core API functions.
@@ LUALIB_API is a mark for all auxiliary library functions.
@@ LUAMOD_API is a mark for all standard library opening functions.
** CHANGE them if you need to define those functions in some special way.
** For instance, if you want to create one Windows DLL with the core and
** the libraries, you may want to use the following definition (define
** LUA_BUILD_AS_DLL to get it).
*/
#if defined(__cplusplus)
#define LUA_EXTERN extern "C"  
#define LUA_EXTERN_ LUA_EXTERN
#else
#define LUA_EXTERN extern
#define LUA_EXTERN_ 
#endif

/* LUA_API */
#ifdef LUA_BUILD_AS_DLL
#if defined(LUA_CORE) || defined(LUA_LIB)
#define LUA_API_ __declspec(dllexport)
#else
#define LUA_API_ __declspec(dllimport)
#endif
#define LUA_API LUA_EXTERN_ LUA_API_
#else
#define LUA_API_
#define LUA_API LUA_EXTERN
#endif

#define LUALIB_API LUA_API

/* LUAMOD_API */    
#define LUAMOD_API LUA_EXTERN LUA_API_

#endif