
#ifndef _LUA_RULE_CONTEXT_H
#define _LUA_RULE_CONTEXT_H

#include "antlr4-runtime.h"

namespace lua
{
class LuaRuleContext : public antlr4::ParserRuleContext
{
public:
    enum class Stat
    {
        Semistat = 1,
        Assign,
        Functioncall_,
        Label_,
        Break,
        Goto,
        Do,
        While,
        Repeat,
        If,
        Fornumerical,
        Forgeneric,
        Funcnamedef,
        Localfunc,
        Vardecl
    };
    enum class Exp
    {
        Nil = 1,
        False,
        True,
        Number_,
        String_,
        Varargexp,
        Functiondef_,
        Prefixexp_,
        Tableconstructor_,
        Exponentiation,
        Unary,
        Multidiv,
        A,
        Concatenation,
        Relation,
        And,
        Or,
        Bitwise
    };
    enum class Var
    {
        Normalvar = 1,
        Indextable
    };
    enum class Prefixexp
    {
        Nameindex = 1,
        Callindex,
        Expindex
    };
    enum class Member
    {
        Index = 1,
        Access
    };
    enum class Args
    {
        Normalarg = 1,
        Tablearg,
        Stringarg
    };
    enum class Parlist
    {
        Nameparlist = 1,
        Varparlist,
        Emptyparlist
    };
    enum class Field
    {
        Indexedfield = 1,
        Namedfield,
        Expfield
    };
    enum class Fieldsep
    {
        Semi = 1,
        Colon
    };
    enum class Number
    {
        Int = 1,
        Hex,
        Float,
        Hexfloat
    };
    enum class String
    {
        Normalstring = 1,
        Charstring,
        Longstring
    };
    enum class Functionargs
    {
        Args_ = 1,
        Nameargs
    };

private:
    mutable size_t _altNum = antlr4::atn::ATN::INVALID_ALT_NUMBER;

    void FixAltNum() const;

public:
    using antlr4::ParserRuleContext::ParserRuleContext;

    size_t getAltNumber() const override;
    void setAltNumber(size_t altNumber) override;
};

}  // namespace lua
#endif