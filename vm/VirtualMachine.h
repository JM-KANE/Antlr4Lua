#ifndef _VIRTUAL_MACHINE_H
#define _VIRTUAL_MACHINE_H

#include "State.h"

namespace lua
{
class VirtualMachine
{
    std::vector<std::unique_ptr<Closure>> _closureStorage;
    std::vector<std::unique_ptr<Table>> _tableStorage;

    State self;

public:
    VirtualMachine();
    void Run(const Prototype& p);

    Closure& NewLuaClosure(const Prototype& p);
    Closure& NewFuncClosure(Function f, size_t nUpvals);
    Table& NewLuaTable(size_t nArr, size_t nRec);

    void Call(int64_t nArgs, int64_t nResults);
};
}  // namespace lua

#endif