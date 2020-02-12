#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include "shell.h"

union regs_t {
    struct {
        BYTE PSW;
        BYTE A;
        BYTE C;
        BYTE B;
        BYTE E;
        BYTE D;
        BYTE L;
        BYTE H;
        BYTE SPL;
        BYTE SPH;
        BYTE PCL;
        BYTE PCH;
    } b;
    struct {
        WORD PSWA;
        WORD BC;
        WORD DE;
        WORD HL;
        WORD SP;
        WORD PC;
    } w;
};

union regs_t regs;

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

const int ram_size = 8192;

BYTE *ram;

BYTE my_accessor(WORD address, bool read_byte, BYTE value)
{
    if (address >= ram_size)
        return 0x00;
    if (read_byte)
        value = ram[address];
    else
        ram[address] = value;

    return value;
}

/**********************************************
   Registers command 
 **********************************************/
static void show_registers() 
{
    printf("a = %02x ", cpu_get_reg_A());
    printf("b = %02x ", cpu_get_reg_B());
    printf("c = %02x ", cpu_get_reg_C());
    printf("d = %02x ", cpu_get_reg_D());
    printf("e = %02x ", cpu_get_reg_E());
    printf("hl = %04x ", cpu_get_reg_HL());
    printf("sp = %04x ", cpu_get_reg_SP());
    printf("pc = %04X ", cpu_get_reg_PC());
    
    printf("\n");
}

static int registers(int argc, char **argv)
{
    int tmp;
    WORD value = 0;
    char *reg_name;

    if (argc == 3) {
        reg_name = argv[1];
        if (1 == sscanf(argv[2], "%4x", &tmp))
            value = (WORD) tmp;
        if (0 == strncmp(reg_name, "a", 2))
            cpu_set_reg_A(lo(value));
        else if (0 == strncmp(reg_name, "b", 2))
            cpu_set_reg_B(lo(value));
        else if (0 == strncmp(reg_name, "c", 2))
            cpu_set_reg_C(lo(value));
        else if (0 == strncmp(reg_name, "bc", 3))
            cpu_set_reg_BC(value);
        else if (0 == strncmp(reg_name, "d", 2))
            cpu_set_reg_D(lo(value));
        else if (0 == strncmp(reg_name, "e", 2))
            cpu_set_reg_E(lo(value));
        else if (0 == strncmp(reg_name, "de", 3))
            cpu_set_reg_DE(value);
        else if (0 == strncmp(reg_name, "h", 2))
            cpu_set_reg_H(lo(value));
        else if (0 == strncmp(reg_name, "l", 2))
            cpu_set_reg_L(lo(value));
        else if (0 == strncmp(reg_name, "hl", 3))
            cpu_set_reg_HL(value);
        else if (0 == strncmp(reg_name, "sp", 3))
            cpu_set_reg_SP(value);
        else if (0 == strncmp(reg_name, "pc", 3))
            cpu_set_reg_PC(value);
    }
    
    show_registers();
    return 0;
}

void cpu_shell_load_commands()
{
    shell_add_command("registers", "View/change 8080 registers.", registers, false);
}

int main(int argc, char **argv)
{
    ram = malloc(ram_size);
    shell_set_accessor(my_accessor);
    shell_initialize("8080 shell");
    cpu_shell_load_commands();
    shell_loop();
    shell_finalize();
    printf("\n");
    return 0;
}
