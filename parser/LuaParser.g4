// $antlr-format alignTrailingComments true, columnLimit 150, minEmptyLines 1, maxEmptyLinesToKeep 1, reflowComments false, useTab false
// $antlr-format allowShortRulesOnASingleLine false, allowShortBlocksOnASingleLine true, alignSemicolons hanging, alignColons hanging

parser grammar LuaParser;

options {
    tokenVocab = LuaLexer;
    superClass = LuaParserBase;
    contextSuperClass = LuaRuleContext;
}

@header {
#include "LuaParserBase.h"
#include "LuaRuleContext.h"
}

@parser::members {
    int loopDepth = 0;
    std::vector<bool> isVara{true};
}

start_
    : chunk chunkend
    ;

chunk
    : block                 # normalblock
    | {interactive}? exp    # evalexpblock
    |                       # emptyblock
    ;

chunkend
    : EOF
    ;

block
    : {PushBlock($ctx);} stat* retstat? {PopBlock();}
    ;

stat
    : ';'                                                                       
        # semistat
    | varlist '=' explist                                                       
        # assign
    | functioncall                                                              
        # functioncall_
    | label                                                                     
        # label_
    | 'break' {if (0 == loopDepth) notifyErrorListeners("break outside loop");}
        # break
    | 'goto' NAME                                                               
        # goto
    | 'do' block 'end'                                                          
        # do
    | 'while' exp 'do' {++loopDepth;} block {--loopDepth;} 'end'                                              
        # while
    | 'repeat' {++loopDepth;} block {--loopDepth;} 'until' exp                                                
        # repeat
    | 'if' exp 'then' block ('elseif' exp 'then' block)* ('else' block)? 'end'  
        # if
    | 'for' NAME '=' exp ',' exp (',' exp)? 'do' {++loopDepth;} block {--loopDepth;} 'end'                    
        # fornumerical
    | 'for' namelist 'in' explist 'do' {++loopDepth;} block {--loopDepth;} 'end'                              
        # forgeneric
    | 'function' funcname funcbody                                              
        # funcnamedef
    | 'local' 'function' NAME funcbody                                          
        # localfunc
    | 'local' attnamelist ('=' explist)?                                        
        # vardecl
    ;

attnamelist
    : NAME attrib (',' NAME attrib)*
    ;

attrib
    : ('<' NAME '>')?
    ;

retstat
    : 'return' explist? ';'?
    ;

label
    : '::' NAME {CheckLabel($NAME->getText());} '::'
    ;

funcname
    : NAME ('.' NAME)* (':' NAME)?
    ;

varlist
    : var (',' var)*
    ;

namelist
    : NAME (',' NAME)*
    ;

explist
    : exp (',' exp)*
    ;

exp
    : 'nil'                             # nil
    | 'false'                           # false
    | 'true'                            # true
    | number                            # number_
    | string                            # string_
    | '...' {if (!isVara.back()) notifyErrorListeners("cannot use '...' outside a vararg function"); }                            
        # varargexp
    | functiondef                       # functiondef_
    | prefixexp                         # prefixexp_
    | tableconstructor                  # tableconstructor_
    | <assoc = right> exp ('^') exp     # exponentiation
    | uop exp                           # unary 
    | exp bop1 exp                      # multidiv
    | exp bop2 exp                      # addsub
    | <assoc = right> exp ('..') exp    # concatenation 
    | exp bop3 exp                      # relation
    | exp ('and') exp                   # and
    | exp ('or') exp                    # or
    | exp bop4 exp                      # bitwise 
    ;

uop
    : 'not' # not
    | '#'   # len
    | '-'   # unm
    | '~'   # bnot
    ;

bop1
    : '*'   # mul
    | '/'   # div
    | '%'   # mod
    | '//'  # idiv
    ;
bop2
    : '+'  # add
    | '-'  # sub
    ;
bop3
    : '<'   # lt
    | '>'   # gt
    | '<='  # le
    | '>='  # ge
    | '~='  # ne
    | '=='  # eq
    ;
bop4
    : '&'   # band
    | '|'   # bor
    | '~'   # bxor
    | '<<'  # shl
    | '>>'  # shr
    ;
    
// var ::=  Name | prefixexp '[' exp ']' | prefixexp '.' Name 
var
    : NAME              # normalvar
    | prefixexp member  # indextable
    ;

// prefixexp ::= var | functioncall | '(' exp ')'
prefixexp
    : { IsFunctionCall() }? NAME member*    # nameindex
    | functioncall member*                  # callindex
    | '(' exp ')' member*                   # expindex
    ;

// member ::= '[' exp ']' | '.' NAME
member 
    : '[' exp ']'   # index
    | '.' NAME      # access
    ;

// functioncall ::=  prefixexp args | prefixexp ':' NAME args;
functioncall
    : ( NAME | '(' exp ')' ) callpart+
    ;

callpart
    : member* functionargs
    ;

functionargs
    : args              # args_
    | ':' NAME args     # nameargs
    ;

args
    : '(' explist? ')'  # normalarg
    | tableconstructor  # tablearg
    | string            # stringarg
    ;

functiondef
    : 'function' funcbody
    ;

funcbody
    : '(' parlist ')' block { isVara.pop_back(); } 'end'
    ;

/* lparser.c says "is 'parlist' not empty?"
 * That code does so by checking la(1) == ')'.
 * This means that parlist can derive empty.
 */
parlist
    : namelist { isVara.emplace_back(false); } (',' '...' { isVara.back() = true; })? 
        # nameparlist
    | '...' { isVara.emplace_back(true); }
        # varparlist
    |       { isVara.emplace_back(false); }  
        # emptyparlist
    ;

tableconstructor
    : '{' fieldlist? '}'
    ;

fieldlist
    : field (fieldsep field)* fieldsep?
    ;

field
    : '[' exp ']' '=' exp   # indexedfield
    | NAME '=' exp          # namedfield
    | exp                   # expfield
    ;

fieldsep
    : ','   # semi
    | ';'   # colon
    ;

number
    : INT       # int
    | HEX       # hex
    | FLOAT     # float
    | HEX_FLOAT # hexfloat
    ;

string
    : NORMALSTRING  # normalstring
    | CHARSTRING    # charstring
    | LONGSTRING    # longstring
    ;