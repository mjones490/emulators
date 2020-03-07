#ifndef __8080_shell_h
#define __8080_shell_h
   
int disassemble(int argc, char **argv);
int registers(int argc, char **argv);
int step(int argc, char **argv);
int go(int argc, char **argv);
int halt(int argc, char **argv);
int load(int argc, char **argv);
int breakpoint(int argc, char **argv);
void cpu_shell_load_commands();
void show_registers();
#endif
