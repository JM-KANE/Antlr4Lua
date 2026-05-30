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
int32_t Move(State* ls);
int32_t Insert(State* ls);
int32_t Remove(State* ls);
int32_t Sort(State* ls);
int32_t Concat(State* ls);
int32_t Pack(State* ls);
int32_t Unpack(State* ls);

}  // namespace table

}  // namespace stdlib

}  // namespace lua

#endif