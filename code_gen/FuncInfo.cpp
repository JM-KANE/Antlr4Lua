#include "FuncInfo.h"
using namespace lua;

lua::LocVarInfo::LocVarInfo(std::string n, uint32_t slv, slot_type st, size_t s, size_t e)
    : name(std::move(n)),
      scopeLv(slv),
      slot(st),
      startPC(s),
      endPC(e)
{
}

// lua::FuncInfo::FuncInfo(const LuaParser::Start_Context* start)
// {
// }

lua::FuncInfo::FuncInfo(FuncInfo* p) : parent(p)
{
    breaks.emplace_back();
}

int32_t lua::FuncInfo::Int2fb(int32_t x)
{
    int32_t e = 0;
    if (x < 8)
    {
        return x;
    }
    while (x >= (8 << 4))
    {
        x = (x + 0xf) >> 4;
        e += 4;
    }
    while (x >= (8 << 1))
    {
        x = (x + 1) >> 1;
        e++;
    }
    return ((e + 1) << 3) | (x - 8);
}

size_t lua::FuncInfo::PC() const
{
    return insts.size();
}

slot_type lua::FuncInfo::AllocReg()
{
    if (255 == usedRegs)
    {
        // TODO error
    }
    ++usedRegs;
    if (usedRegs > maxRegs)
    {
        usedRegs = maxRegs;
    }
    return usedRegs - 1;
}

slot_type lua::FuncInfo::AllocRegs(slot_type n)
{
    if (n <= 0)
    {
        // TODO error
    }
    for (size_t i = 0; i < n; i++)
    {
        AllocReg();
    }

    return usedRegs - n;
}

void lua::FuncInfo::FreeReg()
{
    if (0 == usedRegs)
    {
        // TODO error
    }
    --usedRegs;
}

void lua::FuncInfo::FreeRegs(slot_type n)
{
    if (n > 0)
    {
        for (size_t i = 0; i < n; i++)
        {
            FreeReg();
        }
    }
}

void lua::FuncInfo::EnterScope(bool b)
{
    ++scopeLv;
    breaks.emplace_back(b ? std::make_unique<std::vector<size_t>>() : nullptr);
}

void lua::FuncInfo::ExitScope(size_t endPC)
{
    auto pending = std::move(breaks.back());
    breaks.pop_back();
    if (pending)
    {
        auto a = GetJmpArgA();
        for (auto&& pc : *pending)
        {
            auto sBx = PC() - 1 - pc;
            auto i = uint32_t(Op::JMP) | a << 6 | (sBx + cv::MAXARG_sBx) << 14;
            insts[pc] = uint32_t(i);
        }
    }
    --scopeLv;
    RemoveScopeLocVars(false, endPC);
}

size_t lua::FuncInfo::IndexOfConstant(any_type k)
{
    auto sz = constants.size();
    auto [it, suc] = constants.try_emplace(std::move(k), sz);
    return suc ? sz : it->second;
}

slot_type lua::FuncInfo::AddLocVar(std::string name, size_t startPC)
{
    auto& newVar = locVars.emplace_back(std::move(name), scopeLv, AllocReg(), startPC, 0);
    auto [it, suc] = locNames.try_emplace(newVar.name, &newVar);
    if (!suc)
    {
        newVar.prev = it->second;
        it->second = &newVar;
    }
    return newVar.slot;
}

void lua::FuncInfo::RemoveLocVar(LocVarInfo* locVar)
{
    FreeReg();
    if (!locVar->prev)
    {
        locNames.erase(locVar->name);
    }
    else if (locVar->prev->scopeLv == locVar->scopeLv)
    {
        RemoveLocVar(locVar->prev);
    }
    else
    {
        locNames.at(locVar->name) = locVar->prev;
    }
}

void lua::FuncInfo::RemoveScopeLocVars(bool bf, size_t endPC)
{
    std::vector<LocVarInfo*> locVars;
    locVars.reserve(locNames.size());
    for (auto it = locNames.begin(); it != locNames.end(); ++it)
    {
        locVars.emplace_back(it->second);
    }
    for (auto&& locVar : locVars)
    {
        if (bf || locVar->scopeLv > scopeLv)
        {
            locVar->endPC = endPC;
            RemoveLocVar(locVar);
        }
    }
}

slot_type lua::FuncInfo::SlotOfLocVar(const std::string& name) const
{
    auto it = locNames.find(name);
    return it == locNames.end() ? -1 : it->second->slot;
}

int64_t lua::FuncInfo::IndexOfUpval(const std::string& name)
{
    if (auto it = upvalues.find(name); it != upvalues.end())
    {
        return it->second.index;
    }
    if (parent)
    {
        if (auto it = parent->locNames.find(name); it != parent->locNames.end())
        {
            auto locVar = it->second;
            auto idx = upvalues.size();
            upvalues.emplace(name, UpvalInfo{locVar->slot, -1, idx});
            locVar->captured = true;
            return idx;
        }
        if (auto uvIdx = parent->IndexOfUpval(name); uvIdx >= 0)
        {
            auto idx = upvalues.size();
            upvalues.emplace(name, UpvalInfo{-1, uvIdx, idx});
            return idx;
        }
    }
    return -1;
}

slot_type lua::FuncInfo::GetJmpArgA() const
{
    bool hasCapturedLocVars{};
    auto minSlotOfLocVars = maxRegs;
    for (auto&& [name, locVar] : locNames)
    {
        if (locVar->scopeLv == scopeLv)
        {
            for (auto v = locVar; v && v->scopeLv == scopeLv; v = v->prev)
            {
                if (v->captured)
                {
                    hasCapturedLocVars = true;
                }
                if (v->slot < minSlotOfLocVars && v->name[0] != '(')
                {
                    minSlotOfLocVars = v->slot;
                }
            }
        }
    }
    return hasCapturedLocVars ? minSlotOfLocVars + 1 : 0;
}

void lua::FuncInfo::AddBreakJmp(size_t pc)
{
    for (size_t j = 0; j <= scopeLv; j++)
    {
        auto i = scopeLv - j;
        if (breaks[i])
        {
            breaks[i]->emplace_back(pc);
            return;
        }
    }

    // error
}

void lua::FuncInfo::CloseOpenUpvals()
{
    auto a = GetJmpArgA();
    if (a > 0)
    {
        EmitJmp(a, 0);
    }
}

void lua::FuncInfo::FixSbx(size_t pc, int32_t sBx)
{
    auto& i = insts[pc];
    i = i << 18 >> 18;
    i |= uint32_t(sBx + cv::MAXARG_sBx) << 14;  // reset sBx
}

void lua::FuncInfo::FixEndPC(const std::string& name, int32_t delta)
{
    for (auto it = locVars.rbegin(); it != locVars.rend(); it++)
    {
        if (it->name == name)
        {
            it->endPC += delta;
            return;
        }
    }
}

void lua::FuncInfo::EmitABC(Op opcode, slot_type a, slot_type b, slot_type c)
{
    auto i = uint32_t(opcode) | a << 6 | b << 23 | c << 14;
    insts.emplace_back(i);
}

void lua::FuncInfo::EmitABx(Op opcode, slot_type a, int32_t bx)
{
    auto i = uint32_t(opcode) | a << 6 | bx << 14;
    insts.emplace_back(i);
}

void lua::FuncInfo::EmitAsBx(Op opcode, slot_type a, int32_t bx)
{
    auto i = uint32_t(opcode) | a << 6 | (bx + cv::MAXARG_sBx) << 14;
    insts.emplace_back(i);
}

void lua::FuncInfo::EmitAx(Op opcode, int32_t ax)
{
    auto i = uint32_t(opcode) | ax << 6;
    insts.emplace_back(i);
}

void lua::FuncInfo::EmitReturn(slot_type a, slot_type n)
{
    EmitABC(Op::RETURN, a, n + 1, 0);
}

void lua::FuncInfo::EmitClosure(slot_type a, int32_t bx)
{
    EmitABx(Op::CLOSURE, a, bx);
}

void lua::FuncInfo::EmitTailCall(slot_type a, slot_type n)
{
    EmitABC(Op::TAILCALL, a, n + 1, 0);
}

void lua::FuncInfo::EmitCall(slot_type a, slot_type b, slot_type c)
{
    EmitABC(Op::CALL, a, b + 1, c + 1);
}

void lua::FuncInfo::EmitLoadNil(slot_type a, slot_type n)
{
    EmitABC(Op::LOADNIL, a, n - 1, 0);
}

void lua::FuncInfo::EmitLoadBool(slot_type a, slot_type b, slot_type c)
{
    EmitABC(Op::LOADBOOL, a, b, c);
}

void lua::FuncInfo::EmitLoadK(slot_type a, any_type k)
{
    auto idx = (int32_t)IndexOfConstant(std::move(k));
    if (idx < (1 << 18))
    {
        EmitABx(Op::LOADK, a, idx);
    }
    else
    {
        EmitABx(Op::LOADKX, a, 0);
        EmitAx(Op::EXTRAARG, idx);
    }
}

void lua::FuncInfo::EmitVararg(slot_type a, slot_type n)
{
    EmitABC(Op::VARARG, a, n + 1, 0);
}

void lua::FuncInfo::EmitMove(slot_type a, slot_type b)
{
    EmitABC(Op::MOVE, a, b, 0);
}

void lua::FuncInfo::EmitSetUpval(slot_type a, slot_type b)
{
    EmitABC(Op::SETUPVAL, a, b, 0);
}

void lua::FuncInfo::EmitGetUpval(slot_type a, slot_type b)
{
    EmitABC(Op::GETUPVAL, a, b, 0);
}

void lua::FuncInfo::EmitSetTable(slot_type a, slot_type b, slot_type c)
{
    EmitABC(Op::SETTABLE, a, b, c);
}

void lua::FuncInfo::EmitGetTable(slot_type a, slot_type b, slot_type c)
{
    EmitABC(Op::GETTABLE, a, b, c);
}

void lua::FuncInfo::EmitSetTabUp(slot_type a, slot_type b, slot_type c)
{
    EmitABC(Op::SETTABUP, a, b, c);
}

void lua::FuncInfo::EmitGetTabUp(slot_type a, slot_type b, slot_type c)
{
    EmitABC(Op::GETTABUP, a, b, c);
}

size_t lua::FuncInfo::EmitJmp(slot_type a, int32_t sBx)
{
    EmitAsBx(Op::JMP, a, sBx);
    return PC() - 1;
}

void lua::FuncInfo::EmitTest(slot_type a, slot_type c)
{
    EmitABC(Op::TEST, a, 0, c);
}

void lua::FuncInfo::EmitTestSet(slot_type a, slot_type b, slot_type c)
{
    EmitABC(Op::TESTSET, a, b, c);
}

size_t lua::FuncInfo::EmitForPrep(slot_type a, int32_t sBx)
{
    EmitAsBx(Op::FORPREP, a, sBx);
    return PC() - 1;
}

size_t lua::FuncInfo::EmitForLoop(slot_type a, int32_t sBx)
{
    EmitAsBx(Op::FORLOOP, a, sBx);
    return PC() - 1;
}

void lua::FuncInfo::EmitTForCall(slot_type a, slot_type c)
{
    EmitABC(Op::TFORCALL, a, 0, c);
}

void lua::FuncInfo::EmitTForLoop(slot_type a, int32_t sBx)
{
    EmitAsBx(Op::TFORLOOP, a, sBx);
}

void lua::FuncInfo::EmitSelf(slot_type a, slot_type b, slot_type c)
{
    EmitABC(Op::SELF, a, b, c);
}

void lua::FuncInfo::EmitNewTable(slot_type a, slot_type nArr, slot_type nRec)
{
    EmitABC(Op::NEWTABLE, a, Int2fb(nArr), Int2fb(nRec));
}

void lua::FuncInfo::EmitSetList(slot_type a, slot_type b, slot_type c)
{
    EmitABC(Op::SETLIST, a, b, c);
}

void lua::FuncInfo::EmitConcat(slot_type a, slot_type b, slot_type c)
{
    EmitABC(Op::CONCAT, a, b, c);
}

Prototype lua::FuncInfo::ToProto()
{
    Prototype proto;
    proto.LineDefined = line;
    proto.LastLineDefined = lastLine;
    proto.NumParams = (uint8_t)numParams;
    proto.MaxStackSize = uint8_t(maxRegs);
    proto.Code = std::move(insts);
    GetConstants(proto.Constants);
    GetUpvalues(proto.Upvalues);
    ToSubProtos(proto.Protos);
    proto.LineInfo = std::move(lineNums);
    GetLocVars(proto.LocVars);
    GetUpvalueNames(proto.UpvalueNames);
    if (line == 0)
        proto.LastLineDefined = 0;

    // TODO
    if (proto.MaxStackSize < 2)
    {
        proto.MaxStackSize = 2;
    }
    if (isVararg)
    {
        proto.IsVararg = 1;
    }

    return proto;
}

void lua::FuncInfo::GetConstants(std::vector<any_type>& v)
{
    v.resize(constants.size());
    while (!constants.empty())
    {
        auto n = constants.extract(constants.begin());
        v[n.mapped()] = std::move(n.key());
    }
}

void lua::FuncInfo::GetUpvalues(std::vector<Prototype::Upvalue>& v)
{
    v.resize(upvalues.size());
    for (auto&& [n, uv] : upvalues)
    {
        if (uv.locVarSlot >= 0)
        {
            v[uv.index] = {1, uint8_t(uv.locVarSlot)};
        }
        else
        {
            v[uv.index] = {0, uint8_t(uv.upvalIndex)};
        }
    }
}

void lua::FuncInfo::GetUpvalueNames(std::vector<std::string>& v)
{
    v.resize(upvalues.size());
    while (!upvalues.empty())
    {
        auto n = upvalues.extract(upvalues.begin());
        v[n.mapped().index] = std::move(n.key());
    }
}

void lua::FuncInfo::GetLocVars(std::vector<Prototype::LocVar>& v)
{
    v.reserve(locVars.size());
    for (auto&& locVar : locVars)
    {
        v.emplace_back(std::move(locVar.name), uint32_t(locVar.startPC), uint32_t(locVar.endPC));
    }
}

void lua::FuncInfo::ToSubProtos(std::vector<Prototype>& v)
{
    v.reserve(subFuncs.size());
    for (auto&& fi : subFuncs)
    {
        v.emplace_back(fi.ToProto());
    }
}
