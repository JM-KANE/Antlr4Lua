#ifndef INCOMPLETE_ERROR_STRATEGY_H
#define INCOMPLETE_ERROR_STRATEGY_H

#include "antlr4-runtime.h"

namespace lua
{

class IncompleteErrorStrategy : public antlr4::DefaultErrorStrategy
{
public:
    bool incomplete = true;

    antlr4::Token* recoverInline(antlr4::Parser* recognizer) override;

    void reset()
    {
        incomplete = true;
    }

    bool InComplete() const;
};
}

#endif