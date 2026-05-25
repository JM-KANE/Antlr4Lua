#ifndef _REF_MAP_H
#define _REF_MAP_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace lua
{

template <typename T>
struct ref_hash
{
    using argument_type = std::reference_wrapper<T>;
    using result_type = size_t;

    auto operator()(const argument_type& c) const
    {
        return std::hash<std::decay_t<T>>()(c.get());
    }
};
template <typename T>
struct ref_eq
{
    using argument_type = std::reference_wrapper<T>;

    constexpr auto operator()(const argument_type& c1, const argument_type& c2) const
    {
        return std::addressof(c1.get()) == std::addressof(c2.get()) || std::equal_to<T>()(c1.get(), c2.get());
    }
};
template <typename K>
using ref_set = std::unordered_set<std::reference_wrapper<K>, ref_hash<K>, ref_eq<K>>;
template <typename K, typename T>
using ref_map = std::unordered_map<std::reference_wrapper<K>, T, ref_hash<K>, ref_eq<K>>;

using string_ref_set = ref_set<const std::string>;
template <typename T>
using string_ref_map = ref_map<const std::string, T>;

using any_type = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;

}  // namespace lua
#endif
