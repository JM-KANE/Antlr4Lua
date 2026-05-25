/* eslint-disable no-underscore-dangle */
/* cspell: ignore antlr, longlong, ULONGLONG, MAXDB */

#ifndef _LUA_PARSER_BASE_H
#define _LUA_PARSER_BASE_H

#include "antlr4-runtime.h"

namespace lua
{
class LuaParserBase : public antlr4::Parser
{

protected:
    LuaParserBase(antlr4::TokenStream* input);

public:
    bool IsFunctionCall();
};
}  // namespace lua
#endif