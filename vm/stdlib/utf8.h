#ifndef _UTF8_H
#define _UTF8_H
#include "libtype.h"

namespace lua
{
struct State;

namespace stdlib
{

int32_t OpenUTF8Lib(State* ls);
namespace utf8
{

}  // namespace utf8

}  // namespace stdlib

}  // namespace lua

#endif