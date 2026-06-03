#include <iostream>
#include "vm/VirtualMachine.h"
using namespace lua;

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, const char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc > 1)
    {
        VirtualMachine vm(argc, argv);
        vm.Run();
    }
    else
    {
        /* code */
    }

    return 0;
}