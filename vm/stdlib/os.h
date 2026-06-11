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

int32_t Clock(State* ls);
int32_t Date(State* ls);
int32_t Difftime(State* ls);
int32_t Execute(State* ls);
int32_t Exit(State* ls);
int32_t Getenv(State* ls);
int32_t Remove(State* ls);
int32_t Rename(State* ls);
int32_t Setlocale(State* ls);
int32_t Time(State* ls);
int32_t Tmpname(State* ls);
}  // namespace os

}  // namespace stdlib

}  // namespace lua

#endif