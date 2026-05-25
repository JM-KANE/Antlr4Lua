#include "State.h"
using namespace lua;

lua::State::State() : registry(8)
{
    registry.Put(cv::RIDX_MAINTHREAD, this);
    registry.Put(cv::RIDX_GLOBALS, &globals);
    PushLuaStack(cv::MINSTACK, this);
}

void lua::State::PushLuaStack(size_t size, State* st)
{
    auto& stackPrev = stack();
    auto& stack = stacks.emplace_back(std::make_unique<Stack>(size, st));
    stack->prev = &stackPrev;
}

std::unique_ptr<Stack> lua::State::PopLuaStack()
{
    auto stk = std::move(stacks.back());
    stk->prev = {};
    stacks.pop_back();
    return stk;
}

void lua::State::Call(int32_t nArgs, int32_t nRes)
{
    auto val = stack().Get(-(nArgs + 1));

    Closure* c{};
    if (val.IsClosure())
    {
        c = std::get<Closure*>(val);
    }
    else if (auto mf = val.GetMetafield("__call", this))
    {
        if (mf->IsClosure())
        {
            c = std::get<Closure*>(*mf);
            stack().Push(val);

            // args
        }
    }

    if (c)
    {
        // c->proto
    }
}
