#ifndef _FUNC_INFO_H
#define _FUNC_INFO_H

#include <vector>
#include <memory>
#include "type.h"
#include "OpCodes.h"

#include "LuaParser.h"

namespace lua
{
using slot_type = int16_t;

// TODO chunk and header

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

    std::string Source;
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
};

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

struct FuncInfo
{
    FuncInfo* parent{};
    std::vector<FuncInfo> subFuncs;
    slot_type usedRegs{};
    slot_type maxRegs{255};
    uint32_t scopeLv{};
    std::vector<LocVarInfo> locVars;
    string_ref_map<LocVarInfo*> locNames;
    std::unordered_map<std::string, UpvalInfo> upvalues;
    std::unordered_map<any_type, size_t> constants;
    std::vector<std::unique_ptr<std::vector<size_t>>> breaks;
    std::vector<uint32_t> insts;
    std::vector<uint32_t> lineNums;
    uint32_t line{};
    uint32_t lastLine{};
    uint32_t numParams{};
    bool isVararg = true;

    // FuncInfo() = default;
    FuncInfo(FuncInfo* p = {});

    static int32_t Int2fb(int32_t x);

    size_t PC() const;
    slot_type AllocReg();
    slot_type AllocRegs(slot_type n);
    void FreeReg();
    void FreeRegs(slot_type n);

    void EnterScope(bool b);
    void ExitScope(size_t endPC);

    size_t IndexOfConstant(any_type k);
    int16_t AddLocVar(std::string name, size_t startPC);
    void RemoveLocVar(LocVarInfo* locVar);
    void RemoveScopeLocVars(bool bf, size_t endPC);
    slot_type SlotOfLocVar(const std::string& name) const;
    int64_t IndexOfUpval(const std::string& name);

    slot_type GetJmpArgA() const;
    void AddBreakJmp(size_t pc);
    void CloseOpenUpvals();
    void FixSbx(size_t pc, int32_t sBx);
    void FixEndPC(const std::string& name, int32_t delta);

    void EmitABC(Op opcode, slot_type a, slot_type b, slot_type c);
    void EmitABx(Op opcode, slot_type a, int32_t bx);
    void EmitAsBx(Op opcode, slot_type a, int32_t bx);
    void EmitAx(Op opcode, int32_t ax);

    void EmitReturn(slot_type a, slot_type n);
    void EmitClosure(slot_type a, int32_t bx);
    void EmitTailCall(slot_type a, slot_type n);
    void EmitCall(slot_type a, slot_type b, slot_type c);
    void EmitLoadNil(slot_type a, slot_type n);
    void EmitLoadBool(slot_type a, slot_type b, slot_type c);
    void EmitLoadK(slot_type a, any_type k);
    void EmitVararg(slot_type a, slot_type n);
    void EmitMove(slot_type a, slot_type b);
    void EmitSetUpval(slot_type a, slot_type b);
    void EmitGetUpval(slot_type a, slot_type b);
    void EmitSetTable(slot_type a, slot_type b, slot_type c);
    void EmitGetTable(slot_type a, slot_type b, slot_type c);
    void EmitSetTabUp(slot_type a, slot_type b, slot_type c);
    void EmitGetTabUp(slot_type a, slot_type b, slot_type c);
    size_t EmitJmp(slot_type a, int32_t sBx);
    void EmitTest(slot_type a, slot_type c);
    void EmitTestSet(slot_type a, slot_type b, slot_type c);
    size_t EmitForPrep(slot_type a, int32_t sBx);
    size_t EmitForLoop(slot_type a, int32_t sBx);
    void EmitTForCall(slot_type a, slot_type c);
    void EmitTForLoop(slot_type a, int32_t sBx);
    void EmitSelf(slot_type a, slot_type b, slot_type c);
    void EmitNewTable(slot_type a, slot_type nArr, slot_type nRec);
    void EmitSetList(slot_type a, slot_type b, slot_type c);
    void EmitConcat(slot_type a, slot_type b, slot_type c);

    Prototype ToProto();
    void GetConstants(std::vector<any_type>& v);
    void GetUpvalues(std::vector<Prototype::Upvalue>& v);
    void GetUpvalueNames(std::vector<std::string>& v);
    void GetLocVars(std::vector<Prototype::LocVar>& v);
    void ToSubProtos(std::vector<Prototype>& v);
};

}  // namespace lua

#endif