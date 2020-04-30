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

struct instruction_group_t {
    int shift_cnt;
    WORD mask;
};

struct instruction_group_t instruction_group[] = {
    { 12, 0x000f }, // 1111000000000000 -> 0000000000001111 0 4 bits
    { 10, 0x0003 }, // 1111220000000000 -> 0000000000000022 1 6 bits
    { 8,  0x0003 }, // 1111223300000000 -> 0000000000000033 2 8 bits
    { 6,  0x0003 }, // 1111223344000000 -> 0000000000000044 3 10 bits
    { 5,  0x0001 }, // 1111223344500000 -> 0000000000000005 4 11 bits  
};

enum entry_type {
    ET_INSTRUCTION,
    ET_LIST
};
struct instruction_t {
    enum entry_type type;    
    char            *mnemonic;
    char            group;
    WORD            code;
};

#define GRP_0 0
#define GRP_1 1
#define GRP_2 2
#define GRP_3 3
#define GRP_4 4

#define INSTRUCTION_DEF(mnemonic, code, group) \
    { ET_INSTRUCTION, #mnemonic, group, code }

struct instruction_t instruction[] = {
    INSTRUCTION_DEF( A,     0xa000, GRP_0),
    INSTRUCTION_DEF( AB,    0xb000, GRP_0), 
    INSTRUCTION_DEF( BR,    0x4400, GRP_3),
    INSTRUCTION_DEF( JMP,   0x1000, GRP_2),
    INSTRUCTION_DEF( LI,    0x0200, GRP_4),
    INSTRUCTION_DEF( SBO,   0x1d00, GRP_2),
    INSTRUCTION_DEF( XOR,   0x2400, GRP_1)
};

struct instruction_list_t {
    enum entry_type type;
    struct instruction_list_t *list[1];
};

struct instruction_list_t* root;

static inline WORD get_index(int group, WORD code)
{
    return (code >> instruction_group[group].shift_cnt) & instruction_group[group].mask;
}

void init_instruction()
{
    size_t instruction_count = sizeof(instruction) / sizeof(struct instruction_t);
    
    size_t list_size = sizeof(enum entry_type) +  (sizeof(struct instruction_list_t *) * instruction_group[0].mask);
    root = malloc(list_size);
    memset(root, 0, list_size);
    root->type = ET_LIST;
 
    int group;
    int i;
    int inst_ndx;

    struct instruction_list_t *branch;
    struct instruction_list_t *new_branch;

    size_t total_size = list_size;

    for (i = 0; i < instruction_count; i++) {
        group = 0;
        branch = root;
        while (true) {
            inst_ndx = get_index(group, instruction[i].code);
            if (group == instruction[i].group) {
                branch->list[inst_ndx] = (void *) &instruction[i];
                break;
            } else {
                group++;
                if (branch->list[inst_ndx] == NULL) {
                    size_t list_size = sizeof(enum entry_type) +  
                        (sizeof(struct instruction_list_t *) * instruction_group[group].mask);
                    new_branch = malloc(list_size);
                    memset(new_branch, 0, list_size);
                    new_branch->type = ET_LIST;
                    branch->list[inst_ndx] = new_branch;
                    total_size += list_size;
                }
                branch = branch->list[inst_ndx];
            }
        }
    }

    printf("Total instruction list size = %zu bytes.\n", total_size);
}

struct instruction_t *decode_instruction(WORD code)
{
    struct instruction_list_t *branch = root;
    int inst_ndx;
    int group = 0;

    while (true) {
        inst_ndx = get_index(group, code);
        if (branch->list[inst_ndx] ==  NULL)
            return 0;
        if (branch->list[inst_ndx]->type == ET_INSTRUCTION)
            return (struct instruction_t *) branch->list[inst_ndx];
        
        branch = branch->list[inst_ndx];
        group++;
    }
}

int disassemble(int argc, char **argv)
{
    unsigned int tmp;
    struct instruction_t *instruction;
    WORD op;
    if (argc != 2) {
        printf("disassemble word, where word is word to disassemble.\n");
        return 0;
    }

    sscanf(argv[1], "%04x", &tmp);
    op = tmp;
    instruction = decode_instruction(op);
    if (instruction != 0)
        printf("Intruction = %s\n", instruction->mnemonic);

    return 0;
}

char *reg_name[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "ra", "rb", "rc", "rd", "re", "rf"
};

void show_registers()
{
    int i;
    printf("pc: %04x wp: %04x st: %04x\n", regs.pc, regs.wp, regs.st);

    for (i = 0; i < 16; i++) {
        printf("%s = %04x  ", reg_name[i], get_register(i));
        if (i == 7 || i == 15)
            printf("\n");
    }
}

int registers(int argc, char **argv)
{
    int tmp;
    char *reg_name;
    int arg_no = 1;

    while (argc > arg_no) {
        reg_name = argv[arg_no++];
        if (1 == sscanf(argv[arg_no++], "%4x", &tmp)) {
            if (0 == strcmp("pc", reg_name))
                regs.pc = (WORD) tmp;
            else if (0 == strcmp("wp", reg_name))
                regs.wp = (WORD) tmp;
            else if (0 == strcmp("st", reg_name))
                regs.st = (WORD) tmp;
        }
    }

    show_registers();

    return 0;
}

int main(int argc, char **argv)
{
    cpu_state.bus = my_accessor;

    init_instruction();
    ram = malloc(ram_size);
    shell_set_accessor(my_accessor);
    shell_initialize("tms9900 shell");
    
    shell_add_command("disassemble", "Disassemble a word.", disassemble, false);
    shell_add_command("registers", "View/change regisgers.", registers, false);
    
    shell_loop();
    shell_finalize();
    printf("\n");
}

