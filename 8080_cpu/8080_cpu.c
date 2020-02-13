#include "8080_cpu.h"
#include "cpu_types.h"

void cpu_set_reg_A(BYTE value) 
{
    regs.b.A = value;
}

BYTE cpu_get_reg_A()
{
    return regs.b.A;
}

void cpu_set_reg_B(BYTE value) 
{
    regs.b.B = value;
}

BYTE cpu_get_reg_B()
{
    return regs.b.B;
}

void cpu_set_reg_C(BYTE value) 
{
    regs.b.C = value;
}

BYTE cpu_get_reg_C()
{
    return regs.b.C;
}

void cpu_set_reg_BC(WORD value) 
{
    regs.w.BC = value;
}

WORD cpu_get_reg_BC()
{
    return regs.w.BC;
}

void cpu_set_reg_D(BYTE value) 
{
    regs.b.D = value;
}

BYTE cpu_get_reg_D()
{
    return regs.b.D;
}

void cpu_set_reg_E(BYTE value) 
{
    regs.b.E = value;
}

BYTE cpu_get_reg_E()
{
    return regs.b.E;
}

void cpu_set_reg_DE(WORD value) 
{
    regs.w.DE = value;
}

WORD cpu_get_reg_DE()
{
    return regs.w.DE;
}

void cpu_set_reg_H(BYTE value) 
{
    regs.b.H = value;
}

BYTE cpu_get_reg_H()
{
    return regs.b.H;
}

void cpu_set_reg_L(BYTE value) 
{
    regs.b.L = value;
}

BYTE cpu_get_reg_L()
{
    return regs.b.L;
}

void cpu_set_reg_HL(WORD value) 
{
    regs.w.HL = value;
}

WORD cpu_get_reg_HL()
{
    return regs.w.HL;
}

void cpu_set_reg_SP(WORD value) 
{
    regs.w.SP = value;
}

WORD cpu_get_reg_SP()
{
    return regs.w.SP;
}

void cpu_set_reg_PC(WORD value) 
{
    regs.w.PC = value;
}

WORD cpu_get_reg_PC()
{
    return regs.w.PC;
}

