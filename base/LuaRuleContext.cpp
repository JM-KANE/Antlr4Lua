#include "LuaParser.h"
using namespace lua;

size_t lua::LuaRuleContext::getAltNumber() const
{
    if (!_altNum && getRuleIndex() == LuaParser::RuleExp)
    {
        if (dynamic_cast<const LuaParser::NilContext*>(this))
        {
            _altNum = 1;
        }
        else if (dynamic_cast<const LuaParser::FalseContext*>(this))
        {
            _altNum = 2;
        }
        else if (dynamic_cast<const LuaParser::TrueContext*>(this))
        {
            _altNum = 3;
        }
        else if (dynamic_cast<const LuaParser::Number_Context*>(this))
        {
            _altNum = 4;
        }
        else if (dynamic_cast<const LuaParser::String_Context*>(this))
        {
            _altNum = 5;
        }
        else if (dynamic_cast<const LuaParser::VarargexpContext*>(this))
        {
            _altNum = 6;
        }
        else if (dynamic_cast<const LuaParser::Functiondef_Context*>(this))
        {
            _altNum = 7;
        }
        else if (dynamic_cast<const LuaParser::Prefixexp_Context*>(this))
        {
            _altNum = 8;
        }
        else if (dynamic_cast<const LuaParser::TableconstructorContext*>(this))
        {
            _altNum = 9;
        }
        else if (dynamic_cast<const LuaParser::ExponentiationContext*>(this))
        {
            _altNum = 10;
        }
        else if (dynamic_cast<const LuaParser::UnaryContext*>(this))
        {
            _altNum = 11;
        }
        else if (dynamic_cast<const LuaParser::MultidivContext*>(this))
        {
            _altNum = 12;
        }
        else if (dynamic_cast<const LuaParser::AddsubContext*>(this))
        {
            _altNum = 13;
        }
        else if (dynamic_cast<const LuaParser::ConcatenationContext*>(this))
        {
            _altNum = 14;
        }
        else if (dynamic_cast<const LuaParser::RelationContext*>(this))
        {
            _altNum = 15;
        }
        else if (dynamic_cast<const LuaParser::AndContext*>(this))
        {
            _altNum = 16;
        }
        else if (dynamic_cast<const LuaParser::OrContext*>(this))
        {
            _altNum = 17;
        }
        else if (dynamic_cast<const LuaParser::BitwiseContext*>(this))
        {
            _altNum = 18;
        }
    }
    return _altNum;
}

void lua::LuaRuleContext::setAltNumber(size_t altNumber)
{
    _altNum = altNumber;
}
