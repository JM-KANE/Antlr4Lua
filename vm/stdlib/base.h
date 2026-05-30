#ifndef _BASE_H
#define _BASE_H
#include "libtype.h"

namespace lua
{

namespace stdlib
{

int32_t OpenBaseLib(State* ls);
namespace base
{
int32_t Print(State* ls);
int32_t Assert(State* ls);
int32_t Error(State* ls);
int32_t Warn(State* ls);
int32_t Select(State* ls);
int32_t IPairs(State* ls);
int32_t Pairs(State* ls);
int32_t Next(State* ls);
int32_t Load(State* ls);
int32_t LoadFile(State* ls);
int32_t DoFile(State* ls);
int32_t PCall(State* ls);
int32_t XPCall(State* ls);
int32_t GetMetatable(State* ls);
int32_t SetMetatable(State* ls);
int32_t RawEqual(State* ls);
int32_t RawLen(State* ls);
int32_t RawGet(State* ls);
int32_t RawSet(State* ls);
int32_t Type(State* ls);
int32_t ToString(State* ls);
int32_t ToNumber(State* ls);
int32_t CollectGarbage(State* ls);
}  // namespace base

}  // namespace stdlib

}  // namespace lua

#endif