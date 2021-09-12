#include "6502_cpu.h"
#include "cpu_types.h"
#include "instructions.h"

void cpu_set_reg_A(BYTE value)
{
    regs.A = value;
}

BYTE cpu_get_reg_A()
{
    return regs.A;
}

void cpu_set_reg_X(BYTE value)
{
    regs.X = value;
}

BYTE cpu_get_reg_X()
{
    return regs.X;
}

void cpu_set_reg_Y(BYTE value)
{
    regs.Y = value;
}

BYTE cpu_get_reg_Y()
{
    return regs.Y;
}

void cpu_set_reg_SP(BYTE value)
{
    regs.SP = value;
}

BYTE cpu_get_reg_SP()
{
    return regs.SP;
}

void cpu_set_reg_PS(BYTE value)
{
    regs.PS = value;
}

BYTE cpu_get_reg_PS()
{
    return regs.PS;
}

void cpu_set_reg_PC(WORD value)
{
    regs.PC = value;
}

WORD cpu_get_reg_PC()
{
    return regs.PC;
}

WORD cpu_get_prev_PC()
{
    return cpu_state.prev_PC;
}

enum ADDRESS_MODE cpu_get_address_mode(BYTE code)
{
    return instruction[code].mode;
}

int cpu_get_instruction_size(BYTE code)
{
    return instruction[code].size;
}

void cpu_set_signal(BYTE signal)
{
    set_signal(signal);
}

void cpu_clear_signal(BYTE signal)
{
    clear_signal(signal);
}

void cpu_toggle_signal(BYTE signal)
{
    toggle_signal(signal);
}

BYTE cpu_get_signal(BYTE signal)
{
    return get_signal(signal);
}

