#include "package.h"
using namespace lua;
using namespace stdlib;
#include "../State.h"
#include "../conf.h"

#include "DynamicLibrary.h"
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
    size_t start{std::string::npos};
    size_t end{std::string::npos};
    path_iterator(const std::string& str) : paths(&str), start(0), end(str.find(conf::LUA_PATH_SEP[0]))
    {
    }
    auto& operator++()
    {
        if (std::string::npos == end)
            paths = {};
        else
        {
            start = end + 1;
            end = paths->find(conf::LUA_PATH_SEP[0], start);
        }
        return *this;
    }
    bool operator==(std::default_sentinel_t) const
    {
        return !paths;
    }
    std::string_view operator*() const
    {
        auto it_end = std::string::npos == end ? paths->end() : paths->begin() + end;
        return {paths->begin() + start, it_end};
    }
};

std::array<std::string, 2> searchPath(const std::string& paths, const std::string& name, const std::string& sep,
                                      const std::string& dirSep)
{
    auto nameSep = name;
    replace_all(nameSep, sep, dirSep);
    std::string err;
    for (path_iterator it(paths); it != std::default_sentinel_t(); ++it)
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

int32_t searchLib(State* ls, const std::string& path, const std::string& name, const std::string* rootName = nullptr)
{
    auto& root = rootName ? *rootName : name;
    auto [filename, errMsg] = searchPath(path, root, ".", conf::LUA_DIRSEP);
    if (!errMsg.empty())
    {
        ls->PushString(std::move(errMsg));
        return 1;
    }
    DynamicLibrary lib(filename, 0);
    auto LibError = [&]()
    {
        auto msg = "error loading module '" + root + "' from file '" + filename + "':\n\t";
        msg += lib.Error();
        return ls->Error2(msg.c_str());
    };

    errMsg = std::move(lib.Error());
    if (!errMsg.empty())
    {
        return LibError();
    }
    auto nameOpen = name;
    replace_all(nameOpen, ".", "_");
    nameOpen = "luaopen_" + nameOpen;
    if (auto f = lib.GetFunction(nameOpen.c_str()))
    {
        ls->PushFunction(f);
        ls->PushString(filename);
        return 2;
    }
    else
    {
        return LibError();
    }
}

bool findLoader(State* ls, const std::string& name)
{
    if ((ls->GetField(LuaUpvalueIndex(1), "searchers") != cv::type::LUA_TTABLE))
    {
        ls->Error2("'package.searchers' must be a table");
    }

    auto errMsg = "module '" + name + "' not found:";

    for (auto i = int64_t(1); i <= 3; i++)
    {
        ls->RawGetI(3, i);
        ls->PushString(name);
        ls->Call(1, 2);
        if (ls->IsFunction(-2))
        {
            return true;
        }
        else if (ls->IsString(-2))
        {
            ls->Pop(1);
            errMsg += "\n\t" + ls->CheckString(-1);
        }
        else
        {
            ls->Pop(2);
        }
    }

    ls->Pop(1);
    ls->PushNil();
    ls->Error2(errMsg.c_str());
    return false;
}

int32_t preloadSearch(State* ls)
{
    auto name = ls->CheckString(1);
    ls->GetField(cv::LUA_REGISTRYINDEX, str::LUA_PRELOAD_TABLE);
    if (ls->GetField(-1, name) == cv::type::LUA_TNIL)
    {
        ls->PushString("no field package.preload['" + name + "']");
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
        return ls->Error2("'package.path' must be a string");
    }

    auto [filename, errMsg] = searchPath(path, name, ".", conf::LUA_DIRSEP);
    if (!errMsg.empty())
    {
        ls->PushString(std::move(errMsg));
        return 1;
    }

    if (ls->LoadFile(filename) == TStatus::LUA_OK)
    {
        ls->PushString(filename);
        return 2;
    }
    else
    {
        std::ostringstream os;
        os << "error loading module '" << ls->CheckString(1) << "' from file '" << filename << "':\n\t";
        ls->Catch(os);
        return ls->Error2(os.str().c_str());
    }
}

int32_t cSearch(State* ls)
{
    auto name = ls->CheckString(1);
    ls->GetField(LuaUpvalueIndex(1), "cpath");
    auto [path, ok] = ls->ToStringX(-1);
    if (!ok)
    {
        return ls->Error2("'package.cpath' must be a string");
    }

    return searchLib(ls, path, name);
}

int32_t allInOneSearch(State* ls)
{
    auto name = ls->CheckString(1);
    auto dot = name.find_first_of('.');
    if (dot == std::string::npos)
    {
        return 0;
    }
    auto rootName = name.substr(0, dot);
    ls->GetField(LuaUpvalueIndex(1), "cpath");
    auto [path, ok] = ls->ToStringX(-1);
    if (!ok)
    {
        return 0;
    }

    return searchLib(ls, path, name, &rootName);
}

void createSearchersTable(State* ls)
{
    std::array serachers{preloadSearch, luaSearcher, cSearch, allInOneSearch};
    ls->CreateTable((int32_t)serachers.size(), 0);
    for (size_t i = 0; i < serachers.size(); i++)
    {
        ls->PushValue(-2);
        ls->PushFuncClosure(serachers[i], 1);
        ls->RawSetI(-2, int64_t(i + 1));
    }
    ls->SetField(-2, "searchers");
}

#ifdef _WIN32
#include <windows.h>
const std::string& GetExecPath()
{
    static std::string execPath;
    if (execPath.empty())
    {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string fullPath(buffer);
        size_t lastSlash = fullPath.find_last_of(conf::LUA_DIRSEP[0]);
        execPath = lastSlash != std::string::npos ? fullPath.substr(0, lastSlash) : "c:\\";
    }
    return execPath;
}
#endif  // #ifdef _WIN32

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
    {
        std::string path = conf::LUA_PATH_DEFAULT.data();
#ifdef _WIN32
        replace_all(path, conf::LUA_EXEC_DIR, GetExecPath());
#endif
        return path;
    }
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
    {
        std::string path = conf::LUA_CPATH_DEFAULT.data();
#ifdef _WIN32
        replace_all(path, conf::LUA_EXEC_DIR, GetExecPath());
#endif
        return path;
    }
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
    return 1;
}

int32_t lua::stdlib::package::Require(State* ls)
{
    auto name = ls->CheckString(1);
    ls->SetTop(1);
    ls->GetField(cv::LUA_REGISTRYINDEX, str::LUA_LOADED_TABLE);
    ls->GetField(2, name);
    if (ls->ToBoolean(-1))
    {
        ls->PushNil();
        return 2;
    }

    ls->Pop(1);
    if (!findLoader(ls, name))
    {
        ls->PushNil();
        return 2;
    }
    auto filename = ls->ToString(-1);
    ls->PushString(name);
    ls->Insert(-2);
    ls->Call(2, 1);
    if (!ls->IsNil(-1))
    {
        ls->SetField(2, name);
        ls->PushString(std::move(filename));
    }
    else if (ls->GetField(2, name) == cv::type::LUA_TNIL)
    {
        ls->PushBoolean(true);
        ls->PushValue(-1);
        ls->SetField(2, name);
        ls->PushString(std::move(filename));
    }
    return 2;
}

int32_t lua::stdlib::package::Loadlib(State* ls)
{
    auto libname = ls->CheckString(1);
    auto name = ls->CheckString(2);
    bool link = "*" == name;
    DynamicLibrary lib(libname, link ? 1 : 0);
    auto errMsg = std::move(lib.Error());
    if (!errMsg.empty())
    {
        ls->PushNil();
        ls->PushString(errMsg);
        return 2;
    }

    if (link)
    {
        ls->PushBoolean(true);
        lib.Release();
        return 1;
    }
    else if (auto func = lib.GetFunction(name))
    {
        ls->PushFunction(func);
        return 1;
    }
    else
    {
        ls->PushNil();
        ls->PushString(lib.Error());
        return 2;
    }
}

int32_t lua::stdlib::package::Searchpath(State* ls)
{
    auto name = ls->CheckString(1);
    auto path = ls->CheckString(2);
    auto sep = ls->OptString(3, ".");
    auto rep = ls->OptString(4, conf::LUA_DIRSEP);
    if (auto [filename, errMsg] = searchPath(path, name, sep, rep); errMsg.empty())
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
