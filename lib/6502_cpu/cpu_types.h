#ifndef __CPU_TYPES_H
#define __CPU_TYPES_H
#include "6502_cpu_enums.h"
#include "types.h"
/******
  CPU
 ******/
struct regs_t {
    WORD PC;
    BYTE SP;
    BYTE PS;
    BYTE A;
    BYTE X;
    BYTE Y;
};

struct cpu_state_t {
    accessor_t bus;
    struct regs_t *regs;
    WORD prev_PC;
    BYTE signal; 
};

struct running_t { 
    struct instruction_desc_t *descripter;
    struct instruction_t *instruction;
    enum ADDRESS_MODE op_mode;
    WORD op_address;
    int clocks;
};

extern struct cpu_state_t cpu_state;
extern struct running_t running;
extern struct regs_t regs;

static inline void set_signal(BYTE signal)
{
    cpu_state.signal |= signal;
}

static inline void clear_signal(BYTE signal)
{
    cpu_state.signal &= ~signal;
}

static inline void toggle_signal(BYTE signal)
{
    cpu_state.signal ^= signal;
}

static inline BYTE get_signal(BYTE signal)
{
    return cpu_state.signal & signal;
}

void init_cpu(accessor_t bus);

static inline BYTE get_byte(WORD address) 
{
    return cpu_state.bus(address, true, 0);
}

static inline BYTE put_byte(WORD address, BYTE value) 
{
    return cpu_state.bus(address, false, value);
}

static inline WORD get_word(WORD address)
{
    return word(get_byte(address), get_byte(address + 1));
}

static inline BYTE get_next_byte()
{
    return get_byte(regs.PC++);
}

static inline WORD get_next_word()
{
    BYTE lo = get_next_byte();
    BYTE hi = get_next_byte();
    return word(lo, hi);
}

static inline void push(BYTE value)
{
    put_byte(word(regs.SP--, 0x01), value);
}

static inline BYTE pop()
{
    return get_byte(word(++regs.SP, 0x01));
}

// Flag functions
static inline BYTE get_flags(BYTE flags)
{
    return regs.PS & flags;
}

static inline void set_flags(BYTE flags)
{
    regs.PS |= flags;
}

static inline bool are_set(BYTE flags)
{
    return regs.PS & flags;
}

static inline void clear_flags(BYTE flags)
{
    regs.PS &= ~flags;
}

static inline bool are_clear(BYTE flags)
{
    return !(are_set(flags));
}

static inline void toggle_flags(BYTE flags, bool state)
{
    if (state)
        set_flags(flags);
    else
        clear_flags(flags);
}


void interrupt(WORD vector);
#endif
