#ifndef __CPU_TYPES_H
#define __CPU_TYPES_H

struct regs_t {
    WORD pc;    // Program counter
    WORD wp;    // Workspace pointer
    WORD st;    // Status
};

struct cpu_state_t {
    accessor_t  bus;
};

extern struct regs_t regs;
extern struct cpu_state_t cpu_state;

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
    return ((WORD) get_byte(address) << 8) | get_byte(address + 1);
}

static inline WORD put_word(WORD address, WORD value)
{
    put_byte(address, value >> 8);
    put_byte(address + 1, value & 0xff);
    return value;
}

static inline WORD get_next_word()
{
    WORD value = get_word(regs.pc);
    regs.pc += 2;
    return value;
}

static inline WORD get_register_address(int reg_no)
{
    return regs.wp + (reg_no << 1);
}

static inline WORD get_register_value(int reg_no)
{
    return get_word(get_register_address(reg_no));
}

static inline WORD set_register_value(int reg_no, WORD value)
{
    return put_word(get_register_address(reg_no), value);
}

#endif
