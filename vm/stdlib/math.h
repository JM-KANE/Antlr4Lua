#ifndef _MATH_H
#define _MATH_H
#include "libtype.h"

namespace lua
{
struct State;

namespace stdlib
{

int32_t OpenMathLib(State* ls);
namespace math
{

}  // namespace math

}  // namespace stdlib

}  // namespace lua

#endif