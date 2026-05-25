#ifndef _STATE_H
#define _STATE_H

#include "Struct.h"
namespace lua
{
struct State
{
    std::vector<std::unique_ptr<Stack>> stacks;
    Table registry;
    Table globals;

    State();

    auto& stack()
    {
        return *stacks.back();
    }

    void PushLuaStack(size_t size, State* st);
    std::unique_ptr<Stack> PopLuaStack();

    void Call(int32_t nArgs, int32_t nRes);
};

}  // namespace lua
#endif