#ifndef INCOMPLETE_ERROR_STRATEGY_H
#define INCOMPLETE_ERROR_STRATEGY_H

#include "antlr4-runtime.h"

namespace lua
{

class IncompleteErrorStrategy : public antlr4::DefaultErrorStrategy
{
public:
    bool incomplete = false;

    antlr4::Token* recoverInline(antlr4::Parser* recognizer) override;

    void sync(antlr4::Parser* recognizer) override;

    void reset()
    {
        incomplete = false;
    }

    bool InComplete() const;
    bool CheckComplete(antlr4::Parser* recognizer);
};
}

#endif