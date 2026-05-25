
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
        Semistat,
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
        Nil,
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
        Normalvar,
        Indextable
    };
    enum class Prefixexp
    {
        Nameindex,
        Callindex,
        Expindex
    };
    enum class Member
    {
        Index,
        Access
    };
    enum class Args
    {
        Normalarg,
        Tablearg,
        Stringarg
    };
    enum class Parlist
    {
        Nameparlist,
        Varparlist,
        Emptyparlist
    };
    enum class Field
    {
        Indexedfield,
        Namedfield,
        Expfield
    };
    enum class Fieldsep
    {
        Semi,
        Colon
    };
    enum class Number
    {
        Int,
        Hex,
        Float,
        Hexfloat
    };
    enum class String
    {
        Normalstring,
        Charstring,
        Longstring
    };
    enum class Functionargs
    {
        Args_,
        Nameargs
    };

private:
    size_t _altNum = antlr4::atn::ATN::INVALID_ALT_NUMBER;

public:
    using antlr4::ParserRuleContext::ParserRuleContext;

    size_t getAltNumber() const override;
    void setAltNumber(size_t altNumber) override;
};

}  // namespace lua
#endif