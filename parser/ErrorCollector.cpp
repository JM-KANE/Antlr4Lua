#include "ErrorCollector.h"
using namespace lua;

void ErrorCollector::syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line,
                                 size_t charPositionInLine, const std::string& msg, std::exception_ptr e)
{
    errors_.emplace_back(offendingSymbol, line, msg, charPositionInLine);
}

void ErrorCollector::CompileError(antlr4::Token* offendingSymbol, size_t line, std::string msg)
{
    errors_.emplace_back(offendingSymbol, line, std::move(msg));
}
