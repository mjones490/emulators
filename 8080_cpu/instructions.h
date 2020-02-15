#ifndef __instructions_h
#define __instructions_h

#include "8080_cpu.h"
#include "cpu_types.h"

struct instruction_t {
    void (*handler)();
    char* mnemonic;
    char* args;
    BYTE code;
    BYTE start;
    BYTE end;
    BYTE step;
};

extern struct instruction_t *instruction_map[256];
extern struct instruction_t instruction[];

void map_instructions();
#endif // __instructons_h
