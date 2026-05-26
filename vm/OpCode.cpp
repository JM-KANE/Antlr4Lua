#include "OpCode.h"
#include "Instruction.h"
#include "State.h"
#include "Operator.h"

using namespace lua;

namespace lua
{

int32_t Fb2int(int32_t x)
{
    if (x < 8)
    {
        return x;
    }
    else
    {
        return ((x & 7) + 8) << uint32_t((x >> 3) - 1);
    }
}

// R(A) := R(B)
void move(Instruction i, State* ls)
{
    auto [a, b, _] = i.ABC();
    ls->Copy(a + 1, b + 1);
}
// R(A) := Kst(Bx)
void loadK(Instruction i, State* ls)
{
    auto [a, bx] = i.ABx();
    a += 1;
    ls->GetConst(bx);
    ls->Replace(a);
}
// R(A) := Kst(extra arg)
void loadKx(Instruction i, State* ls)
{
    auto [a, _] = i.ABx();
    a += 1;
    auto ax = Instruction(ls->Fetch()).Ax();
    ls->GetConst(ax);
    ls->Replace(a);
}
// R(A) := (bool)B; if (C) pc++
void loadBool(Instruction i, State* ls)
{
    auto [a, b, c] = i.ABC();
    ++a;
    ls->PushBoolean(b);
    ls->Replace(a);
    if (c)
        ls->AddPC(1);
}
// R(A), R(A+1), ..., R(A+B) := nil
void loadNil(Instruction i, State* ls)
{
    auto [a, b, _] = i.ABC();
    ++a;
    ls->PushNil();
    for (size_t i = a; i <= a + b; i++)
    {
        ls->Copy(-1, i);
    }
    ls->Pop(1);
}
// R(A) := UpValue[B]
void getUpval(Instruction i, State* ls)
{
    auto [a, b, _] = i.ABC();
    ls->Copy(cv::REGISTRYINDEX - (b + 1), a + 1);
}
// R(A) := UpValue[B][RK(C)]
void getTabUp(Instruction i, State* ls)
{
    auto [a, b, c] = i.ABC();
    ++a;
    ++b;
    ls->GetRK(c);
    ls->GetTable(cv::REGISTRYINDEX - b);
    ls->Replace(a);
}
// R(A) := R(B)[RK(C)]
void getTable(Instruction i, State* ls)
{
    auto [a, b, c] = i.ABC();
    ++a;
    ++b;
    ls->GetRK(c);
    ls->GetTable(b);
    ls->Replace(a);
}
// UpValue[A][RK(B)] := RK(C)
void setTabUp(Instruction i, State* ls)
{
    auto [a, b, c] = i.ABC();
    ++a;
    ls->GetRK(b);
    ls->GetRK(c);
    ls->SetTable(cv::REGISTRYINDEX - a);
}
// UpValue[B] := R(A)
void setUpval(Instruction i, State* ls)
{
    auto [a, b, _] = i.ABC();
    ls->Copy(a + 1, cv::REGISTRYINDEX - (b + 1));
}
// R(A)[RK(B)] := RK(C)
void setTable(Instruction i, State* ls)
{
    auto [a, b, c] = i.ABC();
    ++a;
    ls->GetRK(b);
    ls->GetRK(c);
    ls->SetTable(a);
}
// R(A) := {} (size = B,C)
void newTable(Instruction i, State* ls)
{
    auto [a, b, c] = i.ABC();
    ++a;
    ls->CreateTable(Fb2int(b), Fb2int(c));
    ls->Replace(a);
}
// R(A+1) := R(B); R(A) := R(B)[RK(C)]
void self(Instruction i, State* ls)
{
    auto [a, b, c] = i.ABC();
    ++a;
    ++b;
    ls->Copy(b, a + 1);
    ls->GetRK(c);
    ls->GetTable(b);
    ls->Replace(a);
}

namespace
{
template <typename T>
void BinaryArith(Instruction i, State* ls)
{
    auto [a, b, c] = i.ABC();
    ++a;
    ls->GetRK(b);
    ls->GetRK(c);
    ls->Arith<T>();
    ls->Replace(a);
}
template <typename T>
void UnaryArith(Instruction i, State* ls)
{
    auto [a, b, _] = i.ABC();
    ++a;
    ls->PushValue(b + 1);
    ls->Arith<T>();
    ls->Replace(a);
}
}  // namespace

constexpr auto add = BinaryArith<op::Add>;
constexpr auto sub = BinaryArith<op::Sub>;
constexpr auto mul = BinaryArith<op::Mul>;
constexpr auto mod = BinaryArith<op::Mod>;
constexpr auto pow = BinaryArith<op::Pow>;
constexpr auto div = BinaryArith<op::Div>;
constexpr auto idiv = BinaryArith<op::IDiv>;
constexpr auto band = BinaryArith<op::BAnd>;
constexpr auto bor = BinaryArith<op::BOr>;
constexpr auto bxor = BinaryArith<op::BXor>;
constexpr auto shl = BinaryArith<op::ShL>;
constexpr auto shr = BinaryArith<op::ShR>;
constexpr auto unm = UnaryArith<op::UnM>;
constexpr auto bnot = UnaryArith<op::BNot>;

void _not(Instruction i, State* ls)
{
    auto [a, b, _] = i.ABC();
    ++a;
    ++b;
    ls->PushBoolean(!ls->ToBoolean(b));
    ls->Replace(a);
}

void length(Instruction i, State* ls)
{
    auto [a, b, _] = i.ABC();
    ++a;
    ++b;
    ls->Len(b);
    ls->Replace(a);
}

void concat(Instruction i, State* ls)
{
    auto [a, b, c] = i.ABC();
    ++a;
    ++b;
    ++c;
    auto n = c - b + 1;
    ls->CheckStack(n);
    for (auto i = b; i <= c; i++)
    {
        ls->PushValue(i);
    }
    ls->Concat(n);
    ls->Replace(a);
}

void jmp(Instruction i, State* ls)
{
    auto [a, sBx] = i.AsBx();

    ls->AddPC(sBx);
    if (a != 0)
    {
        ls->CloseUpvalues(a);
    }
}

void eq(Instruction i, State* ls)
{
}

void lt(Instruction i, State* ls)
{
}

void le(Instruction i, State* ls)
{
}

void test(Instruction i, State* ls)
{
}

void testSet(Instruction i, State* ls)
{
}

void call(Instruction i, State* ls)
{
}

void tailCall(Instruction i, State* ls)
{
}

void _return(Instruction i, State* ls)
{
}

void forLoop(Instruction i, State* ls)
{
}

void forPrep(Instruction i, State* ls)
{
}

void tForCall(Instruction i, State* ls)
{
}

void tForLoop(Instruction i, State* ls)
{
}

void setList(Instruction i, State* ls)
{
}

void closure(Instruction i, State* ls)
{
}

void vararg(Instruction i, State* ls)
{
}

}  // namespace lua

const std::array<OpCode, OpCode::ISA>& lua::OpCode::Get()
{
    // constexpr auto op1 = {1 << 1, OpArg::::R, OpArg::::N, Inst::ABC, "MOVE    ", move};

    static constexpr std::array<OpCode, ISA> opcodes{OpCode{1 << 1, OpArg::R, OpArg::N, Inst::ABC, "MOVE    ", move},
                                                     {1 << 1, OpArg::K, OpArg::N, Inst::ABx, "LOADK   ", loadK},
                                                     {1 << 1, OpArg::N, OpArg::N, Inst::ABx, "LOADKX  ", loadKx},
                                                     {1 << 1, OpArg::U, OpArg::U, Inst::ABC, "LOADBOOL", loadBool},
                                                     {1 << 1, OpArg::U, OpArg::N, Inst::ABC, "LOADNIL ", loadNil},
                                                     {1 << 1, OpArg::U, OpArg::N, Inst::ABC, "GETUPVAL", getUpval},
                                                     {1 << 1, OpArg::U, OpArg::K, Inst::ABC, "GETTABUP", getTabUp},
                                                     {1 << 1, OpArg::R, OpArg::K, Inst::ABC, "GETTABLE", getTable},
                                                     {0, OpArg::K, OpArg::K, Inst::ABC, "SETTABUP", setTabUp},
                                                     {0, OpArg::U, OpArg::N, Inst::ABC, "SETUPVAL", setUpval},
                                                     {0, OpArg::K, OpArg::K, Inst::ABC, "SETTABLE", setTable},
                                                     {1 << 1, OpArg::U, OpArg::U, Inst::ABC, "NEWTABLE", newTable},
                                                     {1 << 1, OpArg::R, OpArg::K, Inst::ABC, "SELF    ", self},
                                                     {1 << 1, OpArg::K, OpArg::K, Inst::ABC, "ADD     ", add},
                                                     {1 << 1, OpArg::K, OpArg::K, Inst::ABC, "SUB     ", sub},
                                                     {1 << 1, OpArg::K, OpArg::K, Inst::ABC, "MUL     ", mul},
                                                     {1 << 1, OpArg::K, OpArg::K, Inst::ABC, "MOD     ", mod},
                                                     {1 << 1, OpArg::K, OpArg::K, Inst::ABC, "POW     ", pow},
                                                     {1 << 1, OpArg::K, OpArg::K, Inst::ABC, "DIV     ", div},
                                                     {1 << 1, OpArg::K, OpArg::K, Inst::ABC, "IDIV    ", idiv},
                                                     {1 << 1, OpArg::K, OpArg::K, Inst::ABC, "BAND    ", band},
                                                     {1 << 1, OpArg::K, OpArg::K, Inst::ABC, "BOR     ", bor},
                                                     {1 << 1, OpArg::K, OpArg::K, Inst::ABC, "BXOR    ", bxor},
                                                     {1 << 1, OpArg::K, OpArg::K, Inst::ABC, "SHL     ", shl},
                                                     {1 << 1, OpArg::K, OpArg::K, Inst::ABC, "SHR     ", shr},
                                                     {1 << 1, OpArg::R, OpArg::N, Inst::ABC, "UNM     ", unm},
                                                     {1 << 1, OpArg::R, OpArg::N, Inst::ABC, "BNOT    ", bnot},
                                                     {1 << 1, OpArg::R, OpArg::N, Inst::ABC, "NOT     ", _not},
                                                     {1 << 1, OpArg::R, OpArg::N, Inst::ABC, "LEN     ", length},
                                                     {1 << 1, OpArg::R, OpArg::R, Inst::ABC, "CONCAT  ", concat},
                                                     {0, OpArg::R, OpArg::N, Inst::AsBx, "JMP     ", jmp},
                                                     {1, OpArg::K, OpArg::K, Inst::ABC, "EQ      ", eq},
                                                     {1, OpArg::K, OpArg::K, Inst::ABC, "LT      ", lt},
                                                     {1, OpArg::K, OpArg::K, Inst::ABC, "LE      ", le},
                                                     {1, OpArg::N, OpArg::U, Inst::ABC, "TEST    ", test},
                                                     {1 | 1 << 1, OpArg::R, OpArg::U, Inst::ABC, "TESTSET ", testSet},
                                                     {1 << 1, OpArg::U, OpArg::U, Inst::ABC, "CALL    ", call},
                                                     {1 << 1, OpArg::U, OpArg::U, Inst::ABC, "TAILCALL", tailCall},
                                                     {0, OpArg::U, OpArg::N, Inst::ABC, "RETURN  ", _return},
                                                     {1 << 1, OpArg::R, OpArg::N, Inst::AsBx, "FORLOOP ", forLoop},
                                                     {1 << 1, OpArg::R, OpArg::N, Inst::AsBx, "FORPREP ", forPrep},
                                                     {0, OpArg::N, OpArg::U, Inst::ABC, "TFORCALL", tForCall},
                                                     {1 << 1, OpArg::R, OpArg::N, Inst::AsBx, "TFORLOOP", tForLoop},
                                                     {0, OpArg::U, OpArg::U, Inst::ABC, "SETLIST ", setList},
                                                     {1 << 1, OpArg::U, OpArg::N, Inst::ABx, "CLOSURE ", closure},
                                                     {1 << 1, OpArg::U, OpArg::N, Inst::ABC, "VARARG  ", vararg},
                                                     {0, OpArg::U, OpArg::U, Inst::Ax, "EXTRAARG", nullptr}};
    return opcodes;
}
