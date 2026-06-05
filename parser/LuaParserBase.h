/* eslint-disable no-underscore-dangle */
/* cspell: ignore antlr, longlong, ULONGLONG, MAXDB */

#ifndef _LUA_PARSER_BASE_H
#define _LUA_PARSER_BASE_H

#include "ErrorCollector.h"
#include <string>
namespace lua
{
class LuaRuleContext;
class LuaParserBase : public antlr4::Parser
{
public:
using block_labels = std::unordered_set<std::string>;  

protected:
    bool interactive = false;

    std::vector<LuaRuleContext*> blocksStack;
    std::unordered_map<LuaRuleContext*, block_labels> blockLabels;

    LuaParserBase(antlr4::TokenStream* input);

    void PushBlock(LuaRuleContext* n);
    void PopBlock();
    std::pair<const std::string*, bool> AddLabel(std::string&& l);
    void CheckLabel(std::string&& l);

public:

    bool IsFunctionCall();

    void SetREPL();
    bool IsREPL() const;

    bool HasLabel(const std::string& l) const;
    bool HasLabel(LuaRuleContext*  b, const std::string& l) const;

    const block_labels* GetLabels(LuaRuleContext* b) const;
};
}  // namespace lua     
#endif