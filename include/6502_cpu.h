#ifndef __6502_CPU_H
#define __6502_CPU_H
#include <types.h>
#include "6502_cpu_enums.h"

void cpu_set_reg_A(BYTE value);
BYTE cpu_get_reg_A();
void cpu_set_reg_X(BYTE value);
BYTE cpu_get_reg_X();
void cpu_set_reg_Y(BYTE value);
BYTE cpu_get_reg_Y();
void cpu_set_reg_SP(BYTE value);
BYTE cpu_get_reg_SP();
void cpu_set_reg_PS(BYTE value);
BYTE cpu_get_reg_PS();
void cpu_set_reg_PC(WORD value);
WORD cpu_get_reg_PC();
WORD cpu_get_prev_PC();
enum NMONIC cpu_get_nmonic(BYTE code);
enum ADDRESS_MODE cpu_get_address_mode(BYTE code);
int cpu_get_instruction_size(BYTE code);
BYTE cpu_get_clocks();
BYTE cpu_execute_instruction();
void cpu_init(accessor_t bus);
void cpu_set_signal(BYTE signal);
void cpu_clear_signal(BYTE signal);
void cpu_toggle_signal(BYTE signal);
BYTE cpu_get_signal(BYTE signal);
BYTE cpu_peek(WORD address);
BYTE cpu_poke(WORD address, BYTE value);
#endif
