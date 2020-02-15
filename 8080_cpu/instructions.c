#include "instructions.h"

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
    return *rp;
}

static inline void set_reg_pair(WORD value)
{
    *reg_pair[(cpu_state.code >> 4) & 0b11] = value;
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
    put_dest(value);
}

INSTRUCTION(DCR)
{
    BYTE value = get_dest() - 1;
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

INSTRUCTION(JMP)
{
    regs.w.PC = get_next_word();
}

INSTRUCTION(HALT)
{
    printf("Halt!\n");
}

#define DDDSSS  0x37, 0x75, 0x01
#define DDD     0x00, 0x38, 0x08
#define DI      0x00, 0x38, 0x08 
#define RPI     0x00, 0x30, 0x10
#define ADDR    0x00, 0x00, 0x00
#define RP      0x00, 0x30, 0x10
#define RPBD    0x00, 0x10, 0x10
#define IMP     0x00, 0x00, 0x00

#define INSTRUCTION_DEF(mnemonic, code, args) \
    { __ ## mnemonic, #mnemonic, #args, code, args }

struct instruction_t *instruction_map[256];
struct instruction_t instruction[] = {
    INSTRUCTION_DEF( NOP, 0x00, IMP ),
    INSTRUCTION_DEF( MOV, 0x40, DDDSSS),
    INSTRUCTION_DEF( MVI, 0x06, DI ),
    INSTRUCTION_DEF( LXI, 0x01, RPI ),
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
    INSTRUCTION_DEF( JMP, 0xc3, ADDR ),
    INSTRUCTION_DEF( HALT, 0x76, IMP )
};


void map_instructions()
{
    size_t instruction_count = sizeof(instruction) / sizeof(struct instruction_t);
    int instruction_num;
    int reg_count;
    BYTE map_code = 0;

    do {
        instruction_map[map_code] = 0;
    } while (++map_code);
    
    for (instruction_num = 0; instruction_num < instruction_count; ++instruction_num) {
        printf("Setting instruction %s %s: ", instruction[instruction_num].mnemonic, 
            instruction[instruction_num].args);
        for (reg_count = instruction[instruction_num].start; 
            reg_count <= instruction[instruction_num].end;
            reg_count += instruction[instruction_num].step) {
            map_code = instruction[instruction_num].code | reg_count;
            printf("%02x ", map_code);
            instruction_map[map_code] = &instruction[instruction_num];
            if (instruction[instruction_num].step == 0)
                break;
        }
        printf("\n");
    }

    printf("\n");
}

