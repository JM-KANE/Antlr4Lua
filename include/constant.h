#ifndef CONSTANT_H
#define CONSTANT_H

#include <stdint.h>
#include <limits>

namespace lua
{

namespace cv
{
constexpr auto MAXARG_Bx = 1 << (18 - 1);    // 262143
constexpr auto MAXARG_sBx = MAXARG_Bx >> 1;  // 131071
constexpr auto LFIELDS_PER_FLUSH = 50;

constexpr auto LUA_MINSTACK = 20;
constexpr auto LUAI_MAXSTACK = 1'000'000;
constexpr auto LUA_REGISTRYINDEX = -LUAI_MAXSTACK - 1000;

constexpr auto LUA_MAXINTEGER = std::numeric_limits<int64_t>::max();
constexpr auto LUA_MININTEGER = std::numeric_limits<int64_t>::min();

constexpr auto LuaUpvalueIndex(int32_t i)
{
    return LUA_REGISTRYINDEX - i;
}

constexpr int64_t LUA_RIDX_MAINTHREAD = 1;
constexpr int64_t LUA_RIDX_GLOBALS = 2;
constexpr int64_t LUA_MULTRET = -1;

namespace type
{
constexpr uint8_t LUA_TNIL = 0;
constexpr uint8_t LUA_TBOOLEAN = 1;
constexpr uint8_t LUA_TLIGHTUSERDATA = 2;
constexpr uint8_t LUA_TNUMBER = 3;
constexpr uint8_t LUA_TSTRING = 4;
constexpr uint8_t LUA_TTABLE = 5;
constexpr uint8_t LUA_TFUNCTION = 6;
constexpr uint8_t LUA_TUSERDATA = 7;
constexpr uint8_t LUA_TTHREAD = 8;
constexpr uint8_t LUA_TNONE = -1;
}  // namespace type

}  // namespace cv

using cv::LuaUpvalueIndex;

namespace str
{
constexpr char ENV[] = "_ENV";
constexpr char LUA_LOADED_TABLE[] = "_LOADED";
constexpr char LUA_PRELOAD_TABLE[] = "_PRELOAD";

constexpr char INDEX[] = "__index";
constexpr char CALL[] = "__call";
constexpr char NEWINDEX[] = "__newindex";
constexpr char ADD[] = "__add";
constexpr char SUB[] = "__sub";
constexpr char MUL[] = "__mul";
constexpr char MOD[] = "__mod";
constexpr char POW[] = "__pow";
constexpr char DIV[] = "__div";
constexpr char IDIV[] = "__idiv";
constexpr char BAND[] = "__band";
constexpr char BOR[] = "__bor";
constexpr char BXOR[] = "__bxor";
constexpr char SHL[] = "__shl";
constexpr char SHR[] = "__shr";
constexpr char UNM[] = "__unm";
constexpr char BNOT[] = "__bnot";
constexpr char LEN[] = "__len";
constexpr char CONCAT[] = "__concat";
constexpr char EQ[] = "__eq";
constexpr char LT[] = "__lt";
constexpr char LE[] = "__le";
constexpr char NAME[] = "__name";
constexpr char PAIRS[] = "__pairs";
constexpr char METATABLE[] = "__metatable";
constexpr char TOSTRING[] = "__tostring";

}  // namespace str

enum class TStatus : uint8_t
{
    LUA_OK,
    LUA_YIELD,
    LUA_ERRRUN,
    LUA_ERRSYNTAX,
    LUA_ERRMEM,
    LUA_ERRGCMM,
    LUA_ERRERR,
    LUA_ERRFILE
};
}  // namespace lua

#endif
