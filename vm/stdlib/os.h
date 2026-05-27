#ifndef _OS_H
#define _OS_H
#include "libtype.h"

namespace lua
{
struct State;

namespace stdlib
{

int32_t OpenOSLib(State* ls);
namespace os
{

}  // namespace os

}  // namespace stdlib

}  // namespace lua

#endif