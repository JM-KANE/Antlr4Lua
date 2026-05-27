#ifndef _PACKAGE_H
#define _PACKAGE_H
#include "libtype.h"

namespace lua
{
struct State;

namespace stdlib
{

int32_t OpenPackageLib(State* ls);
namespace package
{

}  // namespace package

}  // namespace stdlib

}  // namespace lua

#endif