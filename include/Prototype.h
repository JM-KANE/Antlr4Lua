#ifndef PROTOTYPE_H
#define PROTOTYPE_H
#include "SyntaxError.h"
#include <vector>
#include <memory>
#include <variant>

// TODO chunk and header

namespace lua
{

using any_type = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;

struct TopPrototype;
struct Prototype
{
    struct Upvalue
    {
        uint8_t Instack;
        uint8_t Idx;
    };
    struct LocVar
    {
        std::string VarName;
        uint32_t StartPC{};
        uint32_t EndPC{};
    };

    Prototype* Parent{};
    uint32_t LineDefined{};
    uint32_t LastLineDefined{};
    uint8_t NumParams{};
    uint8_t IsVararg{};
    uint8_t MaxStackSize{};
    std::vector<uint32_t> Code;
    std::vector<any_type> Constants;
    std::vector<Upvalue> Upvalues;
    std::vector<Prototype> Protos;
    std::vector<uint32_t> LineInfo;
    std::vector<LocVar> LocVars;
    std::vector<std::string> UpvalueNames;

    const TopPrototype* Top() const
    {
        auto p = this;
        while (p->Parent)
        {
            p = p->Parent;
        }
        return reinterpret_cast<const TopPrototype*>(p);
    }
};
struct TopPrototype : Prototype
{
    std::unique_ptr<SyntaxError> err;
    std::string Source;

    std::string ShortSource() const
    {
        if (!Source.empty() && (Source.front() == '@' || Source.front() == '='))
        {
            return Source.substr(1);
        }
        std::string src = "[string \"";
        src += Source;
        src += "\"]";
        return src;
    }
};

}  // namespace lua

#endif