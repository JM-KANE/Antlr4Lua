#ifndef CONF_H
#define CONF_H

#include <filesystem>
#include <array>

namespace lua
{

template <typename T>
struct length : std::extent<T>
{
};
template <typename T, size_t N>
struct length<std::array<T, N>> : std::extent<T[N]>
{
};
template <typename T>
constexpr auto length_v = length<T>::value;
template <typename... Ts>
constexpr auto total_size = (length_v<Ts> + ...);

namespace conf
{

constexpr const char LUA_DIRSEP[] = {std::filesystem::path::preferred_separator};
constexpr const char LUA_PATH_SEP[] = ";";

template <char SEP, typename... Ts>
constexpr auto concat_sep(const Ts&... strs)
{
    constexpr std::size_t total_len = total_size<Ts...>;
    std::array<char, total_len> result{};
    size_t pos = 0;
    bool first = true;
    (
        [&](const auto& str)
        {
            auto sz = std::size(str);
            if (!first)
            {
                result[pos++] = SEP;
            }
            for (size_t i = 0; i + 1 < sz; i++)
            {
                result[pos++] = str[i];
            }
            first = false;
        }(strs),
        ...);
    return result;
}
template <typename... Ts>
constexpr auto concat(const Ts&... strs)
{
    constexpr std::size_t total_len = total_size<Ts...> - sizeof...(Ts);
    std::array<char, total_len + 1> result{};
    size_t pos = 0;
    (
        [&](const auto& str)
        {
            auto sz = std::size(str);
            for (size_t i = 0; i + 1 < sz; i++)
            {
                result[pos++] = str[i];
            }
        }(strs),
        ...);
    return result;
}
template <typename... Ts>
constexpr auto concat_dir(const Ts&... paths)
{
    return concat_sep<LUA_DIRSEP[0], Ts...>(paths...);
}
template <typename... Ts>
constexpr auto concat_path(const Ts&... paths)
{
    return concat_sep<LUA_PATH_SEP[0], Ts...>(paths...);
}

constexpr const char LUA_PATH_MARK[] = "?";
constexpr const char LUA_EXEC_DIR[] = "!";
constexpr const char LUA_VDIR[] = "5.3";
constexpr const char LUA_IGMARK[] = "-";

#if defined(_WIN32)
constexpr auto ENV_PATH_SEP_STR = ';';
constexpr auto LUA_LDIR = concat_dir(LUA_EXEC_DIR, "lua");
constexpr auto LUA_CDIR = concat_dir(LUA_EXEC_DIR);
constexpr auto LUA_SHRDIR = concat_dir(LUA_EXEC_DIR, "..", "share", "lua", LUA_VDIR);
constexpr auto LUA_PATH_DEFAULT = concat_path(
    concat_dir(LUA_LDIR, concat(LUA_PATH_MARK, ".lua")), concat_dir(LUA_LDIR, concat_dir(LUA_PATH_MARK, "init.lua")),
    concat_dir(LUA_CDIR, concat(LUA_PATH_MARK, ".lua")), concat_dir(LUA_CDIR, concat_dir(LUA_PATH_MARK, "init.lua")),
    concat_dir(LUA_SHRDIR, concat(LUA_PATH_MARK, ".lua")),
    concat_dir(LUA_SHRDIR, concat_dir(LUA_PATH_MARK, "init.lua")), concat_dir(".", concat(LUA_PATH_MARK, ".lua")),
    concat_dir(".", concat_dir(LUA_PATH_MARK, "init.lua")));
constexpr auto LUA_CPATH_DEFAULT =
    concat_path(concat_dir(LUA_CDIR, concat(LUA_PATH_MARK, ".dll")),
                concat_dir(LUA_CDIR, "..", "lib", "lua", LUA_VDIR, concat(LUA_PATH_MARK, ".dll")),
                concat_dir(LUA_CDIR, "loadall.dll"), concat_dir(".", concat(LUA_PATH_MARK, ".dll")));
#else
constexpr auto ENV_PATH_SEP_STR = ':';
constexpr auto LUA_ROOT = concat_dir("", "usr", "local");
constexpr auto LUA_LDIR = concat_dir(LUA_ROOT, "share", "lua", LUA_VDIR);
constexpr auto LUA_CDIR = concat_dir(LUA_ROOT, "lib", "lua", LUA_VDIR);
constexpr auto LUA_PATH_DEFAULT = concat_path(
    concat_dir(LUA_LDIR, concat(LUA_PATH_MARK, ".lua")), concat_dir(LUA_LDIR, concat_dir(LUA_PATH_MARK, "init.lua")),
    concat_dir(LUA_CDIR, concat(LUA_PATH_MARK, ".lua")), concat_dir(LUA_CDIR, concat_dir(LUA_PATH_MARK, "init.lua")),
    concat_dir(".", concat(LUA_PATH_MARK, ".lua")), concat_dir(".", concat_dir(LUA_PATH_MARK, "init.lua")));
constexpr auto LUA_CPATH_DEFAULT =
    concat_path(concat_dir(LUA_CDIR, concat(LUA_PATH_MARK, ".so")), concat_dir(LUA_CDIR, "loadall.so"),
                concat_dir(".", concat(LUA_PATH_MARK, ".so")));

#endif

}  // namespace conf

}  // namespace lua

#endif
