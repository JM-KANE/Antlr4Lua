#ifndef _VIRTUAL_MACHINE_H
#define _VIRTUAL_MACHINE_H

#include "State.h"
#include <forward_list>
#include <tuple>

namespace lua
{
struct Collector
{
    template <typename... Ts>
    using container_type = std::forward_list<Ts...>;
    template <typename T>
    using store_type = std::pair<container_type<T>, size_t>;
    using collect_types = std::tuple<Closure, Table>;
    static constexpr size_t STORAGE_NUM = std::tuple_size_v<collect_types>;
    using stores_type = tuple_map_t<store_type, collect_types>;

    enum class State
    {
        PAUSE,
        PROPAGATE,
        SWEEP
    };

    template <typename T>
    using make_cursor = std::pair<container_type<T>*, typename container_type<T>::iterator>;
    struct Cursor : public tuple_map_t<make_cursor, collect_types>
    {
        using base = tuple_map_t<make_cursor, collect_types>;
        constexpr operator bool() const
        {
            return std::apply([](auto&&... it) { return ((it.first->end() != std::next(it.second)) || ...); },
                              static_cast<const base&>(*this));
        }
    };

    struct Deletor
    {
        bool operator()(auto& v) const
        {
            bool res = !v.color;
            if (!res)
            {
                v.color = 0;
            }
            return res;
        }
    };
    struct Refresher
    {
        void operator()(auto&& v) const
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, Table*> || std::is_same_v<T, Closure*>)
                v->color = 0;
        }
    };

    static constexpr size_t OBJ_SIZE = 56;
    static constexpr size_t INIT_THRESHOLD = 100 * 1024;
    static constexpr size_t STEP_NUM = 10;

    class VirtualMachine* vm;
    size_t threshold = INIT_THRESHOLD;
    size_t speed = 0;
    State state{};
    Cursor sweepCursor;
    std::vector<Value> grey;
    stores_type _Storage;
    std::forward_list<std::weak_ptr<Value>> _closedValue;
    // TODO __gc
    // tobefnz

    Collector(VirtualMachine* _vm);
    void Collect();
    void SingleStep();

    template <typename T, typename... Ts>
    auto& Allocate(Ts&&... args)
    {
        auto& s = std::get<store_type<T>>(_Storage);
        ++s.second;
        return s.first.emplace_front(std::forward<Ts>(args)...);
    }

    void CheckGC();

private:
    size_t Memory() const;

    template <typename OP>
    decltype(auto) Apply(const OP& op)
    {
        return std::apply([&](auto&&... arg) { (op(arg), ...); }, _Storage);
    }
    template <typename OP>
    decltype(auto) Apply(const OP& op) const
    {
        return std::apply([&](auto&&... arg) { (op(arg), ...); }, _Storage);
    }

    template <std::size_t I, typename F, typename... Arrays>
    decltype(auto) InvokeOnce(const F& op, Arrays&&... arrays)
    {
        return op(std::get<I>(_Storage), std::get<I>(std::forward<Arrays>(arrays))...);
    }
    template <typename F, typename... Arrays, std::size_t... I>
    decltype(auto) DoInvoke(const F& op, std::index_sequence<I...>, Arrays&&... arrays)
    {
        return (InvokeOnce<I>(op, std::forward<Arrays>(arrays)...), ...);
    }
    template <typename F, typename... Arrays>
    decltype(auto) Invoke(const F& op, Arrays&&... arrays)
    {
        return DoInvoke(op, std::make_index_sequence<STORAGE_NUM>(), std::forward<Arrays>(arrays)...);
    }

    void InitialCursor();
    void Refresh();
    void MarkGrey();
    void MarkChildren();
    void Mark();
    void SweepOnce();
    void Sweep();
    std::array<size_t, STORAGE_NUM> DistributeNumber(size_t N);
};

class VirtualMachine
{
public:
    friend class Collector;

private:
    /* global */
    Table registry;
    Table global;
    std::unique_ptr<Table> args;
    int64_t argc;
    const char** argv;

    // TODO randomseed

    /* main */
    State main;

public:
    Collector gc;
    VirtualMachine(int argc, const char** argv);
    void Run(const Prototype& p);

    Closure& NewLuaClosure(const Prototype& p);
    Closure& NewFuncClosure(Function f, size_t nUpvals);
    Table& NewLuaTable(size_t nArr, size_t nRec);

    Table* GetArgs();

    void AddClosed(const ValuePtr& v);

    void CollectGarbage();
    void CheckGC();
};
}  // namespace lua

#endif