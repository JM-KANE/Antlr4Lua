#include "ErrorCollector.h"
using namespace lua;

std::string lua::SyntaxError::Msg() const
{
    return std::to_string(line) + ": " + msg;
}

void ErrorCollector::syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line,
                                 size_t charPositionInLine, const std::string& msg, std::exception_ptr e)
{
    errors_.emplace_back(line, charPositionInLine, msg, offendingSymbol);
}
