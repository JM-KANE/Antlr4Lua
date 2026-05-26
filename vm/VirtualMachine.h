#ifndef _VIRTUAL_MACHINE_H
#define _VIRTUAL_MACHINE_H

#include "State.h"

namespace lua
{
class VirtualMachine
{
    std::vector<Closure> _closureStorage;
    std::vector<Table> _tableStorage;

    State self;

public:
    VirtualMachine();
    void Run(Prototype& p);

    Closure& NewLuaClosure(Prototype& p);
    Table& NewLuaTable(size_t nArr, size_t nRec);

    void Call(int64_t nArgs, int64_t nResults);
};
}  // namespace lua

#endif