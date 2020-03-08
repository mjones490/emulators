#ifndef __6502_shell_h
#define __6502_shell_h
#include "shell.h"
void cpu_shell_load_commands();
int disassemble(int argc, char **argv);
int registers(int argc, char **argv);
int step(int argc, char **argv);
int go(int argc, char **argv);
int assert_interrupt(int argc, char **argv);
void show_registers();
#endif // __6502_shell_commands_h
