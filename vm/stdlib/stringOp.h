#ifndef _STRINGOP_H
#define _STRINGOP_H
#include "libtype.h"

namespace lua
{
struct State;

namespace stdlib
{

int32_t OpenStringLib(State* ls);
namespace stringOp
{

}  // namespace stringOp

}  // namespace stdlib

}  // namespace lua

#endif