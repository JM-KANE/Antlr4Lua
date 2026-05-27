#ifndef _TABLE_H
#define _TABLE_H
#include "libtype.h"

namespace lua
{
struct State;

namespace stdlib
{

int32_t OpenTableLib(State* ls);
namespace table
{

}  // namespace table

}  // namespace stdlib

}  // namespace lua

#endif