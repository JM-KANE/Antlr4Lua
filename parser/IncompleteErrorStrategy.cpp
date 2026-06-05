#include "IncompleteErrorStrategy.h"
using namespace lua;

antlr4::Token* lua::IncompleteErrorStrategy::recoverInline(antlr4::Parser* recognizer)
{
    CheckComplete(recognizer);
    return DefaultErrorStrategy::recoverInline(recognizer);
}

void lua::IncompleteErrorStrategy::sync(antlr4::Parser* recognizer)
{
    CheckComplete(recognizer);
    DefaultErrorStrategy::sync(recognizer);
}

bool lua::IncompleteErrorStrategy::InComplete() const
{
    return incomplete;
}

bool lua::IncompleteErrorStrategy::CheckComplete(antlr4::Parser* recognizer)
{
    size_t nextTokenType = recognizer->getInputStream()->LA(2);
    incomplete = nextTokenType == antlr4::Token::EOF;
    return incomplete;
}
