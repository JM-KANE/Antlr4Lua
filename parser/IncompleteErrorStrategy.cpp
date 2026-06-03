#include "IncompleteErrorStrategy.h"
using namespace lua;

antlr4::Token* lua::IncompleteErrorStrategy::recoverInline(antlr4::Parser* recognizer)
{
    // TODO only do/function/while/if/repeat/for may be incomplete
    incomplete = false;

    //if (recognizer->getCurrentToken()->getType() == antlr4::Token::EOF)
    //{
    //    auto expected = recognizer->getExpectedTokens();
    //    if (!expected.contains(antlr4::Token::EOF))
    //    {
    //        incomplete = true;
    //        throw antlr4::InputMismatchException(recognizer);
    //    }
    //}
    return DefaultErrorStrategy::recoverInline(recognizer);
}

bool lua::IncompleteErrorStrategy::InComplete() const
{
    return incomplete;
}