#ifndef __instructions_h
#define __instructions_h
#include "6502_cpu_enums.h"
#include "cpu_types.h"
#include "modes.h"

//** Processer Flags
#define C 0x01 ///< Carry
#define Z 0x02 ///< Zero
#define I 0x04 ///< Interupt
#define D 0x08 ///< Decimal
#define B 0x10 ///< BRK
#define u 0x20 ///< unused
#define V 0x40 ///< Overflow
#define N 0x80 ///< Negative

struct instruction_desc_t {
    char name[4];
    void (*handler)(void);
};

struct instruction_t {
    enum MNEMONIC       mnemonic;
    BYTE                size;
    enum ADDRESS_MODE   mode;
    BYTE                clocks;
};

// This table is keyed on op code, and describes that
// specific code, such as mnemonic, size, mode, etc.
extern struct instruction_t instruction[256];

// This table is key on mnemonic, and describes that
// instruction.
extern struct instruction_desc_t *instruction_desc;

// For example, to get op code 0x0E, look up that code in the
// instruction table.
//
// Nmonic:      ASL
// Bytes:       3
// Mode:        ABS
// Clocks:      6
//
// Then look in instruction_desc to get the function that handles ASL.

void init_instructions();

#define SET_INSTRUCTION(mnemonic) \
    set_instruction(mnemonic, #mnemonic, __ ## mnemonic)
#endif

#define INSTRUCTION(mnemonic) \
    static void __ ## mnemonic()

void set_map(BYTE code, enum MNEMONIC mnemonic, BYTE size, 
    enum ADDRESS_MODE mode, BYTE clocks);
void set_instruction(enum MNEMONIC mnemonic, char* name, 
    void (*handler)(void));

static WORD address;
static BYTE value;

static inline void test_result(BYTE result)
{
    toggle_flags(Z, result == 0);
    toggle_flags(N, result & 0x80);
}

static inline void compare(BYTE reg)
{
    value = get_operand();
    toggle_flags(C, reg >= value);
    test_result(reg - value);
}

static inline void branch(bool d)
{
    value = get_next_byte();
    address = regs.PC + ((value & 0x80)? word(value, 0xff) : value);
    if (d) {
        ++running.clocks;
        if (hi(address) != hi(regs.PC))
            ++running.clocks;
        regs.PC = address; 
    }
}

