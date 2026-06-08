#include "package.h"
using namespace lua;
using namespace stdlib;
#include "../State.h"
#include "../conf.h"

namespace
{
constexpr FuncReg<8> pkgFuncs{
    pair_type{"config", nullptr},
    {"cpath", nullptr},
    {"loaded", nullptr},
    {"loadlib", package::Loadlib},
    {"path", nullptr},
    {"preload", nullptr},
    {"searchers", nullptr},
    {"searchpath", package::Searchpath},
};
constexpr FuncReg<1> llFuncs{pair_type{"require", package::Require}};

void replace_all(std::string& str, const std::string& from, const std::string& to)
{
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos)
    {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
}

struct path_iterator
{
    const std::string* paths{};
    const std::string* dirSep{};
    size_t start{std::string::npos};
    size_t end{std::string::npos};
    path_iterator(const std::string& str, const std::string& sep) : paths(&str), dirSep(&sep), end(str.find(sep))
    {
    }
    auto& operator++()
    {
        if (std::string::npos == end)
            paths = {};
        else
        {
            start = end;
            end = paths->find(*dirSep, start);
        }
        return *this;
    }
    bool operator==(std::default_sentinel_t) const
    {
        return !paths;
    }
    std::string_view operator*() const
    {
        return {paths->begin() + start, paths->begin() + end};
    }
};

std::array<std::string, 2> searchPath(const std::string& paths, const std::string& name, const std::string& sep,
                                      const std::string& dirSep)
{
    auto nameSep = name;
    replace_all(nameSep, sep, dirSep);
    std::string err;
    for (path_iterator it(paths, dirSep); it != std::default_sentinel_t(); ++it)
    {
        std::string path(*it);
        auto pos = path.find(conf::LUA_PATH_MARK);
        path.replace(pos, 1, nameSep);
        if (std::filesystem::exists(path))
        {
            return {std::move(path), ""};
        }
        if (!err.empty())
            err += "\n\t";
        err += "no file '" + path + "'";
    }
    return {"", std::move(err)};
}

void findLoader(State* ls, const std::string& name)
{

    if ((ls->GetField(LuaUpvalueIndex(1), "searchers") != cv::type::LUA_TTABLE))
    {
        ls->Error2("'package.searchers' must be a table");
    }

    auto errMsg = "module '" + name + "' not found:";

    for (auto i = int64_t(1);; i++)
    {
        if (ls->RawGetI(3, i) == cv::type::LUA_TNIL)
        {
            ls->Pop(1);
            ls->Error2(errMsg.c_str());
        }
        ls->PushString(name);
        ls->Call(1, 2);
        if (ls->IsFunction(-2))
        {
            return;
        }
        else if (ls->IsString(-2))
        {
            ls->Pop(1);
            errMsg += ls->CheckString(-1);
        }
        else
        {
            ls->Pop(2);
        }
    }
}

int32_t preloadSearch(State* ls)
{
    auto name = ls->CheckString(1);
    ls->GetField(cv::LUA_REGISTRYINDEX, str::LUA_PRELOAD_TABLE);
    if (ls->GetField(-1, name) == cv::type::LUA_TNIL)
    {
        ls->PushString("\n\tno field package.preload['" + name + "']");
    }
    return 1;
}

int32_t luaSearcher(State* ls)
{
    auto name = ls->CheckString(1);
    ls->GetField(LuaUpvalueIndex(1), "path");
    auto [path, ok] = ls->ToStringX(-1);
    if (!ok)
    {
        ls->Error2("'package.path' must be a string");
    }

    auto [filename, errMsg] = searchPath(name, path, ".", conf::LUA_DIRSEP);
    if (!errMsg.empty())
    {
        ls->PushString(errMsg);
        return 1;
    }

    if (ls->LoadFile(filename) == TStatus::LUA_OK)
    {
        ls->PushString(filename);
        return 2;
    }
    else
    {
        return ls->Error2("error loading module '%s' from file '%s':\n\t%s", ls->CheckString(1).c_str(),
                          filename.c_str(), ls->CheckString(-1).c_str());
    }
}
int32_t cSearch(State* ls)
{
    return 0;
}
int32_t allSearch(State* ls)
{
    return 0;
}

void createSearchersTable(State* ls)
{
    std::array serachers{preloadSearch, luaSearcher, cSearch, allSearch};
    ls->CreateTable(serachers.size(), 0);
    for (size_t i = 0; i < serachers.size(); i++)
    {
        ls->PushValue(-2);
        ls->PushFuncClosure(serachers[i], 1);
        ls->RawSetI(-2, int64_t(i + 1));
    }
    ls->SetField(-2, "searchers");
}

std::string GetPath()
{
    auto path = []() -> char*
    {
        if (auto path = std::getenv("LUA_PATH_5_3"))
            return path;
        else if (auto path = std::getenv("LUA_PATH"))
            return path;
        else
            return nullptr;
    }();
    if (path)
    {
        if (conf::ENV_PATH_SEP_STR != ';')
        {
            std::string str(path);
            std::replace(std::begin(str), std::end(str), ':', ';');
            return str;
        }
        return path;
    }
    else
        return conf::LUA_PATH_DEFAULT.data();
}
std::string GetCPath()
{
    auto path = []() -> char*
    {
        if (auto path = std::getenv("LUA_CPATH_5_3"))
            return path;
        else if (auto path = std::getenv("LUA_CPATH"))
            return path;
        else
            return nullptr;
    }();
    if (path)
    {
        if (conf::ENV_PATH_SEP_STR != ';')
        {
            std::string str(path);
            std::replace(std::begin(str), std::end(str), ':', ';');
            return str;
        }
        return path;
    }
    else
        return conf::LUA_CPATH_DEFAULT.data();
}

}  // namespace

int32_t lua::stdlib::OpenPackageLib(State* ls)
{
    ls->NewLib(pkgFuncs);
    createSearchersTable(ls);

    ls->PushString(GetPath());
    ls->SetField(-2, "path");
    ls->PushString(GetCPath());
    ls->SetField(-2, "cpath");

    constexpr auto config = conf::concat_sep<'\n'>(conf::LUA_DIRSEP, conf::LUA_PATH_SEP, conf::LUA_PATH_MARK,
                                                   conf::LUA_EXEC_DIR, conf::LUA_IGMARK);
    std::string confStr = config.data();
    ls->PushString(confStr + '\n');
    ls->SetField(-2, "config");

    ls->GetSubTable(cv::LUA_REGISTRYINDEX, str::LUA_LOADED_TABLE);
    ls->SetField(-2, "loaded");
    ls->GetSubTable(cv::LUA_REGISTRYINDEX, str::LUA_PRELOAD_TABLE);
    ls->SetField(-2, "preload");

    ls->PushGlobalTable();
    ls->PushValue(-2);
    ls->SetFuncs(llFuncs, 1);
    ls->Pop(1);
    return 0;
}

int32_t lua::stdlib::package::Require(State* ls)
{
    auto name = ls->CheckString(1);
    ls->SetTop(1);
    ls->GetField(cv::LUA_REGISTRYINDEX, str::LUA_LOADED_TABLE);
    ls->GetField(2, name);
    if (ls->ToBoolean(-1))
    {
        return 1;
    }

    ls->Pop(1);
    findLoader(ls, name);
    ls->PushString(name);
    ls->Insert(-2);
    ls->Call(2, 1);
    if (!ls->IsNil(-1))
    {
        ls->SetField(2, name);
    }
    if (ls->GetField(2, name) == cv::type::LUA_TNIL)
    {
        ls->PushBoolean(true);
        ls->PushValue(-1);
        ls->SetField(2, name);
    }
    return 1;
}

int32_t lua::stdlib::package::Loadlib(State* ls)
{
    return 0;
}

int32_t lua::stdlib::package::Searchpath(State* ls)
{
    auto name = ls->CheckString(1);
    auto path = ls->CheckString(2);
    auto sep = ls->OptString(3, ".");
    auto rep = ls->OptString(4, conf::LUA_DIRSEP);
    if (auto [filename, errMsg] = searchPath(name, path, sep, rep); errMsg.empty())
    {
        ls->PushString(filename);
        return 1;
    }
    else
    {
        ls->PushNil();
        ls->PushString(errMsg);
        return 2;
    }
}
