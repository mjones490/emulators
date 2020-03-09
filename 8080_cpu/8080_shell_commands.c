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

/**********************************************
   Disassemble command
 **********************************************/
char *reg8[] = { "b", "c", "d", "e", "h", "l", "m", "a" };
char *reg16[] = { "b", "d", "h", "sp", "psw" };
char *condition[] = { "nz", "z ", "nc", "c ", "po", "pe", "p ", "m " };

WORD disassemble_inst(WORD address)
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

    mnemonic[2] = ' ';
    mnemonic[3] = ' ';
    mnemonic[4] = 0;
    while (inst->mnemonic[i])
        mnemonic[i++] = tolower(inst->mnemonic[i]);

    switch (inst->inst_type) {
    case IMP:
        printf("       %s\n", mnemonic);
        break;
    case DDDSSS:
        printf("       %s %s,%s\n", mnemonic, reg8[(code >> 3) & 0x07], reg8[(code & 0x07)]);
        break;
    case DDDIMM:
        printf("%02x     %s %s,%02xh\n", get_byte(address), mnemonic, reg8[(code >> 3) & 0x07], get_byte(address));
        address++;
        break;
    case DDD:
        printf("       %s %s\n", mnemonic, reg8[(code >> 0x03) & 0x07]);
        break;
    case RPIMM:
        printf("%02x %02x  %s %s,%04xh\n", get_byte(address), get_byte(address + 1), mnemonic, 
            reg16[(code >> 4) & 0x03], get_word(address));
        address += 2;
        break;
    case RP:
    case RPBD:
        i = (code == 0xf1 || code == 0xf5)? 1 : 0;
        printf("       %s %s\n", mnemonic, reg16[((code >> 4) & 0x03) + i]); 
        break;
    case ADDR:
        printf("%02x %02x  %s %04xh\n", get_byte(address), get_byte(address + 1), mnemonic, get_word(address));
        address += 2;
        break;
    case CCC:
        if (mnemonic[0] == 'r') {
            printf("       r%s\n", condition[(code >> 3) & 0x07]);
        } else {
            printf("%02x %02x  %c%s  %04xh\n", get_byte(address), get_byte(address + 1),  mnemonic[0], 
                condition[(code >> 3) & 0x07], get_word(address));
            address += 2;
        }
        break;
    case IMM:
        printf("%02x     %s %02xh\n", get_byte(address), mnemonic, get_byte(address));
        address++;
        break;
    case SSS:
        printf("       %s %s\n", mnemonic, reg8[code & 0x07]);
        break;
    case NNN:
        printf("       %s %02xh\n", mnemonic, (code >> 3) & 0x07);
        break;
    default:
        printf("\n");
        break;
    }

    return address;

}

int disassemble(int argc, char **argv)
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
char* flag_name[] = { "P  ", "M  ", "NZ ", "Z  ", "", "", "NA ", "A  ", 
    "", "", "PO ", "PE ", "", "", "NC ", "C  " };

void show_registers() 
{
    int i;
    BYTE flags = cpu_get_reg_PSW();

    printf("a = %02x ", cpu_get_reg_A());
    printf("b = %02x ", cpu_get_reg_B());
    printf("c = %02x ", cpu_get_reg_C());
    printf("d = %02x ", cpu_get_reg_D());
    printf("e = %02x ", cpu_get_reg_E());
    printf("hl = %04x ", cpu_get_reg_HL());
    printf("sp = %04x ", cpu_get_reg_SP());
    printf("pc = %04x ", cpu_get_reg_PC());
    printf("psw = %02x  ", flags);
    for (i = 0; i < 8; i++)
        printf("%s", flag_name[(((0x80 >> i) & flags) != 0) + (i << 1)]);
    if (cpu_state.total_clocks > 0) 
        printf("  %d clocks last used", cpu_state.total_clocks);
    printf("\n");
}

void set_register(char *reg_name, WORD value)
{
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

static int registers(int argc, char **argv)
{
    int tmp;
    char *reg_name;
    int arg_no = 1;

    while (argc > arg_no) {
        reg_name = argv[arg_no++];
        if (1 == sscanf(argv[arg_no++], "%4x", &tmp)) 
            set_register(reg_name, (WORD) tmp);
    }
    
    show_registers();
    return 0;
}

static int step(int argc, char **argv)
{
    cpu_execute_instruction();
    show_registers();
    disassemble_inst(cpu_get_reg_PC());
    return 0;
}

static int go(int argc, char **argv)
{
    cpu_set_halted(false);
    //execute_instruction();
    return 0;
}

static int halt(int argc, char **argv)
{
    cpu_set_halted(true);
    return 0;
}

static void load_image(char* file_name, WORD address)
{
    int fd;
    struct stat sb;
    int bytes_read;
    BYTE* buffer;
    int i;

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

    buffer = malloc(sb.st_size);
    printf("Reading %ld bytes...\n", sb.st_size);
    bytes_read = read(fd, buffer, sb.st_size);

    for (i = 0; i < bytes_read; i++)
        put_byte(address + i, *(buffer + i));

    free(buffer);
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

    if (1 != sscanf(argv[1], "%4hx", &address)) {
        printf("Invalid load address.\n");
    }

    filename = argv[2];
    load_image(filename, address);

    return 0;
}

static int breakpoint(int argc, char **argv)
{
    int tmp;

    if (argc == 2 && 1 == sscanf(argv[1], "%4x", &tmp))
        cpu_state.breakpoint = (WORD)tmp;

    printf("Breakpoint set at %04xh.\n", cpu_state.breakpoint);

    return 0;
}

void cpu_shell_load_commands()
{
    shell_add_command("registers", "View/change 8080 registers.", registers, false);
    shell_add_command("step", "Execute single instruction.", step, true);
    shell_add_command("disassemble", "Disassemble code.", disassemble, true);
    shell_add_command("go", "Start program.", go, false);
    shell_add_command("halt", "Halt CPU.", halt, false);
    shell_add_command("load", "Load a binary file the given address.", load, false);
    shell_add_command("breakpoint", "Set or view the PC breakpoint.", breakpoint, false);
}
