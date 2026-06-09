#ifndef SYNTAX_ERROR_H
#define SYNTAX_ERROR_H

#include <cstdint>
#include <string>

namespace lua
{

struct SyntaxError
{
    size_t line;
    size_t charPositionInLine{};
    std::string msg;
    void* token{};

    SyntaxError(void* t, size_t _line, std::string msg, size_t charInline = 0)
        : line(_line),
          token(t),
          msg(std::move(msg)),
          charPositionInLine(charInline)
    {
    }

    std::string Msg() const
    {
        return std::to_string(line) + ": " + msg;
    }
};
}  // namespace lua

#endif