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
#include <ctype.h>
#include "shell.h"
#include "8080_cpu.h"
#include "cpu_types.h"
#include "instructions.h"

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
   Disassemble command
 **********************************************/
char *reg8[] = { "b", "c", "d", "e", "h", "l", "m", "a" };
char *reg16[] = { "bc", "de", "hl", "sp" };

static WORD disassemble_inst(WORD address)
{
    BYTE code;
    char mnemonic[5];
    struct instruction_t *inst;
    int i = 0;

    printf("%04x: ", address);
    code = get_byte(address++);
    printf("%02x ", code);
    inst = instruction_map[code]; 
    
    if (inst == NULL) 
        inst = instruction_map[0];

    do {
        mnemonic[i] = tolower(inst->mnemonic[i]);
        if (mnemonic[i] == 0)
            mnemonic[i] = ' ';
        mnemonic[++i] = 0;
    } while (i < 4);

    if(strcmp(inst->args, "IMP") == 0) {
        printf("       %s\n", mnemonic);
    } else if (strcmp(inst->args, "DDDSSS") == 0) {
        printf("       %s %s,%s\n", mnemonic, reg8[(code >> 3) & 0x07], reg8[(code & 0x07)]);
    } else if (strcmp(inst->args, "DI") == 0) {
        printf("%02x     %s %s,%02xh\n", get_byte(address), mnemonic, reg8[(code >> 3) & 0x07], get_byte(address));
        address++;
    } else if (strcmp(inst->args, "DDD") == 0) {
        printf("       %s %s\n", mnemonic, reg8[(code >> 0x03) & 0x07]);
    } else if (strcmp(inst->args, "RPI") == 0) {
        printf("%02x %02x  %s %s,%04xh\n", get_byte(address), get_byte(address + 1), mnemonic, 
            reg16[(code >> 4) & 0x03], get_word(address));
        address += 2;
    } else if (strncmp(inst->args, "RP", 2) == 0) {
        printf("       %s %s\n", mnemonic, reg16[(code >> 4) & 0x03]); 
    } else if (strcmp(inst->args, "ADDR") == 0) {
        printf("%02x %02x  %s %04xh\n", get_byte(address), get_byte(address + 1), mnemonic, get_word(address));
        address += 2;
    } else {
        printf("\n");
    }
    return address;

}

static int disassemble(int argc, char **argv)
{
    int matched;
    int start;
    int end;
    static WORD address;
    static WORD extend = 1;

    if (1 < argc) { 
        matched = sscanf(argv[1], "%4x-%4x", &start, &end);
        if (matched) {
            address = start;
            if (matched == 1) { 
                end = address;
                extend = 1;
            } else {
                extend = end - address;
            }
        }
    } else {
        end = address + extend;
    }

    do {
        address = disassemble_inst(address);
    } while (address < end);

    return 0;
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
    disassemble_inst(cpu_get_reg_PC());
}

static int registers(int argc, char **argv)
{
    int tmp;
    WORD value = 0;
    char *reg_name;
    int arg_no = 1;

    while (argc > arg_no) {
        reg_name = argv[arg_no++];
        if (1 == sscanf(argv[arg_no++], "%4x", &tmp))
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

static int step()
{
    cpu_execute_instruction();
    show_registers();
    return 0;
}

static void load_image(char* file_name, WORD address)
{
    int fd;
    struct stat sb;
    int bytes_read;

    // Open file
    fd = open(file_name, O_RDONLY);
    if (-1 == fd) {
        printf("Error opening file %s.\n", file_name);
        return;
    }

    if (-1 == fstat(fd, &sb)) {
        printf("Could not determine size of file.\n");
        close(fd);
        return;
    }    

    bytes_read = read(fd, &ram[address], sb.st_size);
    printf("%d bytes read.\n", bytes_read);
    close(fd);
    return;
}

static int load(int argc, char **argv)
{
    WORD address;
    char* filename;

    if (3 > argc) {
        printf("load addr filename\n");
        return 0;
    }

    if (1 != sscanf(argv[1], "%4x", &address)) {
        printf("Invalid load address.\n");
    }

    filename = argv[2];
    load_image(filename, address);

    return 0;
}

void cpu_shell_load_commands()
{
    shell_add_command("registers", "View/change 8080 registers.", registers, false);
    shell_add_command("step", "Execute single instruction.", step, true);
    shell_add_command("disassemble", "Disassemble code.", disassemble, true);
    shell_add_command("load", "Load a binary file the given address.", load, false);
}

int main(int argc, char **argv)
{
    ram = malloc(ram_size);
    shell_set_accessor(my_accessor);
    shell_initialize("8080 shell");
    cpu_init(my_accessor);
    cpu_shell_load_commands();
    shell_loop();
    shell_finalize();
    printf("\n");
    return 0;
}
