#ifndef _LIB_TYPE_H
#define _LIB_TYPE_H
#include <stdint.h>
#include <array>
namespace lua
{
struct State;
using Function = int32_t (*)(State*);
using pair_type = std::pair<const char*, Function>;
template <std::size_t N>
using FuncReg = std::array<pair_type, N>;

}  // namespace lua

#endif