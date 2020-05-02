#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "shell.h"
#include "cpu_types.h"
#include "format.h"
#include "instruction.h"

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

#define GRP_0 0
#define GRP_1 1
#define GRP_2 2
#define GRP_3 3
#define GRP_4 4

#define INSTRUCTION(mnemonic) \
    void __ ## mnemonic(struct operands_t *ops)

INSTRUCTION(B)
{
    regs.pc = ops->src;
}

INSTRUCTION(BL)
{
   set_register_value(11, regs.pc);
   regs.pc = ops->src;
}

INSTRUCTION(JMP)
{
    regs.pc += (ops->disp * 2);
}

INSTRUCTION(LI)
{
    put_word(ops->dest, get_next_word());
}

INSTRUCTION(LWPI)
{
    regs.wp = get_next_word();
}

INSTRUCTION(MOV)
{
    put_word(ops->dest, get_word(ops->src));
}

INSTRUCTION(MOVB)
{
    put_byte(ops->dest, get_byte(ops->src));
}

INSTRUCTION(XOR)
{
    WORD value = get_word(ops->dest);
    put_word(ops->dest, value ^ get_word(ops->src));
}

#define INSTRUCTION_DEF(mnemonic, code, group, format) \
    { ET_INSTRUCTION, #mnemonic, group, code, format, __ ## mnemonic }

#define INSTRUCTION_DEF_NULL(mnemonic, code, group, format) \
    { ET_INSTRUCTION, #mnemonic, group, code, format, NULL }

struct instruction_t instruction[] = {
    INSTRUCTION_DEF_NULL( A,     0xa000, GRP_0, FMT_I    ),
    INSTRUCTION_DEF_NULL( AB,    0xb000, GRP_0, FMT_I    ), 
    INSTRUCTION_DEF( B,     0x0440, GRP_3, FMT_VI   ),
    INSTRUCTION_DEF( BL,    0x0680, GRP_3, FMT_VI   ), 
    INSTRUCTION_DEF( JMP,   0x1000, GRP_2, FMT_II   ),
    INSTRUCTION_DEF_NULL( LDCR,  0x3000, GRP_1, FMT_IV   ),
    INSTRUCTION_DEF( LI,    0x0200, GRP_4, FMT_VIII ),
    INSTRUCTION_DEF( LWPI,  0x02e0, GRP_4, FMT_IX   ),
    INSTRUCTION_DEF( MOV,   0xc000, GRP_0, FMT_I    ),
    INSTRUCTION_DEF( MOVB,  0xd000, GRP_0, FMT_I    ),
    INSTRUCTION_DEF_NULL( RTWP,  0x0380, GRP_4, FMT_VII  ),
    INSTRUCTION_DEF_NULL( SBO,   0x1d00, GRP_2, FMT_X   ),
    INSTRUCTION_DEF_NULL( SLA,   0x0a00, GRP_2, FMT_V    ),
    INSTRUCTION_DEF( XOR,   0x2400, GRP_1, FMT_III  )
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

void execute_instruction()
{
    WORD op = get_next_word();
    struct instruction_t *instruction = decode_instruction(op);
    struct operands_t operands;

    if (instruction != 0 && instruction->handler != NULL) {
        switch (instruction->format) {
        case FMT_I:
            operands = format_I(op);
            break;
            
        case FMT_II:
            operands = format_II(op);
            break;

        case FMT_III:
            operands = format_III(op);
            break;

        case FMT_IV:
            operands = format_IV(op);
            break;

        case FMT_V:
            operands = format_V(op);
            break;

        case FMT_VI:
            operands = format_VI(op);
            break;

        case FMT_VIII:
            operands = format_VIII(op);
            break;
        }

        instruction->handler(&operands);
    }    
}

