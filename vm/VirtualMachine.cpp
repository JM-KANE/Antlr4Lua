#include "VirtualMachine.h"
using namespace lua;

lua::VirtualMachine::VirtualMachine()
{
}

void lua::VirtualMachine::Run(const Prototype& p)
{
    self.OpenLibs();

    auto& c = NewLuaClosure(p);
    self.stack().Push(&c);
    if (!p.Upvalues.empty())
    {
        auto& env = *self.registry.Get(cv::RIDX_GLOBALS);
        c.upvals.front() = std::make_unique<Upvalue>(env);
    }

    self.Call(0, -1);
}

Closure& lua::VirtualMachine::NewLuaClosure(const Prototype& p)
{
    return *_closureStorage.emplace_back(std::make_unique<Closure>(&p));
}

Closure& lua::VirtualMachine::NewFuncClosure(Function f, size_t nUpvals)
{
    return *_closureStorage.emplace_back(std::make_unique<Closure>(f, nUpvals));
}

Table& lua::VirtualMachine::NewLuaTable(size_t nArr, size_t nRec)
{
    return *_tableStorage.emplace_back(std::make_unique<Table>(nArr, nRec));
}

void lua::VirtualMachine::Call(int64_t nArgs, int64_t nResults)
{
}
