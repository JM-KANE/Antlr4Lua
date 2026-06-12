#ifndef IO_H
#define IO_H
#include "libtype.h"

namespace lua
{
struct State;

namespace stdlib
{

int32_t OpenIOLib(State* ls);

namespace io
{
}  // namespace io

}  // namespace stdlib

}  // namespace lua
#endif