#ifndef __6502_shell_h
#define __6502_shell_h
#include "shell.h"
void cpu_shell_load_commands();
void show_registers();
void set_register(char* reg_name, WORD value);
void disasm_instr(WORD *address);
#endif // __6502_shell_commands_h
