#include "LuaException.h"
#include "FuncInfo.h"

using namespace lua;

lua::Exception::Exception(std::string m) : msg(std::move(m))
{
}

std::string lua::Exception::ToString() const
{
    return msg;
}

TStatus lua::FileException::Status() const
{
    return TStatus::LUA_ERRFILE;
}

std::string lua::FileException::ToString() const
{
    return "cannot open " + msg + ": No such file or directory";
}

lua::CodeException::CodeException(size_t l, std::string m) : line(l), Exception(std::move(m))
{
}

SyntaxException::SyntaxException(const TopPrototype* p, size_t l, std::string m)
    : CodeException(l, std::move(m)),
      shortSource{p->ShortSource()}
{
}

lua::SyntaxException::SyntaxException(const TopPrototype* p, SyntaxError&& info)
    : SyntaxException(p, info.line, std::move(info.msg))
{
}

std::string lua::SyntaxException::ToString() const
{
    auto src = shortSource;
    if (line)
    {
        src += ':' + std::to_string(line);
    }
    return src + ": " + msg;
}

TStatus lua::SyntaxException::Status() const
{
    return TStatus::LUA_ERRSYNTAX;
}

lua::RunException::RunException(const TopPrototype* p, size_t l, std::string m)
    : CodeException(l, std::move(m)),
      proto{p}
{
}

std::string lua::RunException::ToString() const
{
    auto src = proto->ShortSource();
    if (line)
    {
        src += ':' + std::to_string(line);
    }
    return src + ": " + msg;
}

TStatus lua::RunException::Status() const
{
    return TStatus::LUA_ERRRUN;
}

TStatus lua::ErrorException::Status() const
{
    return TStatus::LUA_ERRERR;
}
