#include "LuaParser.h"
using namespace lua;

size_t lua::LuaRuleContext::getAltNumber() const
{
    return _altNum;
}

void lua::LuaRuleContext::setAltNumber(size_t altNumber)
{
    _altNum = altNumber;
}
