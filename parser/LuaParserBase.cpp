/* eslint-disable no-underscore-dangle */
/* cspell: ignore antlr, longlong, ULONGLONG, MAXDB */

#include "antlr4-runtime.h"
#include "../generated/LuaParser.h"
using namespace lua;

LuaParserBase::LuaParserBase(antlr4::TokenStream* input) : Parser(input)
{
    removeErrorListeners();
}

void lua::LuaParserBase::PushBlock(LuaRuleContext* n)
{
    blocksStack.push_back(n);
}

void lua::LuaParserBase::PopBlock()
{
    blocksStack.pop_back();
}

std::pair<const std::string*, bool> lua::LuaParserBase::AddLabel(std::string&& l)
{
    auto b = blocksStack.back();
    auto [it, ok] = blockLabels[b].emplace(std::move(l));
    return {&(*it), ok};
}

void lua::LuaParserBase::CheckLabel(std::string&& l)
{
    auto it =
        std::find_if(std::next(blocksStack.rbegin()), blocksStack.rend(), [&](const auto b) { return HasLabel(b, l); });
    const std::string* name{&l};
    if (it == blocksStack.rend())
    {
        if (auto [nameAdd, ok] = AddLabel(std::move(l)); ok)
        {
            return;
        }
        else
            name = nameAdd;
    }
    throw antlr4::RecognitionException("label '" + *name + "' already defined", this, getInputStream(), getContext(),
                                       getCurrentToken());
}

bool LuaParserBase::IsFunctionCall()
{
    antlr4::BufferedTokenStream* stream = static_cast<antlr4::BufferedTokenStream*>(_input);
    auto la = stream->LT(1);
    if (la->getType() != LuaParser::NAME)
        return false;
    la = stream->LT(2);
    if (la->getType() == LuaParser::OP)
        return false;
    return true;
}

void LuaParserBase::SetREPL()
{
    interactive = true;
}

bool lua::LuaParserBase::IsREPL() const
{
    return interactive;
}

bool LuaParserBase::HasLabel(const std::string& l) const
{
    return HasLabel(blocksStack.back(), l);
}

bool lua::LuaParserBase::HasLabel(LuaRuleContext* b, const std::string& l) const
{
    if (auto it = blockLabels.find(b); it != blockLabels.end())
    {
        return it->second.count(l);
    }
    return false;
}

const LuaParserBase::block_labels* lua::LuaParserBase::GetLabels(LuaRuleContext* b) const
{
    auto it = blockLabels.find(b);
    return it == blockLabels.end() ? nullptr : &it->second;
}