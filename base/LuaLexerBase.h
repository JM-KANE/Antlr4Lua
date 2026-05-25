#ifndef _LUA_LEXER_BASE_H
#define _LUA_LEXER_BASE_H

#include "antlr4-runtime.h"

namespace lua
{
class LuaLexerBase : public antlr4::Lexer
{
private:
    antlr4::CharStream* _input;
    size_t start_line;
    size_t start_col;

public:
    LuaLexerBase(antlr4::CharStream* input);
    void HandleComment();
    bool IsLine1Col0();
    void read_long_string(antlr4::CharStream* cs, int sep);
    int skip_sep(antlr4::CharStream* cs);
};
}  // namespace lua
#endif