#ifndef _ERROR_COLLECTOR_H
#define _ERROR_COLLECTOR_H

#include "antlr4-runtime.h"

namespace lua
{
struct SyntaxError
{
    size_t line;
    size_t charPositionInLine{};
    std::string msg;
    antlr4::Token* offendingSymbol;

    SyntaxError(antlr4::Token* sym, size_t _line, std::string msg, size_t charInline = 0);

    std::string Msg() const;
};

class ErrorCollector : public antlr4::BaseErrorListener
{
public:
    void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line,
                     size_t charPositionInLine, const std::string& msg, std::exception_ptr e) override;

    std::vector<SyntaxError>& GetErrors()
    {
        return errors_;
    }
    bool HasErrors() const
    {
        return !errors_.empty();
    }
    void Clear()
    {
        errors_.clear();
    }

    void CompileError(antlr4::Token* offendingSymbol, size_t line, std::string msg);

private:
    std::vector<SyntaxError> errors_;
};

}  // namespace lua

#endif
