#ifndef __8080_CPU_H
#define __8080_CPU_H
#include <stdio.h>
#include "types.h"

void cpu_set_reg_A(BYTE value); 
BYTE cpu_get_reg_A();
void cpu_set_reg_B(BYTE value); 
BYTE cpu_get_reg_B();
void cpu_set_reg_C(BYTE value); 
BYTE cpu_get_reg_C();
void cpu_set_reg_BC(WORD value); 
WORD cpu_get_reg_BC();
void cpu_set_reg_D(BYTE value); 
BYTE cpu_get_reg_D();
void cpu_set_reg_E(BYTE value); 
BYTE cpu_get_reg_E();
void cpu_set_reg_DE(WORD value); 
WORD cpu_get_reg_DE();
void cpu_set_reg_H(BYTE value); 
BYTE cpu_get_reg_H();
void cpu_set_reg_L(BYTE value); 
BYTE cpu_get_reg_L();
void cpu_set_reg_HL(WORD value); 
WORD cpu_get_reg_HL();
void cpu_set_reg_L(BYTE value); 
BYTE cpu_get_reg_L();
void cpu_set_reg_SP(WORD value); 
WORD cpu_get_reg_SP();
void cpu_set_reg_PC(WORD value); 
WORD cpu_get_reg_PC();

void cpu_init(accessor_t bus);
BYTE cpu_execute_instruction();

#endif // __8080_CPU_H
