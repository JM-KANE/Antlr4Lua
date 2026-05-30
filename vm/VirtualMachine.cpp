#include "VirtualMachine.h"
#include <numeric>
using namespace lua;

lua::Collector::Collector(VirtualMachine* _vm) : vm{_vm}
{
}

void lua::Collector::Collect()
{
    while (state != State::PAUSE)
    {
        SingleStep();
    }
    Refresh();
    Mark();
    Sweep();
    threshold = Memory() * 2;
}

void lua::Collector::SingleStep()
{
    switch (state)
    {
    case State::PAUSE:
    {
        Refresh();
        MarkGrey();
        state = State::PROPAGATE;
    }
    break;
    case State::PROPAGATE:
    {
        auto steps = STEP_NUM * (speed + 10) / 10;
        for (size_t i = 0; i < steps && !grey.empty(); ++i)
        {
            MarkChildren();
        }
        if (grey.empty())
        {
            state = State::SWEEP;
        }
    }
    break;
    case State::SWEEP:
    {
        InitialCursor();
        auto steps = STEP_NUM * (speed + 10) / 10;
        auto nums = DistributeNumber(steps);
        auto sweep = [this](auto& s, size_t N, const auto& cur)
        {
            Deletor del;
            auto& fl = s.first;
            size_t i = 0;
            for (auto it = cur.second; std::next(it) != fl.end() && i < N; ++i)
            {
                auto itn = std::next(it);
                if (del(*itn))
                {
                    fl.erase_after(it);
                    --s.second;
                }
                else
                {
                    ++it;
                }
            }
        };
        Invoke(sweep, nums, sweepCursor);
        if (!sweepCursor)
        {
            state = State::PAUSE;
            threshold = Memory() * 2;
        }
    }
    break;
    default:
        break;
    }
}

void lua::Collector::CheckGC()
{
    if (Memory() >= threshold)
    {
        SingleStep();
        if (state != State::PAUSE)
        {
            ++speed;
        }
    }
    else if (speed > 0)
    {
        --speed;
    }
}

size_t lua::Collector::Memory() const
{
    std::size_t sz = 0;
    auto get = [&](auto& s) { sz += s.second; };
    Apply(get);
    return sz * OBJ_SIZE;
}

void lua::Collector::InitialCursor()
{
    auto set = [this](auto& s, auto& cur)
    {
        auto& fl = s.first;
        cur.first = &fl;
        cur.second = fl.before_begin();
    };

    Invoke(set, sweepCursor);
}

void lua::Collector::Refresh()
{
    Apply(
        [](auto& s)
        {
            for (auto&& v : s.first)
                v.color = 0;
        });
    for (auto&& v : _closedValue)
        std::visit(Refresher(), *v.lock());
    grey.clear();
}

void lua::Collector::MarkGrey()
{
    vm->main.Mark(grey);
    std::erase_if(_closedValue,
                  [&](auto& val)
                  {
                      bool res = val.expired();
                      if (!res)
                      {
                          val.lock()->Mark(grey);
                      }
                      return res;
                  });
}

void lua::Collector::MarkChildren()
{
    auto val = grey.back();
    grey.pop_back();
    val.MarkChildren(grey);
    val.SetBlack();
}

void lua::Collector::Mark()
{
    MarkGrey();
    while (!grey.empty())
    {
        MarkChildren();
    }
}

void lua::Collector::SweepOnce()
{
}

void lua::Collector::Sweep()
{
    Apply([](auto& s) { s.second -= std::erase_if(s.first, Deletor()); });
}

static void Hamilton(std::array<size_t, Collector::STORAGE_NUM>& weights, size_t total, size_t sum_w)
{
    using arr_type = std::array<size_t, Collector::STORAGE_NUM>;
    if (sum_w == 0)
    {
        for (auto&& i : weights)
        {
            i = 0;
        }
    }

    arr_type remainders;
    size_t allocated = 0;

    for (size_t i = 0; i < weights.size(); ++i)
    {
        auto w = weights[i];
        size_t val = (total * w) / sum_w;
        weights[i] = val;
        allocated += val;
        remainders[i] = (total * w) % sum_w;
    }
    size_t remainder = total - allocated;
    arr_type idx{};
    std::iota(idx.begin(), idx.end(), 0);
    std::partial_sort(idx.begin(), idx.begin() + remainder, idx.end(),
                      [&](size_t a, size_t b) { return remainders[a] > remainders[b]; });
    for (size_t k = 0; k < remainder; ++k)
        weights[idx[k]]++;
}

std::array<size_t, Collector::STORAGE_NUM> lua::Collector::DistributeNumber(size_t N)
{
    std::array<size_t, Collector::STORAGE_NUM> A{};
    size_t total{};
    std::size_t i = 0;
    auto get = [&](auto& s)
    {
        A[i] = s.second;
        total += s.second;
        ++i;
    };
    Apply(get);
    Hamilton(A, N, total);
    return A;
}

lua::VirtualMachine::VirtualMachine(int _argc, const char** _argv)
    : gc(this),
      argc(_argc),
      argv(_argv),
      registry(8, 0),
      global(0, 20),
      main(this, registry)
{
    main.registry.Put(cv::RIDX_MAINTHREAD, &main);
    main.registry.Put(cv::RIDX_GLOBALS, &global);
    main.PushLuaStack(cv::MINSTACK, &main);
}

void lua::VirtualMachine::Run()
{
    main.OpenLibs();
    main.LoadFile(argv[1]);
    main.Call((int32_t)argc - 2, -1);
}

Closure& lua::VirtualMachine::NewLuaClosure(const Prototype& p)
{
    return gc.Allocate<Closure>(&p);
}

Closure& lua::VirtualMachine::NewFuncClosure(Function f, size_t nUpvals)
{
    return gc.Allocate<Closure>(f, nUpvals);
}

Table& lua::VirtualMachine::NewLuaTable(size_t nArr, size_t nRec)
{
    return gc.Allocate<Table>(nArr, nRec);
}

Table* lua::VirtualMachine::GetArgs()
{
    if (!args)
    {
        args = std::make_unique<Table>(argc - 2, 0);
        for (int64_t i = 0; i < argc; i++)
        {
            args->Put(i - 1, std::string(argv[i]));
        }
    }
    return args.get();
}

void lua::VirtualMachine::AddClosed(const ValuePtr& v)
{
    gc._closedValue.emplace_front(v);
}

void lua::VirtualMachine::CollectGarbage()
{
    gc.Collect();
}

void lua::VirtualMachine::CheckGC()
{
    gc.CheckGC();
}
