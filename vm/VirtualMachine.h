#ifndef _VIRTUAL_MACHINE_H
#define _VIRTUAL_MACHINE_H

#include "State.h"

namespace lua
{
class VirtualMachine
{
    std::vector<Closure> _closureStorage;

    State self;

public:
    void Run(Prototype& p);

    Closure& NewLuaClosure(Prototype& p);

    void Call(int64_t nArgs, int64_t nResults);
};
}  // namespace lua

#endif