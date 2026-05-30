#include "Instruction.h"
#include "OpCode.h"

using namespace lua;

Instruction::Instruction(uint32_t _i) : i(_i)
{
}

lua::Instruction::operator uint32_t&()
{
    return i;
}

lua::Instruction::operator const uint32_t&() const
{
    return i;
}

lua::Op lua::Instruction::Opcode() const
{
    return Op(*this & 0x3F);
}

std::array<int32_t, 3> lua::Instruction::ABC() const
{
    return {int32_t(*this >> 6 & 0xFF), int32_t(*this >> 23 & 0x1FF), int32_t(*this >> 14 & 0x1FF)};
}

std::array<int32_t, 2> lua::Instruction::ABx() const
{
    return {int32_t(*this >> 6 & 0xFF), int32_t(*this >> 14)};
}

std::array<int32_t, 2> lua::Instruction::AsBx() const
{
    auto res = ABx();
    res.back() -= cv::MAXARG_sBx;
    return res;
}

int32_t lua::Instruction::Ax() const
{
    return int32_t(*this >> 6);
}

void lua::Instruction::Execute(State* ls)
{
    auto& reg = OpCode::Get();
    auto op = Opcode();
    auto action = reg[std::size_t(op)].action;
    action(*this, ls);
}
