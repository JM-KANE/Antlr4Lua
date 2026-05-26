#ifndef _OP_CODE_H
#define _OP_CODE_H

#include "../code_gen/OpCodesType.h"
#include <array>

namespace lua
{
using action_type = void (*)(class Instruction i, struct State* ls);

struct OpCode
{
    static constexpr auto ISA = 47;
    uint8_t flag{};
    OpArg argBMode{};
    OpArg argCMode{};
    Inst opMode{};
    const char* name{};
    action_type action;

    static const std::array<OpCode, ISA>& Get();
};

}  // namespace lua

#endif