#ifndef _FUNC_INFO_H
#define _FUNC_INFO_H

#include <vector>
#include <memory>
#include "../include/type.h"
#include "../include/OpCodesType.h"

#include "../include/Prototype.h"

namespace lua
{
using slot_type = int16_t;

struct UpvalInfo
{
    int64_t locVarSlot{};
    int64_t upvalIndex{};
    size_t index{};
};

struct LocVarInfo
{
    LocVarInfo* prev{};
    std::string name;
    uint32_t scopeLv{};
    slot_type slot{};
    size_t startPC{};
    size_t endPC{};
    bool captured{};

    LocVarInfo(std::string n, uint32_t slv, slot_type st, size_t s, size_t e);
};

struct GotoInfo
{
    size_t pc{};
    LocVarInfo* local{};
};

struct LabelInfo
{
    size_t pc = -1;
    std::vector<GotoInfo> gotos;
};

using LocalScopeInfo = std::tuple<const std::string*, const std::string*, uint32_t>;
class LuaRuleContext;
struct FuncInfo
{
    using block_labels_ast = std::unordered_set<std::string>;
    using block_labels = const block_labels_ast*;

    FuncInfo* parent{};
    std::vector<std::unique_ptr<FuncInfo>> subFuncs;
    slot_type usedRegs{};
    slot_type maxRegs{0};
    uint32_t scopeLv{};
    std::vector<std::unique_ptr<LocVarInfo>> locVars;
    string_ref_map<LocVarInfo*> locNames;
    std::unordered_map<std::string, UpvalInfo> upvalues;
    std::unordered_map<any_type, size_t> constants;
    std::vector<std::unique_ptr<std::vector<size_t>>> breaks;

    std::vector<block_labels> labels;
    std::unordered_map<const std::string*, std::unique_ptr<LabelInfo>> labelsInfo;
    std::unique_ptr<LocalScopeInfo> localScope;

    std::vector<uint32_t> insts;
    std::vector<uint32_t> lineNums;
    const uint32_t line{};
    const uint32_t lastLine{};
    uint32_t numParams{};
    bool isVararg = true;
    std::unique_ptr<SyntaxError>* ec{};

    // FuncInfo() = default;
    FuncInfo(LuaRuleContext* node, FuncInfo* p = {});

    static int32_t Int2fb(int32_t x);

    size_t PC() const;
    slot_type AllocReg();
    slot_type AllocRegs(slot_type n);
    void FreeReg();
    void FreeRegs(slot_type n);

    template <typename... Ts>
    void Error(Ts&&... args)
    {
        // ec->CompileError(std::forward<Ts>(args)...);
        *ec = std::make_unique<SyntaxError>(std::forward<Ts>(args)...);
    }
    bool ReleaseScopeError();

    void EnterScope(bool b);
    void ExitScope(size_t endPC);

    size_t IndexOfConstant(any_type k);
    int16_t AddLocVar(std::string name, size_t startPC);
    void RemoveLocVar(LocVarInfo* locVar);
    void RemoveScopeLocVars(bool bf, size_t endPC);
    slot_type SlotOfLocVar(const std::string& name) const;
    int64_t IndexOfUpval(const std::string& name);

    slot_type GetJmpArgA() const;
    slot_type GetJmpArgA(uint32_t lv) const;
    void AddBreakJmp(size_t pc);
    void AddGotoJmp(const std::string& label, size_t pc, uint32_t line);
    void CloseOpenUpvals(uint32_t line);
    void FixGotoSbx(const std::string& name, uint32_t line);
    void FixSbx(size_t pc, int32_t sBx);
    void FixEndPC(const std::string& name, int32_t delta);

    void EmitInstruction(uint32_t line, uint32_t i);
    void EmitABC(uint32_t line, Op opcode, slot_type a, slot_type b, slot_type c);
    void EmitABx(uint32_t line, Op opcode, slot_type a, int32_t bx);
    void EmitAsBx(uint32_t line, Op opcode, slot_type a, int32_t bx);
    void EmitAx(uint32_t line, Op opcode, int32_t ax);

    void EmitReturn(uint32_t line, slot_type a, slot_type n);
    void EmitClosure(uint32_t line, slot_type a, int32_t bx);
    void EmitTailCall(uint32_t line, slot_type a, slot_type n);
    void EmitCall(uint32_t line, slot_type a, slot_type b, slot_type c);
    void EmitLoadNil(uint32_t line, slot_type a, slot_type n);
    void EmitLoadBool(uint32_t line, slot_type a, slot_type b, slot_type c);
    void EmitLoadK(uint32_t line, slot_type a, any_type k);
    void EmitVararg(uint32_t line, slot_type a, slot_type n);
    void EmitMove(uint32_t line, slot_type a, slot_type b);
    void EmitSetUpval(uint32_t line, slot_type a, slot_type b);
    void EmitGetUpval(uint32_t line, slot_type a, slot_type b);
    void EmitSetTable(uint32_t line, slot_type a, slot_type b, slot_type c);
    void EmitGetTable(uint32_t line, slot_type a, slot_type b, slot_type c);
    void EmitSetTabUp(uint32_t line, slot_type a, slot_type b, slot_type c);
    void EmitGetTabUp(uint32_t line, slot_type a, slot_type b, slot_type c);
    size_t EmitJmp(uint32_t line, slot_type a, int32_t sBx);
    void EmitTest(uint32_t line, slot_type a, slot_type c);
    void EmitTestSet(uint32_t line, slot_type a, slot_type b, slot_type c);
    size_t EmitForPrep(uint32_t line, slot_type a, int32_t sBx);
    size_t EmitForLoop(uint32_t line, slot_type a, int32_t sBx);
    void EmitTForCall(uint32_t line, slot_type a, slot_type c);
    void EmitTForLoop(uint32_t line, slot_type a, int32_t sBx);
    void EmitSelf(uint32_t line, slot_type a, slot_type b, slot_type c);
    void EmitNewTable(uint32_t line, slot_type a, slot_type nArr, slot_type nRec);
    void EmitSetList(uint32_t line, slot_type a, slot_type b, slot_type c);
    void EmitConcat(uint32_t line, slot_type a, slot_type b, slot_type c);

    void ToProto(Prototype& proto);
    void GetConstants(std::vector<any_type>& v);
    void GetUpvalues(std::vector<Prototype::Upvalue>& v);
    void GetUpvalueNames(std::vector<std::string>& v);
    void GetLocVars(std::vector<Prototype::LocVar>& v);
    void ToSubProtos(Prototype& p);

private:
    LocVarInfo* CurrentLocal(uint32_t lv) const;
};

}  // namespace lua

#endif