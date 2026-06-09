#ifndef RUN_EXCEPTION_H
#define RUN_EXCEPTION_H
#include "type.h"
#include "Prototype.h"
#include <iostream>
namespace lua
{
struct TopPrototype;
struct SyntaxError;

struct Exception /*: public std::exception*/
{
    std::string msg;

    Exception(std::string m) : msg(std::move(m))
    {
    }
    virtual TStatus Status() const = 0;
    virtual std::string ToString() const
    {
        return msg;
    }
    virtual ~Exception() = default;
};

struct FileException : public Exception
{
    using Exception::Exception;
    TStatus Status() const override
    {
        return TStatus::LUA_ERRFILE;
    }
    std::string ToString() const override
    {
        return "cannot open " + msg + ": No such file or directory";
    }
};

struct CodeException : Exception
{
    size_t line{};

    CodeException(size_t l, std::string m) : line(l), Exception(std::move(m))
    {
    }
};

struct SyntaxException : public CodeException
{
    std::string shortSource;

    SyntaxException(const TopPrototype* p, size_t l, std::string m)
        : CodeException(l, std::move(m)),
          shortSource{p->ShortSource()}
    {
    }
    SyntaxException(const TopPrototype* p, SyntaxError&& info) : SyntaxException(p, info.line, std::move(info.msg))
    {
    }

    std::string ToString() const override
    {
        auto src = shortSource;
        if (line)
        {
            src += ':' + std::to_string(line);
        }
        return src + ": " + msg;
    }
    TStatus Status() const override
    {
        return TStatus::LUA_ERRSYNTAX;
    }
};
struct RunException : public CodeException
{
    const TopPrototype* proto{};
    RunException(const TopPrototype* p, size_t l, std::string m) : CodeException(l, std::move(m)), proto{p}
    {
    }
    std::string ToString() const override
    {
        auto src = proto->ShortSource();
        if (line)
        {
            src += ':' + std::to_string(line);
        }
        return src + ": " + msg;
    }
    TStatus Status() const override
    {
        return TStatus::LUA_ERRRUN;
    }
};
struct ErrorException : public Exception
{
    using Exception::Exception;
    TStatus Status() const override
    {
        return TStatus::LUA_ERRERR;
    }
};

//
inline auto& operator<<(std::ostream& os, const Exception& e)
{
    return os << e.ToString();
}

}  // namespace lua

#endif