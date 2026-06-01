#include "ErrorCollector.h"
using namespace lua;

void ErrorCollector::syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line,
                               size_t charPositionInLine,
                 const std::string& msg, std::exception_ptr e)
{
    errors_.emplace_back(line, charPositionInLine, msg, offendingSymbol);
}