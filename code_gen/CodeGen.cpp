#include "CodeGen.h"
#include <charconv>

using namespace lua;

std::string lua::CodeGen::Unescape(std::string_view src)
{
    std::string dst;
    auto escapeD = [&](size_t& i)
    {
        auto si = i;
        for (size_t j = 0; j < 3; j++)
        {
            if (src[i] <= '9' && src[i] >= '0')
            {
                ++i;
            }
            else
            {
                break;
            }
        }
        auto cs = src.data();
        int16_t n;
        std::from_chars(cs + si, cs + i, n);
        --i;
        if (n < 256)
        {
            dst.push_back(char(n));
        }
    };
    auto escapeX = [&](size_t& i)
    {
        auto cs = src.data();
        int16_t n;
        std::from_chars(cs + i, cs + i + 2, n, 16);
        ++i;
        if (n < 256)
        {
            dst.push_back(char(n));
        }
    };

    for (size_t i = 0; i < src.size(); ++i)
    {
        if (src[i] == '\\' && i + 1 < src.size())
        {
            ++i;
            if (src[i] <= '9' && src[i] >= '0')
            {
                escapeD(i);
            }
            else
            {
                switch (src[i])
                {
                case 'a':
                    dst += '\a';
                    break;
                case 'b':
                    dst += '\b';
                    break;
                case 'f':
                    dst += '\f';
                    break;
                case 'n':
                    dst += '\n';
                    break;
                case 'r':
                    dst += '\r';
                    break;
                case 't':
                    dst += '\t';
                    break;
                case 'v':
                    dst += '\v';
                    break;
                case 'z':
                    while (src[i + 1] == ' ' || src[i + 1] == '\r' || src[i + 1] == '\n' || src[i + 1] == '\t')
                        ++i;
                    break;
                case 'x':
                    escapeD(++i);
                    break;
                case 'u':
                    // TODO utf8
                    break;
                case '\'':
                case '\"':
                case '\\':
                case '\n':
                case '|':
                case '$':
                case '#':
                    dst += src[i];
                    break;
                case '\r':
                    break;
                default:
                    dst += '\\';
                    dst += src[i];
                }
            }
        }
    }
    return dst;
}

std::string lua::CodeGen::TrimLong(const std::string& str)
{
    auto off = str.find('[', 1) + 1;
    auto newline = str[off] == '\n' ? 1 : 0;
    auto subs = str.substr(off + newline, str.size() - off * 2 - newline);
    return subs;
}

void lua::CodeGen::VisitWithPara(slot_type a, slot_type n, LuaRuleContext* ctx)
{
    auto p = GetPara();
    _a = a;
    _n = n;
    ctx->accept(this);
    SetPara(p);
}

bool lua::CodeGen::IsVarargOrFuncCall(LuaRuleContext* exp)
{
    if (exp->getRuleIndex() == LuaParser::RuleExp)
    {
        auto an = exp->getAltNumber();
        if (an == size_t(LuaRuleContext::Exp::Prefixexp_))
        {
            auto pre = static_cast<LuaParser::Prefixexp_Context*>(exp)->prefixexp();
            if (2 == pre->getAltNumber() && static_cast<LuaParser::CallindexContext*>(pre)->member().empty())
            {
                return true;
            }
        }
        else if (an == size_t(LuaRuleContext::Exp::Varargexp))
        {
            return true;
        }
    }
    return false;
}

void lua::CodeGen::RemoveTailNils(std::vector<LuaParser::ExpContext*>& exps)
{
    if (exps.empty())
        return;
    auto it = std::find_if(exps.rbegin(), exps.rend(), [](auto node) { return node->getAltNumber() != 1; });
    exps.erase(it.base());
}

std::array<slot_type, CodeGen::NP> lua::CodeGen::GetPara() const
{
    return {_a, slot_type(_n)};
}

void lua::CodeGen::SetPara(std::array<slot_type, NP> p)
{
    _a = p[0];
    _n = p[1];
}

int64_t lua::CodeGen::ToInt(const std::string& str)
{
    auto cs = str.c_str();
    int64_t val;
    std::from_chars(cs, cs + str.size(), val);
    return val;
}

int64_t lua::CodeGen::ToHex(const std::string& str)
{
    auto cs = str.c_str();
    int64_t val;
    std::from_chars(cs + 2, cs + str.size(), val, 16);
    return val;
}

double lua::CodeGen::ToFloat(const std::string& str)
{
    auto cs = str.c_str();
    double val;
    std::from_chars(cs, cs + str.size(), val);
    return val;
}

double lua::CodeGen::ToHexFloat(const std::string& str)
{
    auto cs = str.c_str();
    double val;
    std::from_chars(cs + 2, cs + str.size(), val, std::chars_format::hex);
    return val;
}

std::pair<slot_type, uint8_t> lua::CodeGen::ExpToOpArg(LuaParser::ExpContext* node, uint8_t argKinds)
{
    if (argKinds & Arg::CONST)
    {
        slot_type idx = -1;
        if (node->getRuleIndex() == LuaParser::RuleExp)
        {
            switch (node->getAltNumber())
            {
            case (size_t)LuaRuleContext::Exp::Nil:
                idx = fi->IndexOfConstant(nullptr);
                break;
            case (size_t)LuaRuleContext::Exp::False:
                idx = fi->IndexOfConstant(false);
                break;
            case (size_t)LuaRuleContext::Exp::True:
                idx = fi->IndexOfConstant(true);
                break;
            case (size_t)LuaRuleContext::Exp::Number_:
            {
                auto num = static_cast<LuaParser::Number_Context*>(node)->number();
                switch (num->getAltNumber())
                {
                case (size_t)LuaRuleContext::Number::Int:
                {
                    auto str = static_cast<LuaParser::IntContext*>(num)->INT()->getText();
                    idx = fi->IndexOfConstant(ToInt(str));
                }
                break;
                case (size_t)LuaRuleContext::Number::Hex:
                {
                    auto str = static_cast<LuaParser::HexContext*>(num)->HEX()->getText();
                    idx = fi->IndexOfConstant(ToHex(str));
                }
                break;
                case (size_t)LuaRuleContext::Number::Float:
                {
                    auto str = static_cast<LuaParser::FloatContext*>(num)->FLOAT()->getText();
                    idx = fi->IndexOfConstant(ToFloat(str));
                }
                break;
                case (size_t)LuaRuleContext::Number::Hexfloat:
                {
                    auto str = static_cast<LuaParser::HexfloatContext*>(num)->HEX_FLOAT()->getText();
                    idx = fi->IndexOfConstant(ToHexFloat(str));
                }
                break;
                default:
                    break;
                }
            }
            break;
            case (size_t)LuaRuleContext::Exp::String_:
            {
                auto sn = static_cast<LuaParser::String_Context*>(node)->string();
                switch (sn->getAltNumber())
                {
                case (size_t)LuaRuleContext::String::Normalstring:
                {
                    auto str = static_cast<LuaParser::NormalstringContext*>(sn)->NORMALSTRING()->getText();
                    std::string_view sv(str.begin() + 1, str.end() - 1);
                    idx = fi->IndexOfConstant(Unescape(sv));
                }
                break;
                case (size_t)LuaRuleContext::String::Charstring:
                {
                    auto str = static_cast<LuaParser::CharstringContext*>(sn)->CHARSTRING()->getText();
                    std::string_view sv(str.begin() + 1, str.end() - 1);
                    idx = fi->IndexOfConstant(Unescape(sv));
                }
                break;
                default:
                {
                    auto str = static_cast<LuaParser::LongstringContext*>(sn)->LONGSTRING()->getText();
                    idx = fi->IndexOfConstant(TrimLong(str));
                }
                break;
                }
            }
            break;
            default:
                break;
            }
            if (idx >= 0 && idx <= 0xff)
            {
                return {0x100 + idx, Arg::CONST};
            }
        }
    }
    auto a = fi->AllocReg();
    VisitWithPara(a, 1, node);
    return {a, Arg::REG};
}

std::pair<slot_type, slot_type> lua::CodeGen::ExpToOpArg(const std::vector<LuaParser::ExpContext*>& nodes,
                                                         uint8_t argKinds)
{
    auto b = ExpToOpArg(nodes.front(), argKinds).first;
    auto c = ExpToOpArg(nodes.back(), argKinds).first;
    return {b, c};
}

std::pair<slot_type, uint8_t> lua::CodeGen::NameToOpArg(const std::string& name, uint8_t argKinds)
{
    if (argKinds & Arg::REG)
    {
        if (auto r = fi->SlotOfLocVar(name); r >= 0)
        {
            return {r, Arg::REG};
        }
    }
    if (argKinds & Arg::UPVAL)
    {
        if (auto r = fi->IndexOfUpval(name); r >= 0)
        {
            return {r, Arg::UPVAL};
        }
    }

    auto a = fi->AllocReg();
    VisitNAME(name, a);
    return {a, Arg::REG};
}

slot_type lua::CodeGen::ConstantToOpArg(const any_type& cv)
{
    auto idx = fi->IndexOfConstant(cv);
    if (idx <= 0xff)
    {
        return idx + 0x100;
    }

    // Load constant
    auto a = fi->AllocReg();
    switch (cv.index())
    {
    case 0:
        fi->EmitLoadNil(a, 1);
        break;
    case 1:
        fi->EmitLoadBool(a, std::get<bool>(cv), 0);
        break;
    default:
        fi->EmitLoadK(a, cv);
        break;
    }

    return a;
}

slot_type lua::CodeGen::VisitMember(LuaParser::MemberContext* member)
{
    auto an = member->getAltNumber();
    std::pair<slot_type, uint8_t> p;
    if (1 == an)
    {
        auto exp = static_cast<LuaParser::IndexContext*>(member)->exp();
        p = ExpToOpArg(exp, Arg::RK);
    }
    else
    {
        auto name = static_cast<LuaParser::AccessContext*>(member)->NAME();
        p.first = ConstantToOpArg(name->getText());
    }
    return p.first;
}

Prototype lua::CodeGen::Generate(LuaParser::ChunkContext* ck)
{
    root = std::make_unique<FuncInfo>();
    root->AddLocVar("_ENV", 0);
    fi = root.get();

    visitChunk(ck);

    return ToProto();
}

Prototype lua::CodeGen::ToProto() const
{
    return root->ToProto();
}

std::any lua::CodeGen::DoVisitFuncbody(LuaParser::FuncbodyContext* ctx, bool self)
{
    auto a = _a;
    auto& subFi = fi->subFuncs.emplace_back(fi);

    if (self)
    {
        subFi.AddLocVar("self", 0);
    }
    auto plist = ctx->parlist();
    auto an = plist->getAltNumber();
    subFi.isVararg = 2 == an;
    if (1 == an)
    {
        auto nplist = static_cast<LuaParser::NameparlistContext*>(plist);
        auto names = nplist->namelist()->NAME();
        for (auto&& nn : names)
        {
            subFi.AddLocVar(nn->getText(), 0);
        }
        subFi.isVararg = nplist->DDD();
    }

    fi = &subFi;
    visitBlock(ctx->block());
    fi = subFi.parent;
    subFi.RemoveScopeLocVars(true, subFi.PC() + 1);
    subFi.EmitReturn(0, 0);
    fi->EmitClosure(a, fi->subFuncs.size() - 1);
    return std::any();
}

std::any lua::CodeGen::visitChunk(LuaParser::ChunkContext* ctx)
{
    auto& subFi = fi->subFuncs.emplace_back(fi);
    fi = &subFi;
    visitBlock(ctx->block());
    fi = subFi.parent;
    subFi.RemoveScopeLocVars(true, subFi.PC() + 1);
    subFi.EmitReturn(0, 0);
    return std::any();
}

std::any lua::CodeGen::visitFuncbody(LuaParser::FuncbodyContext* ctx)
{
    return DoVisitFuncbody(ctx, false);
}

std::any lua::CodeGen::visitBlock(LuaParser::BlockContext* ctx)
{
    auto stats = ctx->stat();
    for (auto&& stat : stats)
    {
        stat->accept(this);
    }
    if (auto retstat = ctx->retstat())
    {
        visitRetstat(retstat);
    }
    return std::any();
}

std::any lua::CodeGen::visitRetstat(LuaParser::RetstatContext* ctx)
{
    auto exps = ctx->explist()->exp();
    auto sz = exps.size();
    if (!sz)
    {
        fi->EmitReturn(0, 0);
        return {};
    }
    if (1 == sz)
    {
        auto exp = exps.front();
        if (exp->getAltNumber() == size_t(LuaRuleContext::Exp::Prefixexp_))
        {
            auto pre = static_cast<LuaParser::Prefixexp_Context*>(exp)->prefixexp();
            switch (pre->getAltNumber())
            {
            case 1:
            {
                auto ni = static_cast<LuaParser::NameindexContext*>(pre);
                if (ni->member().empty())
                {
                    auto name = ni->NAME()->getText();
                    if (auto r = fi->SlotOfLocVar(name); r >= 0)
                    {
                        fi->EmitReturn(r, 1);
                        return {};
                    }
                }
            }
            break;
            case 2:
            {
                auto fci = static_cast<LuaParser::CallindexContext*>(pre);
                if (fci->member().empty())
                {
                    auto r = fi->AllocReg();
                    auto nArgs = PrepFuncCall(fci->functioncall(), r);
                    fi->EmitTailCall(r, nArgs);
                    fi->FreeReg();
                    fi->EmitReturn(r, -1);
                    return {};
                }
            }
            break;
            default:
                break;
            }
        }
    }

    auto multRet = IsVarargOrFuncCall(exps.back());
    for (size_t i = 0; i < sz; i++)
    {
        auto r = fi->AllocReg();
        // visitExp(exps[i], r, i + 1 == sz && multRet ? -1 : 1);
        VisitWithPara(r, i + 1 == sz && multRet ? -1 : 1, exps[i]);
    }

    auto nExps = (slot_type)sz;
    fi->FreeRegs(nExps);
    fi->EmitReturn(fi->usedRegs, multRet ? -1 : nExps);  // TODO

    return std::any();
}

// void lua::CodeGen::visitExp(LuaParser::ExpContext* ctx, slot_type a, slot_type n)
// {
//     auto p = GetPara();
//     _a = a;
//     _n = n;
//     switch (ctx->getAltNumber())
//     {
//     case size_t(LuaRuleContext::Exp::Nil):
//         fi->EmitLoadNil(a, n);
//         break;
//     case size_t(LuaRuleContext::Exp::False):
//         fi->EmitLoadBool(a, 0, 0);
//         break;
//     case size_t(LuaRuleContext::Exp::True):
//         fi->EmitLoadBool(a, 1, 0);
//         break;
//     case size_t(LuaRuleContext::Exp::Varargexp):
//         visitVarargexp(static_cast<LuaParser::VarargexpContext*>(ctx));
//         break;

//     default:
//         visitChildren(ctx);
//         break;
//     }

//     SetPara(p);
// }

void lua::CodeGen::DoVarDecl(const std::vector<LuaParser::ExpContext*>& exps, std::vector<std::string>&& names)
{
    auto nExps = exps.size();
    auto nNames = names.size();
    auto oldRegs = fi->usedRegs;
    if (nExps == nNames)
    {
        for (auto&& exp : exps)
        {
            VisitWithPara(fi->AllocReg(), 1, exp);
        }
    }
    else if (nExps > nNames)
    {
        for (size_t i = 0; i < nExps; i++)
        {
            auto exp = exps[i];
            auto a = fi->AllocReg();
            slot_type n = i + 1 == nExps && IsVarargOrFuncCall(exp) ? 0 : 1;
            VisitWithPara(a, n, exp);
        }
    }
    else
    {
        bool multRet{};
        auto n = nNames - nExps;
        for (size_t i = 0; i < nExps; i++)
        {
            auto exp = exps[i];
            auto a = fi->AllocReg();
            if (i + 1 == nExps && IsVarargOrFuncCall(exp))
            {
                multRet = true;
                VisitWithPara(a, n + 1, exp);
                fi->AllocRegs(n);
            }
            else
            {
                VisitWithPara(a, 1, exp);
            }
        }
        if (!multRet)
        {
            auto a = fi->AllocRegs(n);
            fi->EmitLoadNil(a, n);
        }
    }
    fi->usedRegs = oldRegs;
    auto startPC = fi->PC();
    for (auto&& name : names)
    {
        fi->AddLocVar(std::move(name), startPC);
    }
}

std::any lua::CodeGen::visitAssign(LuaParser::AssignContext* ctx)
{
    auto explist = ctx->explist();
    std::vector<LuaParser::ExpContext*> exps;
    if (explist)
    {
        exps = ctx->explist()->exp();
    }
    RemoveTailNils(exps);
    auto nExps = exps.size();
    auto vars = ctx->varlist()->var();
    auto nVars = vars.size();
    auto oldRegs = fi->usedRegs;

    std::vector<slot_type> tRegs(nVars);
    auto kRegs = tRegs;

    for (size_t i = 0; i < nVars; i++)
    {
        if (vars[i]->getAltNumber() == 1)
        {
            auto name = static_cast<LuaParser::NormalvarContext*>(vars[i])->NAME()->getText();
            if (fi->SlotOfLocVar(name) < 0 && fi->IndexOfUpval(name) < 0)
            {
                kRegs[i] = -1;
                if (fi->IndexOfConstant(std::move(name)) > 0xff)
                {
                    kRegs[i] = fi->AllocReg();
                }
            }
        }
        else
        {
            auto indexTable = static_cast<LuaParser::IndextableContext*>(vars[i]);
            tRegs[i] = fi->AllocReg();
            VisitWithPara(tRegs[i], 1, indexTable->prefixexp());
            kRegs[i] = fi->AllocReg();
            VisitWithPara(kRegs[i], 1, indexTable->member());
        }
    }

    std::vector<slot_type> vRegs(nVars);
    for (size_t i = 0; i < nVars; i++)
    {
        vRegs[i] = fi->usedRegs + i;
    }
    if (nExps >= nVars)
    {
        for (size_t i = 0; i < nExps; i++)
        {
            auto exp = exps[i];
            auto a = fi->AllocReg();
            slot_type n = i >= nVars && i + 1 == nExps && IsVarargOrFuncCall(exp) ? 0 : 1;
            VisitWithPara(a, n, exp);
        }
    }
    else
    {
        bool multRet{};
        auto n = nVars - nExps;
        for (size_t i = 0; i < nExps; i++)
        {
            auto exp = exps[i];
            auto a = fi->AllocReg();
            if (i + 1 == nExps && IsVarargOrFuncCall(exp))
            {
                multRet = true;
                VisitWithPara(a, n + 1, exp);
                fi->AllocRegs(n);
            }
            else
            {
                VisitWithPara(a, 1, exp);
            }
        }
        if (!multRet)
        {
            auto a = fi->AllocRegs(n);
            fi->EmitLoadNil(a, n);
        }
    }
    for (size_t i = 0; i < nVars; i++)
    {
        auto exp = vars[i];
        if (exp->getAltNumber() == 1)
        {
            auto varName = static_cast<LuaParser::NormalvarContext*>(vars[i])->NAME()->getText();
            if (auto a = fi->SlotOfLocVar(varName); a >= 0)
            {
                fi->EmitMove(a, vRegs[i]);
            }
            else if (auto b = fi->IndexOfUpval(varName); b >= 0)
            {
                fi->EmitSetUpval(vRegs[i], b);
            }
            else
            {
                slot_type g = fi->SlotOfLocVar("_ENV");
                bool up = g < 0;
                if (!up)
                {
                    g = fi->IndexOfUpval("_ENV");
                }
                auto c = kRegs[i] < 0 ? 0x100 + fi->IndexOfConstant(varName) : kRegs[i];
                if (up)
                {
                    fi->EmitSetTabUp(g, c, vRegs[i]);
                }
                else
                {
                    fi->EmitSetTable(g, c, vRegs[i]);
                }
            }
        }
        else
        {
            fi->EmitSetTable(tRegs[i], kRegs[i], vRegs[i]);
        }
    }
    fi->usedRegs = oldRegs;  // TODO

    return std::any();
}

std::any lua::CodeGen::visitFunctioncall_(LuaParser::Functioncall_Context* ctx)
{
    auto p = GetPara();
    _a = fi->AllocReg();
    _n = 0;
    visitFunctioncall(ctx->functioncall());
    SetPara(p);
    fi->FreeReg();
    return std::any();
}

std::any lua::CodeGen::visitLabel(LuaParser::LabelContext* ctx)
{
    // TODO
    return LuaParserBaseVisitor::visitLabel(ctx);
}

std::any lua::CodeGen::visitBreak(LuaParser::BreakContext* ctx)
{
    auto pc = fi->EmitJmp(0, 0);
    fi->AddBreakJmp(pc);
    return std::any();
}

std::any lua::CodeGen::visitGoto(LuaParser::GotoContext* ctx)
{
    // TODO
    return LuaParserBaseVisitor::visitGoto(ctx);
}

std::any lua::CodeGen::visitDo(LuaParser::DoContext* ctx)
{
    fi->EnterScope(false);
    visitBlock(ctx->block());
    fi->CloseOpenUpvals();
    fi->ExitScope(fi->PC());
    return std::any();
}

std::any lua::CodeGen::visitWhile(LuaParser::WhileContext* ctx)
{
    auto pcBeforeExp = fi->PC();
    auto oldRegs = fi->usedRegs;
    auto [a, _] = ExpToOpArg(ctx->exp(), Arg::REG);
    fi->usedRegs = oldRegs;
    fi->EmitTest(a, 0);
    auto pcJmpToEnd = fi->EmitJmp(0, 0);

    fi->EnterScope(true);
    visitBlock(ctx->block());
    fi->CloseOpenUpvals();
    fi->EmitJmp(0, pcBeforeExp - fi->PC() - 1);
    fi->ExitScope(fi->PC() - 1);
    fi->FixSbx(pcJmpToEnd, fi->PC() - pcJmpToEnd - 1);

    return std::any();
}

std::any lua::CodeGen::visitRepeat(LuaParser::RepeatContext* ctx)
{
    fi->EnterScope(true);
    auto pcBeforeBlock = fi->PC();
    visitBlock(ctx->block());
    auto oldRegs = fi->usedRegs;
    auto [a, _] = ExpToOpArg(ctx->exp(), Arg::REG);
    fi->usedRegs = oldRegs;
    fi->EmitTest(a, 0);
    fi->EmitJmp(fi->GetJmpArgA(), pcBeforeBlock - fi->PC() - 1);
    fi->CloseOpenUpvals();
    fi->ExitScope(fi->PC());

    return std::any();
}

std::any lua::CodeGen::visitIf(LuaParser::IfContext* ctx)
{
    auto exps = ctx->exp();
    auto blocks = ctx->block();
    if (blocks.size() > exps.size())
    {
        exps.emplace_back();
    }

    std::vector<size_t> pcJmpToEnds(exps.size());
    size_t pcJmpToNextExp = -1;

    for (size_t i = 0; i < exps.size(); i++)
    {
        if (pcJmpToNextExp != size_t(-1))
        {
            fi->FixSbx(pcJmpToNextExp, fi->PC() - pcJmpToNextExp - 1);
        }
        auto exp = exps[i];
        auto oldRegs = fi->usedRegs;
        auto a = exp ? ExpToOpArg(exp, Arg::REG).first : ConstantToOpArg(true);
        fi->usedRegs = oldRegs;
        fi->EmitTest(a, 0);
        pcJmpToNextExp = fi->EmitJmp(0, 0);

        auto block = blocks[i];
        fi->EnterScope(false);
        visitBlock(block);
        fi->CloseOpenUpvals();
        fi->ExitScope(fi->PC());
        pcJmpToEnds[i] = i + 1 < exps.size() ? fi->EmitJmp(0, 0) : pcJmpToNextExp;
    }

    for (auto&& pc : pcJmpToEnds)
    {
        fi->FixSbx(pc, fi->PC() - pc - 1);
    }

    return std::any();
}

std::any lua::CodeGen::visitFornumerical(LuaParser::FornumericalContext* ctx)
{
    static const std::string forIndexVar = "(for index)";
    static const std::string forLimitVar = "(for limit)";
    static const std::string forStepVar = "(for step)";

    fi->EnterScope(true);
    DoVarDecl(ctx->exp(), {forIndexVar, forLimitVar, forStepVar});
    fi->AddLocVar(ctx->NAME()->getText(), fi->PC() + 1);

    auto a = fi->usedRegs - 4;
    auto pcForPrep = fi->EmitForPrep(a, 0);
    visitBlock(ctx->block());
    fi->CloseOpenUpvals();
    auto pcForLoop = fi->EmitForLoop(a, 0);
    fi->FixSbx(pcForPrep, pcForLoop - pcForPrep - 1);
    fi->FixSbx(pcForLoop, pcForPrep - pcForLoop);
    fi->ExitScope(fi->PC() - 1);
    fi->FixEndPC(forIndexVar, 1);
    fi->FixEndPC(forLimitVar, 1);
    fi->FixEndPC(forStepVar, 1);
    return std::any();
}

std::any lua::CodeGen::visitForgeneric(LuaParser::ForgenericContext* ctx)
{
    static const std::string forGeneratorVar = "(for generator)";
    static const std::string forStateVar = "(for state)";
    static const std::string forControlVar = "(for control)";
    static const std::string forClosingVar = "(for closing)";

    auto exps = ctx->explist()->exp();
    fi->EnterScope(true);
    DoVarDecl(exps, {forGeneratorVar, forStateVar, forControlVar, forClosingVar});
    auto names = ctx->namelist()->NAME();
    for (auto&& nn : names)
    {
        fi->AddLocVar(nn->getText(), fi->PC() + 1);
    }

    auto pcJmpToTFC = fi->EmitJmp(0, 0);
    visitBlock(ctx->block());
    fi->CloseOpenUpvals();
    fi->FixSbx(pcJmpToTFC, fi->PC() - pcJmpToTFC - 1);

    auto r = fi->SlotOfLocVar(forGeneratorVar);
    fi->EmitTForCall(r, (slot_type)names.size());
    fi->EmitForLoop(r + 2, int32_t(pcJmpToTFC - fi->PC()));
    fi->ExitScope(fi->PC() - 1);
    fi->FixEndPC(forGeneratorVar, 2);
    fi->FixEndPC(forStateVar, 2);
    fi->FixEndPC(forControlVar, 2);
    fi->FixEndPC(forClosingVar, 2);

    return std::any();
}

std::any lua::CodeGen::visitFuncnamedef(LuaParser::FuncnamedefContext* ctx)
{
    auto fname = ctx->funcname();
    auto names = fname->NAME();
    auto a = fi->AllocReg();
    bool self{};
    if (names.size() > 1)
    {
        self = fname->COL();
        PrepFuncName(a, names.rbegin(), names.rend() - 1);
    }
    else
    {
        VisitNAME(names.front()->getText(), a);
    }

    auto old_a = _a;
    _a = a;
    DoVisitFuncbody(ctx->funcbody(), self);
    _a = old_a;
    fi->FreeReg();

    return std::any();
}

std::any lua::CodeGen::visitLocalfunc(LuaParser::LocalfuncContext* ctx)
{
    auto r = fi->AddLocVar(ctx->NAME()->getText(), fi->PC() + 1);
    auto old_a = _a;
    _a = r;
    visitFuncbody(ctx->funcbody());
    _a = old_a;
    return std::any();
}

std::any lua::CodeGen::visitVardecl(LuaParser::VardeclContext* ctx)
{
    auto explist = ctx->explist();
    std::vector<LuaParser::ExpContext*> exps;
    if (explist)
    {
        exps = ctx->explist()->exp();
    }
    RemoveTailNils(exps);

    auto nodes = ctx->attnamelist()->NAME();
    std::vector<std::string> names;
    names.reserve(nodes.size());
    for (auto&& n : nodes)
    {
        names.emplace_back(n->getText());
    }

    DoVarDecl(exps, std::move(names));

    return std::any();
}

std::vector<LuaRuleContext*> lua::CodeGen::GetArgs(LuaParser::ArgsContext* args)
{
    std::vector<LuaRuleContext*> res;
    if (1 == args->getAltNumber())
    {
        if (auto l = static_cast<LuaParser::NormalargContext*>(args)->explist())
        {
            auto exps = l->exp();
            res = reinterpret_cast<std::vector<LuaRuleContext*>&&>(exps);
        }
    }
    else
    {
        res.emplace_back(static_cast<LuaRuleContext*>(args->children.front()));
    }

    return res;
}

slot_type lua::CodeGen::PrepMemFuncCall(LuaParser::FunctioncallContext* node, slot_type a, part_iterator start,
                                        part_iterator end, mem_iterator start_m, mem_iterator end_m)
{
    if (start_m == end_m)
    {
        // no member, call with a
        if (start != end)
        {
            auto nArgs = PrepFuncCall(node, a, start, end);
            fi->EmitCall(a, nArgs, 1);
        }
        else if (auto name = node->NAME())
        {
            VisitNAME(name->getText(), a);
        }
        else
        {
            VisitWithPara(a, 1, node->exp());
        }
    }
    else
    {
        auto oldRegs = fi->usedRegs;
        auto r = fi->AllocReg();
        PrepMemFuncCall(node, r, start, end, std::next(start_m), end_m);

        auto member = *start_m;
        auto c = VisitMember(member);
        fi->usedRegs = oldRegs;
        fi->EmitGetTable(a, r, c);
    }

    return slot_type();
}

slot_type lua::CodeGen::PrepMemFuncCall(LuaParser::FunctioncallContext* node, slot_type a, part_iterator start,
                                        part_iterator end, MemberContexts& mems)
{
    return PrepMemFuncCall(node, a, start, end, mems.rbegin(), mems.rend());
}

slot_type lua::CodeGen::PrepFuncCall(LuaParser::FunctioncallContext* node, slot_type a, part_iterator start,
                                     part_iterator end)
{
    auto mems = (*start)->member();
    auto next = std::next(start);
    PrepMemFuncCall(node, a, next, end, mems);

    auto fArgs = (*start)->functionargs();
    auto an = fArgs->getAltNumber();

    LuaParser::ArgsContext* argsN{};
    if (2 == an)
    {
        auto nameArg = static_cast<LuaParser::NameargsContext*>(fArgs);
        auto name = nameArg->NAME();
        argsN = nameArg->args();
        fi->AllocReg();
        auto [c, k] = NameToOpArg(name->getText(), Arg::RK);
        fi->EmitSelf(a, a, c);
        if (Arg::REG == k)
        {
            fi->FreeRegs(1);
        }
    }
    else
    {
        argsN = static_cast<LuaParser::Args_Context*>(fArgs)->args();
    }

    auto args = GetArgs(argsN);
    auto nArgs = args.size();
    bool lastArgIsVarargOrFuncCall{};
    for (size_t i = 0; i < nArgs; i++)
    {
        auto arg = args[i];
        auto tmp = fi->AllocReg();
        int32_t n = 1;
        if (i + 1 == nArgs && IsVarargOrFuncCall(arg))
        {
            lastArgIsVarargOrFuncCall = true;
            n = -1;
        }
        VisitWithPara(tmp, n, arg);
    }
    fi->FreeRegs(nArgs);  // TODO lastArgIsVarargOrFuncCall = true ?

    if (2 == an)
    {
        fi->FreeReg();
        ++nArgs;
    }
    if (lastArgIsVarargOrFuncCall)
    {
        nArgs = -1;
    }

    return nArgs;
}

slot_type lua::CodeGen::PrepFuncCall(LuaParser::FunctioncallContext* node, slot_type r)
{
    auto callParts = node->callpart();
    return PrepFuncCall(node, r, callParts.rbegin(), callParts.rend());
}

void lua::CodeGen::PrepPrefix(LuaParser::PrefixexpContext* pre, slot_type a, mem_iterator start, mem_iterator end)
{
    auto next = std::next(start);
    std::pair<slot_type, uint8_t> p;

    auto oldRegs = fi->usedRegs;
    if (next == end)
    {
        auto k = Arg::RU;
        auto an = pre->getAltNumber();
        switch (an)
        {
        case 1:
        {
            auto name = static_cast<LuaParser::NameindexContext*>(pre)->NAME();
            p = NameToOpArg(name->getText(), k);
        }
        break;
        case 2:
        {
            auto fc = static_cast<LuaParser::CallindexContext*>(pre)->functioncall();
            p = {fi->AllocReg(), Arg::REG};
            VisitWithPara(p.first, 1, fc);
        }
        break;
        default:
        {
            auto exp = static_cast<LuaParser::ExpindexContext*>(pre)->exp();
            p = ExpToOpArg(exp, k);
        }
        break;
        }
    }
    else
    {
        p = {fi->AllocReg(), Arg::REG};
        PrepPrefix(pre, p.first, next, end);
    }

    auto c = VisitMember(*start);
    fi->usedRegs = oldRegs;
    Arg::UPVAL == p.second ? fi->EmitGetTabUp(a, p.first, c) : fi->EmitGetTable(a, p.first, c);
}

void lua::CodeGen::PrepPrefix(LuaParser::PrefixexpContext* pre, slot_type a, slot_type n, MemberContexts& mems)
{
    if (mems.empty())
    {
        auto an = pre->getAltNumber();
        switch (an)
        {
        case 1:
        {
            auto name = static_cast<LuaParser::NameindexContext*>(pre)->NAME();
            VisitNAME(name->getText(), a);
        }
        break;
        default:
        {
            VisitWithPara(a, n, pre);
        }
        break;
        }
    }
    else
    {
        PrepPrefix(pre, a, mems.rbegin(), mems.rend());
    }
}

slot_type lua::CodeGen::PrepFuncName(slot_type a, name_iterator start, name_iterator end)
{
    auto next = std::next(start);
    std::pair<slot_type, uint8_t> p;

    auto oldRegs = fi->usedRegs;
    if (next == end)
    {
        auto k = Arg::RU;
        auto name = *end;
        p = NameToOpArg(name->getText(), k);
    }
    else
    {
        p = {fi->AllocReg(), Arg::REG};
        PrepFuncName(p.first, next, end);
    }
    auto c = ConstantToOpArg((*start)->getText());
    fi->usedRegs = oldRegs;
    Arg::UPVAL == p.second ? fi->EmitGetTabUp(a, p.first, c) : fi->EmitGetTable(a, p.first, c);
    return {};
}

std::any lua::CodeGen::visitTerminal(antlr4::tree::TerminalNode* ctx)
{
    return std::any();
}

void lua::CodeGen::VisitNAME(const std::string& text, slot_type a)
{
    if (auto r = fi->SlotOfLocVar(text); r >= 0)
    {
        fi->EmitMove(a, r);
    }
    else if (auto idx = fi->IndexOfUpval(text); idx >= 0)
    {
        fi->EmitGetUpval(a, idx);
    }
    else
    {
        auto oldRegs = fi->usedRegs;
        auto [b, kindB] = NameToOpArg("_ENV", Arg::RU);
        auto c = ConstantToOpArg(text);
        fi->usedRegs = oldRegs;
        Arg::UPVAL == kindB ? fi->EmitGetTabUp(a, b, c) : fi->EmitGetTable(a, b, c);
    }
}

std::any lua::CodeGen::visitNil(LuaParser::NilContext* ctx)
{
    fi->EmitLoadNil(_a, _n);
    return std::any();
}

std::any lua::CodeGen::visitFalse(LuaParser::FalseContext* ctx)
{
    fi->EmitLoadBool(_a, 0, 0);
    return std::any();
}

std::any lua::CodeGen::visitTrue(LuaParser::TrueContext* ctx)
{
    fi->EmitLoadBool(_a, 1, 0);
    return std::any();
}

std::any lua::CodeGen::visitInt(LuaParser::IntContext* ctx)
{
    auto str = ctx->INT()->getText();
    fi->EmitLoadK(_a, ToInt(str));
    return std::any();
}

std::any lua::CodeGen::visitHex(LuaParser::HexContext* ctx)
{
    auto str = ctx->HEX()->getText();
    fi->EmitLoadK(_a, ToHex(str));
    return std::any();
}

std::any lua::CodeGen::visitFloat(LuaParser::FloatContext* ctx)
{
    auto str = ctx->FLOAT()->getText();
    fi->EmitLoadK(_a, ToFloat(str));
    return std::any();
}

std::any lua::CodeGen::visitHexfloat(LuaParser::HexfloatContext* ctx)
{
    auto str = ctx->HEX_FLOAT()->getText();
    fi->EmitLoadK(_a, ToHexFloat(str));
    return std::any();
}

std::any lua::CodeGen::visitNormalstring(LuaParser::NormalstringContext* ctx)
{
    auto str = ctx->NORMALSTRING()->getText();
    std::string_view sv(str.begin() + 1, str.end() - 1);
    fi->EmitLoadK(_a, Unescape(sv));
    return std::any();
}

std::any lua::CodeGen::visitCharstring(LuaParser::CharstringContext* ctx)
{
    auto str = ctx->CHARSTRING()->getText();
    std::string_view sv(str.begin() + 1, str.end() - 1);
    fi->EmitLoadK(_a, Unescape(sv));
    return std::any();
}

std::any lua::CodeGen::visitLongstring(LuaParser::LongstringContext* ctx)
{
    auto str = ctx->LONGSTRING()->getText();
    fi->EmitLoadK(_a, TrimLong(str));
    return std::any();
}

std::any lua::CodeGen::visitVarargexp(LuaParser::VarargexpContext* ctx)
{
    auto [a, n] = GetPara();
    if (!fi->isVararg)
    {
        // TODO error
    }
    fi->EmitVararg(a, n);
    return std::any();
}

std::any lua::CodeGen::visitNameindex(LuaParser::NameindexContext* ctx)
{
    auto mems = ctx->member();
    PrepPrefix(ctx, _a, _n, mems);
    return std::any();
}

std::any lua::CodeGen::visitCallindex(LuaParser::CallindexContext* ctx)
{
    auto mems = ctx->member();
    PrepPrefix(ctx, _a, _n, mems);
    return std::any();
}

std::any lua::CodeGen::visitExpindex(LuaParser::ExpindexContext* ctx)
{
    auto mems = ctx->member();
    PrepPrefix(ctx, _a, _n, mems);
    return std::any();
}

std::any lua::CodeGen::visitFunctioncall(LuaParser::FunctioncallContext* ctx)
{
    auto [a, n] = GetPara();
    auto nArgs = PrepFuncCall(ctx, a);
    fi->EmitCall(a, nArgs, n);
    return std::any();
}

std::any lua::CodeGen::visitTableconstructor(LuaParser::TableconstructorContext* ctx)
{
    auto a = _a;
    auto fl = ctx->fieldlist();
    if (!fl)
    {
        fi->EmitNewTable(a, 0, 0);
    }
    auto fields = fl->field();
    auto nArr = std::count_if(fields.begin(), fields.end(), [](auto n) { return n->getAltNumber() < 3; });
    auto last = fields.back();
    auto multRet =
        last->getAltNumber() == 3 && IsVarargOrFuncCall(static_cast<LuaParser::ExpfieldContext*>(last)->exp());
    auto nExps = fields.size();

    size_t arrIdx{};
    for (size_t i = 0; i < nExps; i++)
    {
        auto fd = fields[i];
        auto an = fd->getAltNumber();
        if (an == 3)
        {
            ++arrIdx;
            auto valExp = static_cast<LuaParser::ExpfieldContext*>(last)->exp();
            auto tmp = fi->AllocReg();
            auto lastMult = i + 1 == nExps && multRet;
            VisitWithPara(tmp, lastMult ? -1 : 1, valExp);
            constexpr auto flushSize = 50;
            if (arrIdx % flushSize == 0 || arrIdx == nArr)
            {
                auto n = arrIdx % flushSize;
                if (!n)
                    n = flushSize;
                fi->FreeRegs(n);
                auto c = (arrIdx - 1) / 50 + 1;
                fi->EmitSetList(a, lastMult ? 0 : n, c);
            }
            continue;
        }

        LuaParser::ExpContext* valExp{};
        slot_type b = fi->AllocReg();
        if (an == 1)
        {
            auto ifd = static_cast<LuaParser::IndexedfieldContext*>(last);
            auto exps = ifd->exp();
            valExp = exps.back();
            VisitWithPara(b, 1, exps.front());
        }
        else
        {
            auto nfd = static_cast<LuaParser::NamedfieldContext*>(last);
            valExp = nfd->exp();
            fi->EmitLoadK(b, nfd->NAME()->getText());
        }
        slot_type c = fi->AllocReg();
        VisitWithPara(c, 1, valExp);
        fi->FreeRegs(2);
        fi->EmitSetTable(a, b, c);
    }

    return std::any();
}

template <typename T>
void lua::CodeGen::VisitLogical(LuaParser::ExpContext* ctx)
{
    auto a = _a;
    auto oldRegs = fi->usedRegs;
    auto exps = static_cast<T*>(ctx)->exp();
    auto b = ExpToOpArg(exps.front(), Arg::REG).first;
    fi->usedRegs = oldRegs;
    constexpr auto c = std::is_same_v<LuaParser::AndContext, T> ? 0 : 1;
    fi->EmitTestSet(a, b, c);
    auto pcOfJmp = fi->EmitJmp(0, 0);

    b = ExpToOpArg(exps.back(), Arg::REG).first;
    fi->usedRegs = oldRegs;
    fi->EmitMove(a, b);
    fi->FixSbx(pcOfJmp, fi->PC() - pcOfJmp - 1);
}

void lua::CodeGen::GetRecursiveConcat(std::vector<LuaParser::ExpContext*>& ctxs, LuaParser::ExpContext* exp)
{
    if (exp->getAltNumber() == size_t(LuaRuleContext::Exp::Concatenation))
    {
        auto concat = static_cast<LuaParser::ConcatenationContext*>(exp);
        auto exps = concat->exp();
        GetRecursiveConcat(ctxs, exps.front());
        GetRecursiveConcat(ctxs, exps.back());
    }
    else
    {
        ctxs.emplace_back(exp);
    }
}

std::any lua::CodeGen::visitExponentiation(LuaParser::ExponentiationContext* ctx)
{
    auto a = _a;
    auto oldRegs = fi->usedRegs;
    auto [b, c] = ExpToOpArg(ctx->exp(), Arg::RK);
    fi->EmitABC(Op::POW, a, b, c);
    fi->usedRegs = oldRegs;
    return std::any();
}

std::any lua::CodeGen::visitUnary(LuaParser::UnaryContext* ctx)
{
    auto a = _a;
    auto oldRegs = fi->usedRegs;
    auto [b, _] = ExpToOpArg(ctx->exp(), Arg::REG);
    Op op;
    switch (ctx->uop()->getAltNumber())
    {
    case 1:
        op = Op::NOT;
        break;
    case 2:
        op = Op::LEN;
        break;
    case 3:
        op = Op::UNM;
        break;
    default:
        op = Op::BNOT;
        break;
    }
    fi->EmitABC(op, a, b, 0);
    fi->usedRegs = oldRegs;
    return std::any();
}

std::any lua::CodeGen::visitMultidiv(LuaParser::MultidivContext* ctx)
{
    auto a = _a;
    auto oldRegs = fi->usedRegs;
    auto [b, c] = ExpToOpArg(ctx->exp(), Arg::RK);
    Op op;
    switch (ctx->bop1()->getAltNumber())
    {
    case 1:
        op = Op::MUL;
        break;
    case 2:
        op = Op::DIV;
        break;
    case 3:
        op = Op::MOD;
        break;
    default:
        op = Op::IDIV;
        break;
    }
    fi->EmitABC(op, a, b, c);
    fi->usedRegs = oldRegs;
    return std::any();
}

std::any lua::CodeGen::visitAddsub(LuaParser::AddsubContext* ctx)
{
    auto a = _a;
    auto oldRegs = fi->usedRegs;
    auto [b, c] = ExpToOpArg(ctx->exp(), Arg::RK);
    Op op;
    switch (ctx->bop2()->getAltNumber())
    {
    case 1:
        op = Op::ADD;
        break;
    default:
        op = Op::SUB;
        break;
    }
    fi->EmitABC(op, a, b, c);
    fi->usedRegs = oldRegs;
    return std::any();
}

std::any lua::CodeGen::visitConcatenation(LuaParser::ConcatenationContext* ctx)
{
    auto a = _a;
    std::vector<LuaParser::ExpContext*> exps;
    GetRecursiveConcat(exps, ctx);
    for (auto&& exp : exps)
    {
        VisitWithPara(fi->AllocReg(), 1, exp);
    }

    auto c = fi->usedRegs - 1;
    auto b = c - (slot_type)exps.size() + 1;
    fi->FreeRegs(c - b + 1);
    fi->EmitConcat(a, b, c);
    return std::any();
}

std::any lua::CodeGen::visitRelation(LuaParser::RelationContext* ctx)
{
    auto a = _a;
    auto oldRegs = fi->usedRegs;
    auto [b, c] = ExpToOpArg(ctx->exp(), Arg::RK);
    Op op;
    switch (ctx->bop3()->getAltNumber())
    {
    case 1:
        fi->EmitABC(Op::LT, 1, b, c);
        break;
    case 2:
        fi->EmitABC(Op::LT, 1, c, b);
        break;
    case 3:
        fi->EmitABC(Op::LE, 1, b, c);
        break;
    case 4:
        fi->EmitABC(Op::LE, 1, c, b);
        break;
    case 5:
        fi->EmitABC(Op::EQ, 0, b, c);
        break;
    default:
        fi->EmitABC(Op::EQ, 1, b, c);
        break;
    }
    fi->EmitJmp(0, 1);
    fi->EmitLoadBool(a, 0, 1);
    fi->EmitLoadBool(a, 1, 0);
    fi->usedRegs = oldRegs;
    return std::any();
}

std::any lua::CodeGen::visitAnd(LuaParser::AndContext* ctx)
{
    VisitLogical<LuaParser::AndContext>(ctx);
    return std::any();
}

std::any lua::CodeGen::visitOr(LuaParser::OrContext* ctx)
{
    VisitLogical<LuaParser::OrContext>(ctx);
    return std::any();
}

std::any lua::CodeGen::visitBitwise(LuaParser::BitwiseContext* ctx)
{
    auto a = _a;
    auto oldRegs = fi->usedRegs;
    auto [b, c] = ExpToOpArg(ctx->exp(), Arg::RK);
    Op op;
    switch (ctx->bop4()->getAltNumber())
    {
    case 1:
        op = Op::BAND;
        break;
    case 2:
        op = Op::BOR;
        break;
    case 3:
        op = Op::BXOR;
        break;
    case 4:
        op = Op::SHL;
        break;
    default:
        op = Op::SHR;
        break;
    }
    fi->EmitABC(op, a, b, c);
    fi->usedRegs = oldRegs;
    return std::any();
}
