#include "FuncInfo.h"
using namespace lua;

void lua::TopPrototype::PrintError(std::ostream& os)
{
    for (auto&& err : ec.GetErrors())
    {
        os << ShortSource() << err.Msg() << std::endl;
    }
    ec.Clear();
}

std::string lua::TopPrototype::ShortSource() const
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

lua::FuncInfo::FuncInfo(LuaRuleContext* node, FuncInfo* p) : parent(p), line(node->Line()), lastLine(node->LastLine())
{
    breaks.emplace_back();
    // labels.emplace_back();
    if (parent)
        ec = parent->ec;
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
        Error(nullptr, line,
              "too many registers (limit is 255) in function (start line: " + std::to_string(line) + ")");
    }
    ++usedRegs;
    if (usedRegs > maxRegs)
    {
        maxRegs = usedRegs;
    }
    return usedRegs - 1;
}

slot_type lua::FuncInfo::AllocRegs(slot_type n)
{
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
        return;
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

bool lua::FuncInfo::ReleaseScopeError()
{
    if (localScope)
    {
        auto [l, v, line] = *localScope;
        Error(nullptr, line, "<goto " + *l + "> jumps into the scope of '" + *v + "'");
        localScope.reset();
        return true;
    }
    return false;
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
    auto& newVar = locVars.emplace_back(std::make_unique<LocVarInfo>(std::move(name), scopeLv, AllocReg(), startPC, 0));
    auto [it, suc] = locNames.try_emplace(newVar->name, newVar.get());
    if (!suc)
    {
        newVar->prev = it->second;
        it->second = newVar.get();
    }
    return newVar->slot;
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
    return GetJmpArgA(scopeLv);
}

slot_type FuncInfo::GetJmpArgA(uint32_t lv) const
{
    bool hasCapturedLocVars{};
    auto minSlotOfLocVars = maxRegs;
    for (auto&& [name, locVar] : locNames)
    {
        if (locVar && locVar->scopeLv == lv)
        {
            for (auto v = locVar; v && v->scopeLv == lv; v = v->prev)
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
}

void lua::FuncInfo::AddGotoJmp(const std::string& label, size_t pc, uint32_t line)
{
    const std::string* pName{};
    auto it = std::find_if(labels.rbegin(), labels.rend(),
                           [&](auto pset)
                           {
                               if (!pset)
                                   return false;
                               auto it = pset->find(label);
                               auto res = it != pset->end();
                               if (res)
                                   pName = &(*it);
                               return res;
                           });
    if (it == labels.rend())
    {
        Error(nullptr, line, "no visible label '" + label + "' for <goto>");
        return;
    }

    auto& info = labelsInfo[pName];
    if (!info)
        info = std::make_unique<LabelInfo>(-1);
    auto lv = (uint32_t)std::distance(it, labels.rend()) - 1;

    auto i = uint32_t(Op::JMP) | GetJmpArgA(lv + 1) << 6;
    if (info->pc == size_t(-1))
    {
        info = std::make_unique<LabelInfo>();
        info->gotos.emplace_back(pc, CurrentLocal(lv));
    }
    else
    {
        i |= (info->pc - pc + cv::MAXARG_sBx - 1) << 14;
    }
    insts[pc] = i;
}

void lua::FuncInfo::CloseOpenUpvals(uint32_t line)
{
    auto a = GetJmpArgA();
    if (a > 0)
    {
        EmitJmp(line, a, 0);
    }
}

void lua::FuncInfo::FixGotoSbx(const std::string& name, uint32_t line)
{
    auto& nameInLabel = *labels.back()->find(name);
    auto [it, res] = labelsInfo.try_emplace(&nameInLabel, nullptr);
    if (res)
    {
        it->second = std::make_unique<LabelInfo>(PC());
    }
    else
    {
        auto& info = *it->second;
        if (info.pc == size_t(-1))
        {
            bool jmpInlocal = !locVars.empty() && !localScope;
            if (jmpInlocal)
            {
                auto localVar = info.gotos.front().local;
                if (localVar)
                    jmpInlocal = locVars.back()->slot > localVar->slot;
            }
            if (jmpInlocal)
            {
                localScope = std::make_unique<LocalScopeInfo>(&nameInLabel, &locVars.back()->name, line);
            }
            info.pc = PC();
            for (auto& [pc, slot] : info.gotos)
            {
                // insts[pc] &= (1 << 14) - 1;
                insts[pc] |= (info.pc - pc + cv::MAXARG_sBx - 1) << 14;
            }
            info.gotos.clear();
        }
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
        if ((*it)->name == name)
        {
            (*it)->endPC += delta;
            return;
        }
    }
}

void lua::FuncInfo::EmitInstruction(uint32_t line, uint32_t i)
{
    insts.emplace_back(i);
    lineNums.emplace_back(line);
}

void lua::FuncInfo::EmitABC(uint32_t line, Op opcode, slot_type a, slot_type b, slot_type c)
{
    auto i = uint32_t(opcode) | a << 6 | b << 23 | c << 14;
    EmitInstruction(line, i);
}

void lua::FuncInfo::EmitABx(uint32_t line, Op opcode, slot_type a, int32_t bx)
{
    auto i = uint32_t(opcode) | a << 6 | bx << 14;
    EmitInstruction(line, i);
}

void lua::FuncInfo::EmitAsBx(uint32_t line, Op opcode, slot_type a, int32_t bx)
{
    auto i = uint32_t(opcode) | a << 6 | (bx + cv::MAXARG_sBx) << 14;
    EmitInstruction(line, i);
}

void lua::FuncInfo::EmitAx(uint32_t line, Op opcode, int32_t ax)
{
    auto i = uint32_t(opcode) | ax << 6;
    EmitInstruction(line, i);
}

void lua::FuncInfo::EmitReturn(uint32_t line, slot_type a, slot_type n)
{
    EmitABC(line, Op::RETURN, a, n + 1, 0);
}

void lua::FuncInfo::EmitClosure(uint32_t line, slot_type a, int32_t bx)
{
    EmitABx(line, Op::CLOSURE, a, bx);
}

void lua::FuncInfo::EmitTailCall(uint32_t line, slot_type a, slot_type n)
{
    EmitABC(line, Op::TAILCALL, a, n + 1, 0);
}

void lua::FuncInfo::EmitCall(uint32_t line, slot_type a, slot_type b, slot_type c)
{
    EmitABC(line, Op::CALL, a, b + 1, c + 1);
}

void lua::FuncInfo::EmitLoadNil(uint32_t line, slot_type a, slot_type n)
{
    if (n > 0)
        EmitABC(line, Op::LOADNIL, a, n - 1, 0);
}

void lua::FuncInfo::EmitLoadBool(uint32_t line, slot_type a, slot_type b, slot_type c)
{
    EmitABC(line, Op::LOADBOOL, a, b, c);
}

void lua::FuncInfo::EmitLoadK(uint32_t line, slot_type a, any_type k)
{
    auto idx = (int32_t)IndexOfConstant(std::move(k));
    if (idx < (1 << 18))
    {
        EmitABx(line, Op::LOADK, a, idx);
    }
    else
    {
        EmitABx(line, Op::LOADKX, a, 0);
        EmitAx(line, Op::EXTRAARG, idx);
    }
}

void lua::FuncInfo::EmitVararg(uint32_t line, slot_type a, slot_type n)
{
    EmitABC(line, Op::VARARG, a, n + 1, 0);
}

void lua::FuncInfo::EmitMove(uint32_t line, slot_type a, slot_type b)
{
    EmitABC(line, Op::MOVE, a, b, 0);
}

void lua::FuncInfo::EmitSetUpval(uint32_t line, slot_type a, slot_type b)
{
    EmitABC(line, Op::SETUPVAL, a, b, 0);
}

void lua::FuncInfo::EmitGetUpval(uint32_t line, slot_type a, slot_type b)
{
    EmitABC(line, Op::GETUPVAL, a, b, 0);
}

void lua::FuncInfo::EmitSetTable(uint32_t line, slot_type a, slot_type b, slot_type c)
{
    EmitABC(line, Op::SETTABLE, a, b, c);
}

void lua::FuncInfo::EmitGetTable(uint32_t line, slot_type a, slot_type b, slot_type c)
{
    EmitABC(line, Op::GETTABLE, a, b, c);
}

void lua::FuncInfo::EmitSetTabUp(uint32_t line, slot_type a, slot_type b, slot_type c)
{
    EmitABC(line, Op::SETTABUP, a, b, c);
}

void lua::FuncInfo::EmitGetTabUp(uint32_t line, slot_type a, slot_type b, slot_type c)
{
    EmitABC(line, Op::GETTABUP, a, b, c);
}

size_t lua::FuncInfo::EmitJmp(uint32_t line, slot_type a, int32_t sBx)
{
    EmitAsBx(line, Op::JMP, a, sBx);
    return PC() - 1;
}

void lua::FuncInfo::EmitTest(uint32_t line, slot_type a, slot_type c)
{
    EmitABC(line, Op::TEST, a, 0, c);
}

void lua::FuncInfo::EmitTestSet(uint32_t line, slot_type a, slot_type b, slot_type c)
{
    EmitABC(line, Op::TESTSET, a, b, c);
}

size_t lua::FuncInfo::EmitForPrep(uint32_t line, slot_type a, int32_t sBx)
{
    EmitAsBx(line, Op::FORPREP, a, sBx);
    return PC() - 1;
}

size_t lua::FuncInfo::EmitForLoop(uint32_t line, slot_type a, int32_t sBx)
{
    EmitAsBx(line, Op::FORLOOP, a, sBx);
    return PC() - 1;
}

void lua::FuncInfo::EmitTForCall(uint32_t line, slot_type a, slot_type c)
{
    EmitABC(line, Op::TFORCALL, a, 0, c);
}

void lua::FuncInfo::EmitTForLoop(uint32_t line, slot_type a, int32_t sBx)
{
    EmitAsBx(line, Op::TFORLOOP, a, sBx);
}

void lua::FuncInfo::EmitSelf(uint32_t line, slot_type a, slot_type b, slot_type c)
{
    EmitABC(line, Op::SELF, a, b, c);
}

void lua::FuncInfo::EmitNewTable(uint32_t line, slot_type a, slot_type nArr, slot_type nRec)
{
    EmitABC(line, Op::NEWTABLE, a, Int2fb(nArr), Int2fb(nRec));
}

void lua::FuncInfo::EmitSetList(uint32_t line, slot_type a, slot_type b, slot_type c)
{
    if (c < (1 << 9))
    {
        EmitABC(line, Op::SETLIST, a, b, c);
    }
    else
    {
        EmitABC(line, Op::SETLIST, a, b, 0);
        EmitAx(line, Op::EXTRAARG, c - 1);
    }
}

void lua::FuncInfo::EmitConcat(uint32_t line, slot_type a, slot_type b, slot_type c)
{
    EmitABC(line, Op::CONCAT, a, b, c);
}

void lua::FuncInfo::ToProto(Prototype& proto)
{
    proto.LineDefined = line;
    proto.LastLineDefined = lastLine;
    proto.NumParams = (uint8_t)numParams;
    proto.MaxStackSize = uint8_t(maxRegs);
    proto.Code = std::move(insts);
    GetConstants(proto.Constants);
    GetUpvalues(proto.Upvalues);
    ToSubProtos(proto);
    proto.LineInfo = std::move(lineNums);
    GetLocVars(proto.LocVars);
    GetUpvalueNames(proto.UpvalueNames);
    if (line == 0)
        proto.LastLineDefined = 0;

    if (proto.MaxStackSize < 2)
    {
        proto.MaxStackSize = 2;
    }
    if (isVararg)
    {
        proto.IsVararg = 1;
    }
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
        v.emplace_back(std::move(locVar->name), uint32_t(locVar->startPC), uint32_t(locVar->endPC));
    }
}

void lua::FuncInfo::ToSubProtos(Prototype& p)
{
    auto& v = p.Protos;
    v.reserve(subFuncs.size());
    for (auto&& fi : subFuncs)
    {
        fi->ToProto(v.emplace_back());
        v.back().Parent = &p;
    }
}

const TopPrototype* lua::Prototype::Top() const
{
    auto p = this;
    while (p->Parent)
    {
        p = p->Parent;
    }
    return static_cast<const TopPrototype*>(p);
}

LocVarInfo* lua::FuncInfo::CurrentLocal(uint32_t lv) const
{
    auto it = std::find_if(locVars.rbegin(), locVars.rend(), [&](auto& var) { return lv == var->scopeLv; });
    return it != locVars.rend() ? it->get() : nullptr;
}
