/* eslint-disable no-underscore-dangle */
/* cspell: ignore antlr, longlong, ULONGLONG, MAXDB */

#ifndef _LUA_PARSER_BASE_H
#define _LUA_PARSER_BASE_H

#include "ErrorCollector.h"

namespace lua
{
class LuaParserBase : public antlr4::Parser
{

protected:
    bool interactive = false;
    LuaParserBase(antlr4::TokenStream* input);

public:
    bool IsFunctionCall();
    void SetREPL();
    bool IsREPL() const;
};
}  // namespace lua
#endif