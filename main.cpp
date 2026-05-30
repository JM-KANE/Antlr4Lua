#include <iostream>
#include "vm/VirtualMachine.h"
using namespace lua;

int main(int argc, const char* argv[])
{
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