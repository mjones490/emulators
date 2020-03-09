#ifndef __8080_shell_h
#define __8080_shell_h
void cpu_shell_load_commands();
void show_registers();
void set_register(char *reg_name, WORD value);
WORD disassemble_inst(WORD address);
#endif
