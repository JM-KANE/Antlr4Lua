#ifndef _COROUTINE_H
#define _COROUTINE_H
#include "libtype.h"

namespace lua
{
struct State;

namespace stdlib
{

int32_t OpenCoroutineLib(State* ls);
namespace coroutine
{

}  // namespace coroutine

}  // namespace stdlib

}  // namespace lua

#endif