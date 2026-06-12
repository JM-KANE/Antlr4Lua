#ifndef _LIB_TYPE_H
#define _LIB_TYPE_H
#include <stdint.h>
#include <array>
#include <string>
#include <random>

namespace lua
{
struct State;
using Function = int32_t(State*);
using Reg = std::pair<const char*, Function*>;
template <std::size_t N>
using FuncReg = std::array<Reg, N>;

template <template <typename> class F, typename Tuple>
struct tuple_map;

template <template <typename> class F, typename... Ts>
struct tuple_map<F, std::tuple<Ts...>>
{
    using type = std::tuple<F<Ts>...>;
};

template <template <typename> class F, typename Tuple>
using tuple_map_t = tuple_map<F, Tuple>::type;

using namespace std::literals;

template <typename... Ts>
using is_lua_number = std::conjunction<
    std::disjunction<std::is_same<std::decay_t<Ts>, int64_t>, std::is_same<std::decay_t<Ts>, double>>...>;
template <typename... Ts>
static constexpr bool is_lua_number_v = is_lua_number<Ts...>::value;

template <typename T>
inline constexpr std::size_t size_of_type = 0;
template <typename T>
    requires requires { std::tuple_size<T>::value; }
inline constexpr std::size_t size_of_type<T> = std::tuple_size_v<T>;

using rand_generator = std::mt19937_64;
using seed_type = rand_generator::result_type;

}  // namespace lua

#endif