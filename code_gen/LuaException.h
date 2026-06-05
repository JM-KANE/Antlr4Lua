#ifndef RUN_EXCEPTION_H
#define RUN_EXCEPTION_H
#include "type.h"
namespace lua
{
struct TopPrototype;
struct SyntaxError;

struct Exception /*: public std::exception*/
{
    std::string msg;

    Exception(std::string m);
    virtual TStatus Status() const = 0;
    virtual std::string ToString() const;
    virtual ~Exception() = default;
};

struct FileException : public Exception
{
    using Exception::Exception;
    TStatus Status() const override;
    std::string ToString() const override;
};

struct CodeException : Exception
{
    size_t line{};

    CodeException(size_t l, std::string m);
};

struct SyntaxException : public CodeException
{
    std::string shortSource;

    SyntaxException(const TopPrototype* p, size_t l, std::string m);
    SyntaxException(const TopPrototype* p, SyntaxError&& info);
    std::string ToString() const override;
    TStatus Status() const override;
};
struct RunException : public CodeException
{
    const TopPrototype* proto{};
    RunException(const TopPrototype* p, size_t l, std::string m);
    std::string ToString() const override;
    TStatus Status() const override;
};
struct ErrorException : public Exception
{
    using Exception::Exception;
    TStatus Status() const override;
};

//
inline auto& operator<<(std::ostream& os, const Exception& e)
{
    return os << e.ToString();
}

}  // namespace lua

#endif