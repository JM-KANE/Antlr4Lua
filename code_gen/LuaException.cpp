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

lua::CodeException::CodeException(const TopPrototype* p, size_t l, std::string m)
    : proto(p),
      line(l),
      Exception(std::move(m))
{
}

std::string lua::CodeException::ToString() const
{
    auto src = proto->ShortSource();
    if (line)
    {
        src += ':' + std::to_string(line);
    }
    return src + ": " + msg;
}

lua::SyntaxException::SyntaxException(const TopPrototype* p, SyntaxError&& info)
    : SyntaxException(p, info.line, std::move(info.msg))
{
}

TStatus lua::SyntaxException::Status() const
{
    return TStatus::LUA_ERRSYNTAX;
}

TStatus lua::RunException::Status() const
{
    return TStatus::LUA_ERRRUN;
}

TStatus lua::ErrorException::Status() const
{
    return TStatus::LUA_ERRERR;
}
