#ifndef __instructions_h
#define __instructions_h

#include "8080_cpu.h"
#include "cpu_types.h"

enum instruction_type {
    DDDSSS,
    DDD,
    SSS,
    RP,
    IMM,
    DDDIMM,
    RPIMM,
    RPBD,
    ADDR,
    IMP,
    CCC,
    NNN
};

struct instruction_t {
    void (*handler)();
    char* mnemonic;
    enum instruction_type inst_type;
    BYTE code;
    BYTE clocks;
};

extern struct instruction_t *instruction_map[256];
extern struct instruction_t instruction[];

void map_instructions();
#endif // __instructons_h
