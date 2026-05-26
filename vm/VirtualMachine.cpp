#include "VirtualMachine.h"
using namespace lua;

lua::VirtualMachine::VirtualMachine()
{
}

void lua::VirtualMachine::Run(Prototype& p)
{
    auto& c = NewLuaClosure(p);
    self.stack().Push(&c);
    if (!p.Upvalues.empty())
    {
        auto env = self.registry.Get(cv::RIDX_GLOBALS);
        c.upvals.front() = env;
    }

    self.Call(0, -1);
}

Closure& lua::VirtualMachine::NewLuaClosure(Prototype& p)
{
    return _closureStorage.emplace_back(&p);
}

Table& lua::VirtualMachine::NewLuaTable(size_t nArr, size_t nRec)
{
    return _tableStorage.emplace_back(nArr, nRec);
}

void lua::VirtualMachine::Call(int64_t nArgs, int64_t nResults)
{
}
