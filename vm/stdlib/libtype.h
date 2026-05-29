#ifndef _LIB_TYPE_H
#define _LIB_TYPE_H
#include <stdint.h>
#include <array>
namespace lua
{
struct State;
using Function = int32_t (*)(State*);
using pair_type = std::pair<const char*, Function>;
template <std::size_t N>
using FuncReg = std::array<pair_type, N>;

template <template <typename> class F, typename Tuple>
struct tuple_map;

template <template <typename> class F, typename... Ts>
struct tuple_map<F, std::tuple<Ts...>>
{
    using type = std::tuple<F<Ts>...>;
};

template <template <typename> class F, typename Tuple>
using tuple_map_t = tuple_map<F, Tuple>::type;

}  // namespace lua

#endif