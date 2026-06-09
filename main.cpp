#include <iostream>
#include "include/alua.hpp"
using namespace lua;

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, const char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    VirtualMachine vm(argc, argv);
    if (argc > 1)
        vm.Run();
    else
        vm.RunREPL();

    return 0;
}