#ifndef __CPU_TYPES_H
#define __CPU_TYPES_H

union regs_t {
    struct {
        BYTE PSW;
        BYTE A;
        BYTE C;
        BYTE B;
        BYTE E;
        BYTE D;
        BYTE L;
        BYTE H;
        BYTE SPL;
        BYTE SPH;
        BYTE PCL;
        BYTE PCH;
        BYTE Y;
        BYTE X;
    } b;
    struct {
        WORD PSWA;
        WORD BC;
        WORD DE;
        WORD HL;
        WORD SP;
        WORD PC;
        WORD XY;
    } w;
};

struct cpu_state_t {
    accessor_t bus;
    port_accessor_t port; 
    union regs_t* regs;
    BYTE code;
    bool halted;
};

extern union regs_t regs;
extern struct cpu_state_t cpu_state;

static inline BYTE read_port(BYTE port)
{
    return cpu_state.port(port, true, 0);
}

static inline BYTE write_port(BYTE port, BYTE value)
{
    return cpu_state.port(port, false, value);
}

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
    regs.b.Y = get_byte(address);
    regs.b.X = get_byte(address + 1);
    return regs.w.XY;
}

static inline WORD put_word(WORD address, WORD value)
{
    regs.w.XY = value;
    put_byte(address, regs.b.Y);
    put_byte(address + 1, regs.b.X);
    return value;
}

static inline BYTE get_next_byte()
{
    return get_byte(regs.w.PC++);
}

static inline WORD get_next_word()
{
    regs.b.Y = get_next_byte();
    regs.b.X = get_next_byte();
    return regs.w.XY;
}

static inline WORD push_word(WORD value)
{
    regs.w.SP -= 2;
    put_word(regs.w.SP, value);
    return value;
}

static inline WORD pop_word()
{
    regs.w.XY = get_word(regs.w.SP);
    regs.w.SP += 2;
    return regs.w.XY;
}

#endif // __CPU_TYPES_H
