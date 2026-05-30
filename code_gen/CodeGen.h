#ifndef _CODE_GEN_H
#define _CODE_GEN_H

#include <array>

#include "LuaParserBaseVisitor.h"
#include "FuncInfo.h"

namespace lua
{
namespace Arg
{
constexpr uint8_t CONST = 1;
constexpr uint8_t REG = 2;
constexpr uint8_t UPVAL = 4;
constexpr uint8_t RK = REG | CONST;
constexpr uint8_t RU = REG | UPVAL;
constexpr uint8_t RUK = RU | CONST;
}  // namespace Arg

class CodeGen : public LuaParserBaseVisitor
{
public:
private:
    static constexpr size_t NP = 2;
    slot_type _a = -1;
    int32_t _n = -2;

    std::unique_ptr<FuncInfo> root;
    FuncInfo* fi{};

    static std::string Unescape(std::string_view src);
    static std::string TrimLong(const std::string& str);

    void VisitWithPara(slot_type a, slot_type n, LuaRuleContext* ctx);

public:
    // static auto& Instance()
    // {
    //     static CodeGen cg;
    //     return cg;
    // }

    static bool IsVarargOrFuncCall(LuaRuleContext* exp);
    static void RemoveTailNils(std::vector<LuaParser::ExpContext*>& exps);

    std::array<slot_type, NP> GetPara() const;
    void SetPara(std::array<slot_type, NP> p);

    std::pair<slot_type, uint8_t> ExpToOpArg(LuaParser::ExpContext* node, uint8_t argKinds);
    std::pair<slot_type, slot_type> ExpToOpArg(const std::vector<LuaParser::ExpContext*>& nodes, uint8_t argKinds);
    std::pair<slot_type, uint8_t> NameToOpArg(const std::string& name, uint8_t argKinds);
    slot_type ConstantToOpArg(const any_type& cv);
    slot_type VisitMember(LuaParser::MemberContext* member);

    void Generate(const std::string& data, Prototype& proto);
    Prototype Generate(LuaParser::ChunkContext* ck);
    void Generate(LuaParser::ChunkContext* ck, Prototype& proto);
    Prototype ToProto() const;

    std::any DoVisitFuncbody(LuaParser::FuncbodyContext* ctx, bool self);
    std::any visitChunk(LuaParser::ChunkContext* ctx) override;
    std::any visitFuncbody(LuaParser::FuncbodyContext* ctx) override;
    std::any visitBlock(LuaParser::BlockContext* ctx) override;
    std::any visitRetstat(LuaParser::RetstatContext* ctx) override;
    // void visitExp(LuaParser::ExpContext* ctx, slot_type a, slot_type n);

    void DoVarDecl(const std::vector<LuaParser::ExpContext*>& exps, std::vector<std::string>&& names);
    std::any visitAssign(LuaParser::AssignContext* ctx) override;
    std::any visitFunctioncall_(LuaParser::Functioncall_Context* ctx) override;
    std::any visitLabel(LuaParser::LabelContext* ctx) override;
    std::any visitBreak(LuaParser::BreakContext* ctx) override;
    std::any visitGoto(LuaParser::GotoContext* ctx) override;
    std::any visitDo(LuaParser::DoContext* ctx) override;
    std::any visitWhile(LuaParser::WhileContext* ctx) override;
    std::any visitRepeat(LuaParser::RepeatContext* ctx) override;
    std::any visitIf(LuaParser::IfContext* ctx) override;
    std::any visitFornumerical(LuaParser::FornumericalContext* ctx) override;
    std::any visitForgeneric(LuaParser::ForgenericContext* ctx) override;
    std::any visitFuncnamedef(LuaParser::FuncnamedefContext* ctx) override;
    std::any visitLocalfunc(LuaParser::LocalfuncContext* ctx) override;
    std::any visitVardecl(LuaParser::VardeclContext* ctx) override;

    using MemberContexts = std::vector<LuaParser::MemberContext*>;
    using mem_iterator = MemberContexts::reverse_iterator;
    using part_iterator = std::vector<LuaParser::CallpartContext*>::reverse_iterator;
    using name_iterator = std::vector<antlr4::tree::TerminalNode*>::reverse_iterator;

    static std::vector<LuaRuleContext*> GetArgs(LuaParser::ArgsContext* args);

    slot_type PrepMemFuncCall(LuaParser::FunctioncallContext* node, slot_type a, part_iterator start, part_iterator end,
                              mem_iterator start_m, mem_iterator end_m);
    slot_type PrepMemFuncCall(LuaParser::FunctioncallContext* node, slot_type a, part_iterator start, part_iterator end,
                              MemberContexts& mems);
    slot_type PrepFuncCall(LuaParser::FunctioncallContext* node, slot_type a, part_iterator start, part_iterator end);
    slot_type PrepFuncCall(LuaParser::FunctioncallContext* node, slot_type r);
    void PrepPrefix(LuaParser::PrefixexpContext* pre, slot_type a, mem_iterator start, mem_iterator end);
    void PrepPrefix(LuaParser::PrefixexpContext* pre, slot_type a, slot_type n, MemberContexts& mems);
    slot_type PrepFuncName(slot_type a, name_iterator start, name_iterator end);

    std::any visitTerminal(antlr4::tree::TerminalNode* ctx) override;
    void VisitNAME(const std::string& text, slot_type a);

    std::any visitNil(LuaParser::NilContext* ctx) override;
    std::any visitFalse(LuaParser::FalseContext* ctx) override;
    std::any visitTrue(LuaParser::TrueContext* ctx) override;
    std::any visitInt(LuaParser::IntContext* ctx) override;
    std::any visitHex(LuaParser::HexContext* ctx) override;
    std::any visitFloat(LuaParser::FloatContext* ctx) override;
    std::any visitHexfloat(LuaParser::HexfloatContext* ctx) override;
    std::any visitNormalstring(LuaParser::NormalstringContext* ctx) override;
    std::any visitCharstring(LuaParser::CharstringContext* ctx) override;
    std::any visitLongstring(LuaParser::LongstringContext* ctx) override;
    std::any visitVarargexp(LuaParser::VarargexpContext* ctx) override;
    std::any visitNameindex(LuaParser::NameindexContext* ctx) override;
    std::any visitCallindex(LuaParser::CallindexContext* ctx) override;
    std::any visitExpindex(LuaParser::ExpindexContext* ctx) override;
    std::any visitFunctioncall(LuaParser::FunctioncallContext* ctx) override;
    std::any visitTableconstructor(LuaParser::TableconstructorContext* ctx) override;

    template <typename T>
    void VisitLogical(LuaParser::ExpContext* ctx);
    void GetRecursiveConcat(std::vector<LuaParser::ExpContext*>& ctxs, LuaParser::ExpContext* exp);
    std::any visitExponentiation(LuaParser::ExponentiationContext* ctx) override;
    std::any visitUnary(LuaParser::UnaryContext* ctx) override;
    std::any visitMultidiv(LuaParser::MultidivContext* ctx) override;
    std::any visitAddsub(LuaParser::AddsubContext* ctx) override;
    std::any visitConcatenation(LuaParser::ConcatenationContext* ctx) override;
    std::any visitRelation(LuaParser::RelationContext* ctx) override;
    std::any visitAnd(LuaParser::AndContext* ctx) override;
    std::any visitOr(LuaParser::OrContext* ctx) override;
    std::any visitBitwise(LuaParser::BitwiseContext* ctx) override;
};

}  // namespace lua
#endif
