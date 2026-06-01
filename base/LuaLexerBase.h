#ifndef _LUA_LEXER_BASE_H
#define _LUA_LEXER_BASE_H

#include "ErrorCollector.h"

namespace lua
{
class LuaLexerBase : public antlr4::Lexer
{
private:
    antlr4::CharStream* _input;
    size_t start_line{1};
    size_t start_col{1};

public:
    LuaLexerBase(antlr4::CharStream* input);
    void HandleComment();
    bool IsLine1Col0();
    void read_long_string(antlr4::CharStream* cs, int sep);
    int skip_sep(antlr4::CharStream* cs);
};
}  // namespace lua
#endif