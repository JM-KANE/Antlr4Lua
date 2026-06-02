#include "ErrorCollector.h"
using namespace lua;

SyntaxError::SyntaxError(antlr4::Token* sym, size_t _line, std::string msg, size_t charInline)
    : line(_line),
      msg(std::move(msg)),
      offendingSymbol(sym),
      charPositionInLine(charInline)
{
}

std::string lua::SyntaxError::Msg() const
{
    return std::to_string(line) + ": " + msg;
}

void ErrorCollector::syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line,
                                 size_t charPositionInLine, const std::string& msg, std::exception_ptr e)
{
    errors_.emplace_back(offendingSymbol, line, msg, charPositionInLine);
}

void ErrorCollector::CompileError(antlr4::Token* offendingSymbol, size_t line, std::string msg)
{
    errors_.emplace_back(offendingSymbol, line, std::move(msg));
}
