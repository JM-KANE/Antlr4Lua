#include "VirtualMachine.h"
using namespace lua;

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

void lua::VirtualMachine::Call(int64_t nArgs, int64_t nResults)
{
}
