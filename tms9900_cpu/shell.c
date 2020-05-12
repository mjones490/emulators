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
#include "cpu_types.h"
#include "format.h"
#include "instruction.h"
#include "cru.h"

struct regs_t regs;
struct cpu_state_t cpu_state;

const int ram_size = 8192;

BYTE *ram;

BYTE my_accessor(WORD address, bool read, BYTE value)
{
    if (address < ram_size) {
        if (read)
            value = ram[address];
        else
            ram[address] = value;
    } else {
        value = 0x00;
    }
    
    return value;
}

char *ws_reg_name[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "ra", "rb", "rc", "rd", "re", "rf"
};

static inline int display_format(char* buf, BYTE t, BYTE r)
{
    switch(t) {
    case 0:
        return sprintf(buf, "%s", ws_reg_name[r]);

    case 1:
        return sprintf(buf, "*%s", ws_reg_name[r]);
    
    case 3:
        return sprintf(buf, "*%s+", ws_reg_name[r]);
    
    case 2:
        if (r > 0)
            return sprintf(buf, "@>%04x(%s)", get_next_word(), ws_reg_name[r]);
        
        return sprintf(buf, "@>%04x", get_next_word());

    default:
        return 0;
    };
}

WORD disassemble_instruction(WORD address)
{
    WORD old_pc = regs.pc;
    WORD code;
    struct instruction_t *instruction;
    WORD tmp;
    char mnemonic[5];
    char operands[16];
    char offset;
    int i = 0;

    strcpy(mnemonic, "nop ");
    memset(operands, 0, 16);

    regs.pc = address;
    code = get_next_word();
    instruction = decode_instruction(code);

    if (instruction != NULL) {
        while (instruction->mnemonic[i])
            mnemonic[i++] = tolower(instruction->mnemonic[i]);
        while (i < 4)
            mnemonic[i++] = ' ';
        
        switch (instruction->format) {
        case FMT_I:
            i = display_format(operands, (code >> 4) & 0x03, code & 0x0f);
            i += sprintf(operands + i, ",");
            display_format(operands + i, (code >> 10) & 0x03, (code >> 6) & 0x0f);
            break;

        case FMT_II:
            offset = (code & 0xff);
            sprintf(operands, ">%04x", regs.pc + (offset * 2));
            break;

        case FMT_III:
        case FMT_IX:
            i = display_format(operands, (code >> 4) & 0x03, code & 0x0f);
            sprintf(operands + i, ",%s", ws_reg_name[(code >> 6) & 0x0f]);
            break;

        case FMT_IV:            
            i = display_format(operands, (code >> 4) & 0x03, code & 0x0f);
            sprintf(operands + i, ",>%x", (code >> 6 & 0x0f));
            break;

        case FMT_V:
            sprintf(operands, "%s,>%x", ws_reg_name[(code & 0x0f)], (code >> 4) & 0x0f);
            break;

        case FMT_VI:
            display_format(operands, (code >> 4) & 0x03, code & 0x0f);
            break;
        
        case FMT_VIII:
            sprintf(operands, "%s,>%04x", ws_reg_name[code & 0x0f], get_next_word());
            break;

        case FMT_X:
            sprintf(operands, ">%02x", code & 0xff);
            break;
        
        case FMT_XI:
            sprintf(operands, ">%04x", get_next_word());
            break;

        case FMT_XII:
            sprintf(operands, "%s", ws_reg_name[code & 0x0f]);
        }
    }

    printf("%04x: ", address);
    for (i = 0; i < 6; i += 2) {
        if (regs.pc > (address + i))
            printf("%04x ", get_word(address + i));
        else
            printf("     ");
    }
    printf("%s %s\n", mnemonic, operands);

    tmp = regs.pc; 
    regs.pc = old_pc;
    return tmp;
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
        address = disassemble_instruction(address);
    } while (address < end);

    return 0;
}

void show_registers()
{
    int i;
    printf("pc: %04x wp: %04x st: %04x\n", regs.pc, regs.wp, regs.st);

    for (i = 0; i < 16; i++) {
        printf("%s = %04x  ", ws_reg_name[i], get_register_value(i));
        if (i == 7 || i == 15)
            printf("\n");
    }
}

int registers(int argc, char **argv)
{
    int tmp;
    char *reg_name;
    int arg_no = 1;
    int i;

    while (argc > arg_no) {
        reg_name = argv[arg_no++];
        if (1 == sscanf(argv[arg_no++], "%4x", &tmp)) {
            if (0 == strcmp("pc", reg_name))
                regs.pc = (WORD) tmp;
            else if (0 == strcmp("wp", reg_name))
                regs.wp = (WORD) tmp;
            else if (0 == strcmp("st", reg_name))
                regs.st = (WORD) tmp;
            else {
                for (i = 0; i < 16; i++) {
                    if (0 == strcmp(ws_reg_name[i], reg_name)) {
                        set_register_value(i, (WORD) tmp);
                    }
                }
            }
        }
    }

    show_registers();
    disassemble_instruction(regs.pc);

    return 0;
}

int step(int argc, char **argv)
{
    unsigned int tmp;
    
    if (argc > 1) {
        if (1 == sscanf(argv[1], "%4x", &tmp))
            regs.pc = (WORD) tmp;
    }

    execute_instruction();
    show_registers();
    disassemble_instruction(regs.pc);
    return 0;
}

void cnt_inst()
{
    unsigned int code;
    int counter = 0;
    for (code = 0; code <= 0xffff; code++) {
        if (decode_instruction(code & 0xffff) != NULL)
            counter++;
        printf("\rChecking %04x.  %04x valid.", code, counter);

    }

    printf("\n");
}

//            24
//        20   |  28   
//         |   |   |
//  0000000011111111  h   
//  0000000111110000  s
//          11110000  mask
//

BYTE hv1, hv2;

cru_accessor_t prev_cru_accessor = NULL;
cru_accessor_t prev_cru_accessor2 = NULL;
WORD my_cru_accessor(WORD soft_address, bool read, WORD value, BYTE size)
{
    struct hw_val_t hw_value;

    if (!read) {
        hw_value = to_hardware(soft_address, value, size, 0x28); 
        hv2 = (hv2 & ~hw_value.bitmask) | hw_value.value;
    } else {
        value |= to_software(0x28, hv2, size, soft_address);
    }

    return value;
}

WORD my_cru_accessor2(WORD soft_address, bool read, WORD value, BYTE size)
{
    struct hw_val_t hw_value;
    
    if (!read) {
        hw_value = to_hardware(soft_address, value, size, 0x20); 
        hv1 = (hv1 & ~hw_value.bitmask) | hw_value.value;
    } else {
        value |= to_software(0x20, hv1, size, soft_address);
    }

    return value;
}

int input(int argc, char **argv)
{
    unsigned int tmp;
    WORD soft_address;
    WORD value;
    int num_bits;

    if (argc != 3)
        return 0;

    if (1 != sscanf(argv[1], "%x", &tmp))
        return 0;

    soft_address = tmp;

    if (1 != sscanf(argv[2], "%d", &num_bits))
        return 0;

    value = cru_in(soft_address, num_bits);

    printf("value = %04x\n", value);
    return 0;
    
}

int output(int argc, char **argv)
{
    unsigned int tmp;
    WORD soft_address;
    WORD value;
    int num_bits;

    if (argc != 4)
        return 0;

    if (1 != sscanf(argv[1], "%x", &tmp))
        return 0;

    soft_address = tmp;

    if (1 != sscanf(argv[2], "%x", &tmp))
        return 0;

    value = tmp;

    if (1 != sscanf(argv[3], "%d", &num_bits))
        return 0;

    value = cru_out(soft_address, value, num_bits);

    printf("value = %04x\n", value);
    return 0;
}

int main(int argc, char **argv)
{
    cpu_state.bus = my_accessor;

    set_cru_accessor(my_cru_accessor2);
    set_cru_accessor(my_cru_accessor);

    init_instruction();
    cnt_inst();

    ram = malloc(ram_size);
    shell_set_accessor(my_accessor);
    shell_initialize("tms9900 shell");
   
    shell_set_extended_commands(SEC_LOAD | SEC_SAVE | SEC_CLEAR);
    shell_add_command("disassemble", "Disassemble a word.", disassemble, true);
    shell_add_command("registers", "View/change regisgers.", registers, false);
    shell_add_command("step", "Execute single instruction.", step, true);
    shell_add_command("output", "", output, false);
    shell_add_command("input", "", input, false);

    shell_load_history("./.shell_history");
    shell_loop();
    shell_save_history("./.shell_history");
    shell_finalize();

    free(ram);
    finalize_instruction();
    printf("\n");
}

