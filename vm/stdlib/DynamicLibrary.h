#ifndef DYNAMIC_LIBRUARY_H
#define DYNAMIC_LIBRUARY_H

#include "libtype.h"
#ifdef _WIN32
#include <windows.h>
using handle_type = HMODULE;
#else
#include <dlfcn.h>
using handle_type = void*;
#endif

namespace lua
{

class DynamicLibrary
{
private:
    handle_type handle;
    std::string error;

public:
    DynamicLibrary(const std::string& path, uint8_t state);
    ~DynamicLibrary();

    std::string& Error();

    Function* GetFunction(const std::string& name);
    handle_type Release();
};

}  // namespace lua

#endif