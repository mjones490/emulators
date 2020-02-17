#include "instructions.h"

#define FLAG_S 0b10000000
#define FLAG_Z 0b01000000
#define FLAG_A 0b00010000
#define FLAG_P 0b00000100
#define FLAG_C 0b00000001

BYTE condition_flag[] = { FLAG_Z, FLAG_C, FLAG_P, FLAG_S };

static inline bool test_flag()
{
    BYTE flag = condition_flag[(cpu_state.code >> 4) & 0x03];
    BYTE compare_state = (cpu_state.code >> 3) & 0x01; 
    return ((regs.b.PSW & flag) > 0) == compare_state;
}

static inline void set_flags(BYTE flags)
{
    regs.b.PSW |= flags;
}

static inline void clear_flags(BYTE flags)
{
    regs.b.PSW &= ~flags;
}

static inline void toggle_flags(BYTE flags, bool state)
{
    if (state)
        set_flags(flags);
    else
        clear_flags(flags);
}

static inline BYTE are_set(BYTE flags)
{
    return regs.b.PSW & flags;
}

static inline void test_result(BYTE result)
{
    toggle_flags(FLAG_S, result & 0b10000000);
    toggle_flags(FLAG_Z, result == 0);
    BYTE p = result ^ (result >> 1);
    p ^= p >> 2;
    p ^= p >> 4;
    toggle_flags(FLAG_P, !(p & 0b00000001));
}

BYTE *reg_op[] = { &regs.b.B, &regs.b.C, &regs.b.D, &regs.b.E, &regs.b.H, &regs.b.L, NULL, &regs.b.A }; 
WORD *reg_pair[] = { &regs.w.BC, &regs.w.DE, &regs.w.HL, &regs.w.SP };

static inline BYTE get_source() 
{
    BYTE *sss = reg_op[cpu_state.code & 0b111];    
    return (sss == NULL)? get_byte(regs.w.HL) : *sss;
}

static inline BYTE get_dest()
{
    BYTE *ddd = reg_op[(cpu_state.code >> 3) & 0b111];
    return (ddd == NULL)? get_byte(regs.w.HL) : *ddd;
}

static inline void put_dest(BYTE value)
{
    BYTE *ddd = reg_op[(cpu_state.code >> 3) & 0b111];
    if (ddd == NULL)
        put_byte(regs.w.HL, value);
    else
        *ddd = value;
}

static inline WORD get_reg_pair()
{
    WORD *rp = reg_pair[(cpu_state.code >> 4) & 0b11];

    // If getting SP and PUSHing, return value in PSWA
    if (rp == &regs.w.SP && cpu_state.code == 0xf5)
        return regs.w.PSWA;

    return *rp;
}

static inline void set_reg_pair(WORD value)
{
    WORD *rp = reg_pair[(cpu_state.code >> 4) & 0b11];
    
    // If setting SP and POPing, set PSWA
    if (rp == &regs.w.SP && cpu_state.code == 0xf1)
        regs.w.PSWA = value;
    else
        *rp = value;
}

#define INSTRUCTION(mnemonic) \
    static void __ ## mnemonic()

INSTRUCTION(NOP)
{
}

INSTRUCTION(MOV)
{
    put_dest(get_source());
}

INSTRUCTION(MVI)
{
    put_dest(get_next_byte());
}

INSTRUCTION(LXI)
{
    set_reg_pair(get_next_word());
}

INSTRUCTION(LDA)
{
    regs.b.A = get_byte(get_next_word());
}

INSTRUCTION(STA)
{
    put_byte(get_next_word(), regs.b.A);
}

INSTRUCTION(LHLD)
{
    regs.w.HL = get_word(get_next_word());
}

INSTRUCTION(SHLD)
{
    put_word(get_next_word(), regs.w.HL);
}

INSTRUCTION(LDAX)
{
    regs.b.A = get_byte(get_reg_pair());
}

INSTRUCTION(STAX)
{
    put_byte(get_reg_pair(), regs.b.A);
}

INSTRUCTION(XCHG)
{
    WORD tmp = regs.w.HL;
    regs.w.HL = regs.w.DE;
    regs.w.DE = tmp;
}
   
INSTRUCTION(INR)
{
    BYTE value = get_dest() + 1;
    test_result(value);
    put_dest(value);
}

INSTRUCTION(DCR)
{
    BYTE value = get_dest() - 1;
    test_result(value);
    put_dest(value);
}

INSTRUCTION(INX)
{
    set_reg_pair(get_reg_pair() + 1);
}

INSTRUCTION(DCX)
{
    set_reg_pair(get_reg_pair() - 1);
}

INSTRUCTION(RLC)
{
    toggle_flags(FLAG_C, regs.b.A & 0x80);
    regs.b.A = (regs.b.A << 1) + (are_set(FLAG_C)? 0x01 : 0x00);
    test_result(regs.b.A);
}

INSTRUCTION(RRC)
{
    toggle_flags(FLAG_C, regs.b.A & 0x01);
    regs.b.A = (regs.b.A >> 1) + (are_set(FLAG_C)? 0x80 : 0x00); 
    test_result(regs.b.A);
}

INSTRUCTION(RAL)
{
    BYTE carry = are_set(FLAG_C);
    toggle_flags(FLAG_C, regs.b.A & 0x80);
    regs.b.A = (regs.b.A << 1) + (carry? 0x01 : 0x00);
    test_result(regs.b.A);
}

INSTRUCTION(RAR)
{
    BYTE carry = are_set(FLAG_C);
    toggle_flags(FLAG_C, regs.b.A & 0x01);
    regs.b.A = (regs.b.A >> 1) + (carry? 0x80 : 0x00);
    test_result(regs.b.A);
}

INSTRUCTION(ANA)
{
    regs.b.A &= get_source();
    test_result(regs.b.A);
    clear_flags(FLAG_C | FLAG_A);
}

INSTRUCTION(ANI)
{
    regs.b.A &= get_next_byte();
    test_result(regs.b.A);
    clear_flags(FLAG_C | FLAG_A);
}

INSTRUCTION(ORA)
{
    regs.b.A |= get_source();
    test_result(regs.b.A);
    clear_flags(FLAG_C | FLAG_A);
}

INSTRUCTION(ORI)
{
    regs.b.A |= get_next_byte();
    test_result(regs.b.A);
    clear_flags(FLAG_C | FLAG_A);
}

INSTRUCTION(XRA)
{
    regs.b.A ^= get_source();
    test_result(regs.b.A);
    clear_flags(FLAG_C | FLAG_A);
}

INSTRUCTION(XRI)
{
    regs.b.A ^= get_next_byte();
    test_result(regs.b.A);
    clear_flags(FLAG_C | FLAG_A);
}

INSTRUCTION(CMP)
{
    BYTE source = get_source();
    BYTE result = regs.b.A - source;
    test_result(result);
    toggle_flags(FLAG_C, regs.b.A < source);
}

INSTRUCTION(CPI)
{
    BYTE source = get_next_byte();
    BYTE result = regs.b.A - source;
    test_result(result);
    toggle_flags(FLAG_C, regs.b.A < source);
}

INSTRUCTION(JMP)
{
    regs.w.PC = get_next_word();
}

INSTRUCTION(Jccc) 
{
    WORD newPC = get_next_word();
    if (test_flag())
        regs.w.PC = newPC;
}

INSTRUCTION(CALL)
{
    WORD newPC = get_next_word();
    push_word(regs.w.PC);
    regs.w.PC = newPC;
}
INSTRUCTION(Cccc)
{
    WORD newPC = get_next_word();
    if (test_flag()) {
        push_word(regs.w.PC);
        regs.w.PC = newPC;
    }
}

INSTRUCTION(RET)
{
    regs.w.PC = pop_word();
}

INSTRUCTION(Rccc)
{
    if (test_flag())
        regs.w.PC = pop_word();
}

INSTRUCTION(PUSH)
{
    push_word(get_reg_pair());
}

INSTRUCTION(POP)
{
    set_reg_pair(pop_word());
}

INSTRUCTION(HALT)
{
    printf("Halt!\n");
}

#define TYPE_DEF(inst_type, start, stop, step) \
    { #inst_type, inst_type, start, stop, step }

struct {
    char* name;
    enum instruction_type inst_type;
    BYTE start;
    BYTE stop;
    BYTE step;
} type_info[] = {
    TYPE_DEF( DDDSSS,   0x37, 0x75, 0x01 ),
    TYPE_DEF( DDD,       0x00, 0x38, 0x08 ),
    TYPE_DEF( SSS,       0x00, 0x07, 0x01 ),
    TYPE_DEF( RP ,       0x00, 0x30, 0x10 ),
    TYPE_DEF( IMM,       0x00, 0x00, 0x00 ),
    TYPE_DEF( DDDIMM,    0x00, 0x38, 0x08 ),
    TYPE_DEF( RPIMM ,    0x00, 0x30, 0x10 ),
    TYPE_DEF( RPBD,      0x00, 0x10, 0x10 ),
    TYPE_DEF( ADDR,      0x00, 0x00, 0x00 ),
    TYPE_DEF( IMP,       0x00, 0x00, 0x00 ),
    TYPE_DEF( CCC,       0x00, 0x38, 0x08 )
};

#define INSTRUCTION_DEF(mnemonic, code, inst_type ) \
    { __ ## mnemonic, #mnemonic, inst_type, code }

struct instruction_t *instruction_map[256];
struct instruction_t instruction[] = {
    INSTRUCTION_DEF( NOP, 0x00, IMP ),
    INSTRUCTION_DEF( MOV, 0x40, DDDSSS ),
    INSTRUCTION_DEF( MVI, 0x06, DDDIMM ),
    INSTRUCTION_DEF( LXI, 0x01, RPIMM ),
    INSTRUCTION_DEF( LDA, 0x3a, ADDR ),
    INSTRUCTION_DEF( STA, 0x32, ADDR ),
    INSTRUCTION_DEF( LHLD, 0x2a, ADDR ),
    INSTRUCTION_DEF( SHLD, 0x22, ADDR ),
    INSTRUCTION_DEF( LDAX, 0x0a, RPBD ),
    INSTRUCTION_DEF( STAX, 0x02, RPBD ),
    INSTRUCTION_DEF( XCHG, 0xeb, IMP ),
    INSTRUCTION_DEF( INR, 0x04, DDD ),
    INSTRUCTION_DEF( DCR, 0x05, DDD ),
    INSTRUCTION_DEF( INX, 0x03, RP ),
    INSTRUCTION_DEF( DCX, 0x0b, RP ),
    INSTRUCTION_DEF( ANA, 0xa0, SSS ),
    INSTRUCTION_DEF( ANI, 0xe6, IMM),
    INSTRUCTION_DEF( ORA, 0xb0, SSS ),
    INSTRUCTION_DEF( ORI, 0xf6, IMM),
    INSTRUCTION_DEF( XRA, 0xa8, SSS ),
    INSTRUCTION_DEF( XRI, 0xee, IMM ),
    INSTRUCTION_DEF( CMP, 0xb8, SSS ),
    INSTRUCTION_DEF( CPI, 0xfe, IMM ),
    INSTRUCTION_DEF( RLC, 0x07, IMP ),
    INSTRUCTION_DEF( RRC, 0x0f, IMP ),
    INSTRUCTION_DEF( RAL, 0x17, IMP ),
    INSTRUCTION_DEF( RAR, 0x1f, IMP ),
    INSTRUCTION_DEF( JMP, 0xc3, ADDR ),
    INSTRUCTION_DEF( Jccc, 0xc2, CCC ),
    INSTRUCTION_DEF( CALL, 0xcd, ADDR ),
    INSTRUCTION_DEF( Cccc, 0xc4, CCC ),
    INSTRUCTION_DEF( RET, 0xc9, IMP ),
    INSTRUCTION_DEF( Rccc, 0xc0, CCC ),
    INSTRUCTION_DEF( PUSH, 0xC5, RP ),
    INSTRUCTION_DEF( POP, 0xC1, RP ),
    INSTRUCTION_DEF( HALT, 0x76, IMP )
};


void map_instructions()
{
    size_t instruction_count = sizeof(instruction) / sizeof(struct instruction_t);
    int instruction_num;
    int inst_type;
    int reg_count;
    BYTE map_code = 0;
    int total = 0;

    do {
        instruction_map[map_code] = 0;
    } while (++map_code);
    
    for (instruction_num = 0; instruction_num < instruction_count; ++instruction_num) {
        inst_type = instruction[instruction_num].inst_type;
        if (type_info[inst_type].inst_type != inst_type) {
            printf("Fatal: Mismatched instruction types found in initializtion.\n");
            exit(0);
        }
        printf("Setting instruction %s %s: ", instruction[instruction_num].mnemonic,
            type_info[inst_type].name);
        for (reg_count = type_info[inst_type].start; 
            reg_count <= type_info[inst_type].stop;
            reg_count += type_info[inst_type].step) {
            map_code = instruction[instruction_num].code | reg_count;
            printf("%02x ", map_code);
            instruction_map[map_code] = &instruction[instruction_num];
            total++;
            if (type_info[inst_type].step == 0)
                break;
        }
        printf("\n");
    }

    printf("%d codes mapped.\n", total);
}

