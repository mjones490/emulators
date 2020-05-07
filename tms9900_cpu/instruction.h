#ifndef __INSTRUCTION_H
#define __INSTRUCTION_H

enum entry_type {
    ET_INSTRUCTION,
    ET_LIST
};

struct instruction_t {
    enum entry_type     type;    
    char                *mnemonic;
    WORD                code;
    int                 format;
    void                (*handler)(struct operands_t *ops);
};

extern struct instruction_t instruction[];

void init_instruction();
void execute_instruction();
struct instruction_t *decode_instruction(WORD code);
void finalize_instruction();
#endif
