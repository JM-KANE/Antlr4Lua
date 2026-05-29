#include <iostream>
#include "LuaLexer.h"
#include "LuaParser.h"

#include "code_gen/CodeGen.h"
#include "vm/VirtualMachine.h"
using namespace antlr4;
using namespace std;
using namespace lua;

int main(int argc, const char* argv[])
{
    const char* filepath = argv[1];
    std::ifstream ifs;
    ifs.open(filepath);
    ANTLRInputStream input(ifs);
    LuaLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    LuaParser parser(&tokens);

    auto chunk = parser.chunk();
    if (parser.getNumberOfSyntaxErrors() > 0)
    {
        cout << "lua file syntax error" << endl;
        return 0;
    }

    for (auto t : tokens.getTokens())
    {
        cout << t->toString() << endl;
    }

    auto cg = CodeGen();
    auto p = cg.Generate(chunk);
    VirtualMachine vm(argc, argv);
    vm.Run(p);
    ifs.close();
    return 0;
}