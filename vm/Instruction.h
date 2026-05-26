#ifndef _INSTRUCTION_H
#define _INSTRUCTION_H
#include "../code_gen/OpCodesType.h"
#include <array>

namespace lua
{

class Instruction
{
    uint32_t i{};

public:
    Instruction() = default;
    Instruction(uint32_t _i);
    operator uint32_t&();
    operator const uint32_t&() const;

    lua::Op Opcode() const;
    std::array<int32_t, 3> ABC() const;
    std::array<int32_t, 2> ABx() const;
    std::array<int32_t, 2> AsBx() const;
    int32_t Ax() const;

    void Execute(struct State* ls);
};
}  // namespace lua

#endif