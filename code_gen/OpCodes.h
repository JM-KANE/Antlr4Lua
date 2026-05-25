#ifndef _OP_CODES_H
#define _OP_CODES_H

#include <stdint.h>

namespace lua
{

namespace cv
{
constexpr auto MAXARG_Bx = 1 << 18 - 1;      // 262143
constexpr auto MAXARG_sBx = MAXARG_Bx >> 1;  // 131071
}  // namespace cv

enum class Op : uint32_t
{
    MOVE,
    LOADK,
    LOADKX,
    LOADBOOL,
    LOADNIL,
    GETUPVAL,
    GETTABUP,
    GETTABLE,
    SETTABUP,
    SETUPVAL,
    SETTABLE,
    NEWTABLE,
    SELF,
    ADD,
    SUB,
    MUL,
    MOD,
    POW,
    DIV,
    IDIV,
    BAND,
    BOR,
    BXOR,
    SHL,
    SHR,
    UNM,
    BNOT,
    NOT,
    LEN,
    CONCAT,
    JMP,
    EQ,
    LT,
    LE,
    TEST,
    TESTSET,
    CALL,
    TAILCALL,
    RETURN,
    FORLOOP,
    FORPREP,
    TFORCALL,
    TFORLOOP,
    SETLIST,
    CLOSURE,
    VARARG,
    EXTRAARG
};

}  // namespace lua

#endif