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

#define FLAG_LGT    0x8000 // Logical Greater Than
#define FLAG_AGT    0x4000 // Arithmatic Greater Than
#define FLAG_EQU    0x2000 // Equal
#define FLAG_CRY    0x1000 // Carry
#define FLAG_OVF    0x0800 // Overflow
#define FLAG_PAR    0x0400 // Parity
#define FLAG_XOP    0x0200 // Extended Operation

static inline void set_flag(WORD flags, bool state)
{
    if (state)
        regs.st |= flags;
    else
        regs.st &= ~flags;
}

static inline void toggle_flag(WORD flags)
{
    regs.st ^= flags;
}

static inline bool check_flag(WORD flags)
{
    return (regs.st & flags) > 0;
}

static inline WORD set_result_flags(WORD result)
{
    set_flag(FLAG_EQU, result == 0);
    set_flag(FLAG_LGT, result != 0);
    set_flag(FLAG_AGT, (result != 0) && !msb(result));
    return result;
}

static inline BYTE set_result_flags_byte(BYTE result)
{
    set_result_flags(result << 8);
    return result;
}

static inline void set_parity_flag(BYTE result)
{
    result ^= result >> 4;
    result ^= result >> 2;
    result ^= result >> 1;
    set_flag(FLAG_PAR, result & 0x01);
}
static inline WORD adder(WORD op1, WORD op2)
{
    unsigned int result32 = op1 + op2;
    WORD result = result32 & 0xffff;
    set_result_flags(result);
    set_flag(FLAG_CRY, (result32 & 0x10000) != 0);
    set_flag(FLAG_OVF, (msb(op1) == msb(op2)) && (msb(result) != msb(op1)));

    return result;
}

static inline BYTE adder_byte(BYTE op1, BYTE op2)
{
    BYTE result = adder(op1 << 8, op2 << 8) >> 8;
    set_parity_flag(result);
    return result;
}

static inline void compare(WORD op1, WORD op2)
{
    set_flag(FLAG_EQU, op1 == op2);
    set_flag(FLAG_LGT, (msb(op1) && !msb(op2)) ||
        ((msb(op1) == msb(op2)) && msb(op2 - op1)));
    set_flag(FLAG_AGT, (!msb(op1) && msb(op2)) ||
        ((msb(op1) == msb(op2)) && msb(op2 - op1)));
}

#define INSTRUCTION(mnemonic) \
    void __ ## mnemonic(struct operands_t *ops)

INSTRUCTION(A)
{
    put_word(ops->dest, adder(get_word(ops->dest), get_word(ops->src)));
}

INSTRUCTION(AB)
{
    put_byte(ops->dest, adder_byte(get_byte(ops->dest), get_byte(ops->src)));
}

INSTRUCTION(ANDI)
{
    put_word(ops->dest, set_result_flags(get_word(ops->dest) & get_next_word()));
}

INSTRUCTION(B)
{
    regs.pc = ops->src;
}

INSTRUCTION(BL)
{
   set_register_value(11, regs.pc);
   regs.pc = ops->src;
}

INSTRUCTION(BLWP)
{
    WORD wp = regs.wp;
    regs.wp = get_word(ops->src);

    set_register_value(13, wp);
    set_register_value(14, regs.pc);
    set_register_value(15, regs.st);

    regs.pc = get_word(ops->src + 2);
}

INSTRUCTION(C)
{
    compare(get_word(ops->src), get_word(ops->dest));
}

INSTRUCTION(CB)
{
    compare(get_byte(ops->src) << 8, get_byte(ops->dest) << 8);
}

INSTRUCTION(CI)
{   
    compare(get_word(ops->dest), get_next_word());
}

INSTRUCTION(CLR)
{
    put_word(ops->src, 0x0000);
}

INSTRUCTION(DEC)
{
    put_word(ops->src, adder(get_word(ops->src), 0xffff));
}

INSTRUCTION(DECT)
{
    put_word(ops->src, adder(get_word(ops->src), 0xfffe));
}

INSTRUCTION(INC)
{
    put_word(ops->src, adder(get_word(ops->src), 0x0001));
}

INSTRUCTION(INCT)
{
    put_word(ops->src, adder(get_word(ops->src), 0x0002));
}

INSTRUCTION(INV)
{
    put_word(ops->src, get_word(ops->src) ^ 0xffff);
}

#define JUMP(condition) \
    if (condition) \
        regs.pc += (ops->disp * 2)

INSTRUCTION(JMP)
{
    JUMP(true);
}

INSTRUCTION(JEQ)
{
    JUMP(check_flag(FLAG_EQU));
}

INSTRUCTION(JNE)
{
    JUMP(!check_flag(FLAG_EQU));
}

INSTRUCTION(LI)
{
    put_word(ops->dest, set_result_flags(get_next_word()));
}

INSTRUCTION(LWPI)
{
    regs.wp = get_next_word();
}

INSTRUCTION(MOV)
{
    put_word(ops->dest, set_result_flags(get_word(ops->src)));
}

INSTRUCTION(MOVB)
{
    put_byte(ops->dest, set_result_flags_byte(get_byte(ops->src)));
}

INSTRUCTION(ORI)
{
    put_word(ops->dest, set_result_flags(get_word(ops->dest) | get_next_word()));
}

INSTRUCTION(RTWP)
{
    regs.st = get_register_value(15);
    regs.pc = get_register_value(14);
    regs.wp = get_register_value(13);
}

INSTRUCTION(S)
{
    put_word(ops->dest, adder(get_word(ops->dest), -get_word(ops->src)));
}

INSTRUCTION(SB)
{
    put_byte(ops->dest, adder_byte(get_byte(ops->dest), -get_byte(ops->src)));
}

INSTRUCTION(SETO)
{
    put_word(ops->src, 0xffff);
}

INSTRUCTION(STST)
{
    put_word(ops->dest, regs.st);
}

INSTRUCTION(STWP)
{
    put_word(ops->dest, regs.wp);
}

INSTRUCTION(SWPB)
{
    put_word(ops->src, (get_byte(ops->src + 1) << 8) | get_byte(ops->src));
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
    INSTRUCTION_DEF( A,     0xa000, GRP_0, FMT_I    ),
    INSTRUCTION_DEF( AB,    0xb000, GRP_0, FMT_I    ), 
    INSTRUCTION_DEF( ANDI,  0x0240, GRP_4, FMT_VIII ),
    INSTRUCTION_DEF( B,     0x0440, GRP_3, FMT_VI   ),
    INSTRUCTION_DEF( BL,    0x0680, GRP_3, FMT_VI   ), 
    INSTRUCTION_DEF( BLWP,  0x0400, GRP_3, FMT_VI   ),
    INSTRUCTION_DEF( C,     0x8000, GRP_0, FMT_I    ),
    INSTRUCTION_DEF( CB,    0x9000, GRP_0, FMT_I    ),
    INSTRUCTION_DEF( CI,    0x0280, GRP_4, FMT_VIII ),
    INSTRUCTION_DEF( CLR,   0x04c0, GRP_3, FMT_VI   ),
    INSTRUCTION_DEF( DEC,   0x0600, GRP_3, FMT_VI   ),
    INSTRUCTION_DEF( DECT,  0x0640, GRP_3, FMT_VI   ),
    INSTRUCTION_DEF( INC,   0x0580, GRP_3, FMT_VI   ),
    INSTRUCTION_DEF( INCT,  0x05c0, GRP_3, FMT_VI   ),
    INSTRUCTION_DEF( INV,   0x0540, GRP_3, FMT_VI   ),
    INSTRUCTION_DEF( JEQ,   0x1300, GRP_2, FMT_II   ),
    INSTRUCTION_DEF( JMP,   0x1000, GRP_2, FMT_II   ),
    INSTRUCTION_DEF( JNE,   0x1600, GRP_2, FMT_II   ),
    INSTRUCTION_DEF_NULL( LDCR,  0x3000, GRP_1, FMT_IV   ),
    INSTRUCTION_DEF( LI,    0x0200, GRP_4, FMT_VIII ),
    INSTRUCTION_DEF( LWPI,  0x02e0, GRP_4, FMT_XI   ),
    INSTRUCTION_DEF( MOV,   0xc000, GRP_0, FMT_I    ),
    INSTRUCTION_DEF( MOVB,  0xd000, GRP_0, FMT_I    ),
    INSTRUCTION_DEF( ORI,   0x0260, GRP_4, FMT_VIII ),
    INSTRUCTION_DEF( RTWP,  0x0380, GRP_4, FMT_VII  ),
    INSTRUCTION_DEF_NULL( SBO,   0x1d00, GRP_2, FMT_X   ),
    INSTRUCTION_DEF( S,     0x6000, GRP_0, FMT_I    ),
    INSTRUCTION_DEF( SB,    0x7000, GRP_0, FMT_I    ), 
    INSTRUCTION_DEF( SETO,  0x0700, GRP_3, FMT_VI   ),
    INSTRUCTION_DEF( STST,  0x02c0, GRP_4, FMT_XII  ),
    INSTRUCTION_DEF( STWP,  0x02a0, GRP_4, FMT_XII  ),
    INSTRUCTION_DEF( SWPB,  0x06c0, GRP_3, FMT_VI   ),
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
        case FMT_X:
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
        case FMT_XII:
            operands = format_VIII(op);
            break;
        }

        instruction->handler(&operands);
    }    
}

