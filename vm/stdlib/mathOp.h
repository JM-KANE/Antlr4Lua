#ifndef _MATHOP_H
#define _MATHOP_H
#include "libtype.h"

namespace lua
{
struct State;

namespace stdlib
{

int32_t OpenMathLib(State* ls);
namespace mathOp
{

}  // namespace mathOp

}  // namespace stdlib

}  // namespace lua

#endif