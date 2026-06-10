#include "DynamicLibrary.h"

namespace lua
{

#ifdef _WIN32
auto loadLib(const char* path)
{
    return LoadLibrary(TEXT(path));
}
constexpr auto loadLibGlb = loadLib;
constexpr auto getFunction = GetProcAddress;
constexpr auto closeLib = FreeLibrary;
constexpr auto libError = GetLastError;
#else
auto loadLib(const char* path)
{
    return dlopen(path, RTLD_NOW);
}
auto loadLibGlb(const char* path)
{
    return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
}
constexpr auto getFunction = dlsym;
constexpr auto closeLib = dlclose;
constexpr auto libError = dlerror;
#endif

}  // namespace lua

using namespace lua;

DynamicLibrary::DynamicLibrary(const std::string& path, uint8_t state)
{
    handle = 1 == state ? loadLibGlb(path.c_str()) : loadLib(path.c_str());
    if (!handle)
        error = libError();
}

DynamicLibrary::~DynamicLibrary()
{
    // TODO close as gcobject
    // if (handle)
    //     closeLib(handle);
}

std::string& lua::DynamicLibrary::Error()
{
    return error;
}

Function* lua::DynamicLibrary::GetFunction(const std::string& name)
{
    auto func = (Function*)getFunction(handle, name.c_str());
    if (!func)
        error = libError();
    return func;
}

handle_type lua::DynamicLibrary::Release()
{
    auto res = handle;
    handle = {};
    return res;
}
