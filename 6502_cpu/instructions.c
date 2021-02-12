#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "instructions.h"
#include "modes.h"

struct instruction_desc_t instruction_desc[MNEMONIC_COUNT + 1];
struct instruction_t instruction[256];

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

static WORD unpack_bcd(BYTE value)
{
    return word(value & 0x0f, value >> 4);
}

static WORD pack_bcd(WORD value)
{
    if (lo(value) > 0x09)
        value += 0x0006;
    
    value =  (((value & 0xff00) >> 4) + (value & 0x00ff));
    
    if ((value & 0x00f0) > 0x0090)
        value += 0x0060;

    return value;
}

static inline void add()
{
    static WORD result;
    
    if (regs.PS & D) 
        result = pack_bcd(
            unpack_bcd(regs.A) + 
            unpack_bcd(value) + 
            (are_set(C)? 1 : 0));
    else 
        result = (WORD)(regs.A + value) + (are_set(C)? 1 : 0);
        
    toggle_flags(C, result & 0x0100);
    result = lo(result);
    toggle_flags(V, (regs.A ^ result) & (value ^ result) & 0x80);
/*   if (regs.PS & D) {
        if ((result & 0x000F) > 0x0009)
            result += 0x0006;
        if ((result & 0x00F0) > 0x0090)
            result += 0x0060;
    } */
    regs.A = result;
    test_result(regs.A);
}

//----------------------------------------
// Instructions
#define INSTRUCTION(mnemonic) \
    static void __ ## mnemonic()

INSTRUCTION(ADC)
{
    value = get_operand();
    add();
}

INSTRUCTION(AND)
{
    regs.A &= get_operand();
    test_result(regs.A);
}

INSTRUCTION(ASL)
{
    value = get_operand();
    toggle_flags(C, value & 0x80);
    value <<= 1;
    test_result(value);
    replace_operand(value);
}

INSTRUCTION(BCC)
{
    branch(are_clear(C));
}

INSTRUCTION(BCS)
{
    branch(are_set(C));
}

INSTRUCTION(BEQ)
{
    branch(are_set(Z));
}

INSTRUCTION(BIT)
{
    value = get_operand();
    toggle_flags(V, value & 0x40);
    toggle_flags(N, value & 0x80);
    toggle_flags(Z, 0 == (value & regs.A));
}

INSTRUCTION(BMI)
{
    branch(are_set(N));
}

INSTRUCTION(BNE)
{
    branch(are_clear(Z));
}

INSTRUCTION(BPL)
{
    branch(are_clear(N));
}

void interrupt(WORD vector)
{
    push(hi(regs.PC));
    push(lo(regs.PC));
    push(regs.PS);
    regs.PC = get_word(vector);
    set_flags(I);
}

INSTRUCTION(BRK)
{
    get_next_byte();
    set_flags(B);
    interrupt(0xFFFE);
    clear_flags(B);
}

INSTRUCTION(RTI)
{
    regs.PS = pop() & ~B;
    value = pop();
    regs.PC = word(value, pop());
}

INSTRUCTION(BVC)
{
    branch(are_clear(V));
}

INSTRUCTION(BVS)
{
    branch(are_set(V));
}

INSTRUCTION(CLC)
{
    clear_flags(C);
}

INSTRUCTION(CLD)
{
    clear_flags(D);
}

INSTRUCTION(CLI)
{
    set_signal(SIG_CLI);
}

INSTRUCTION(CLV)
{
    clear_flags(V);
}

INSTRUCTION(CMP)
{
    compare(regs.A);
}

INSTRUCTION(CPX)
{
    compare(regs.X);
}

INSTRUCTION(CPY)
{
    compare(regs.Y);
}

INSTRUCTION(DEC)
{
    value = get_operand();
    test_result(--value);       
    replace_operand(value);
}

INSTRUCTION(DEX)
{
    test_result(--regs.X);
}

INSTRUCTION(DEY)
{
    test_result(--regs.Y);
}

INSTRUCTION(EOR)
{
    regs.A ^= get_operand();
    test_result(regs.A);
}

INSTRUCTION(INC)
{
    value = get_operand();
    test_result(++value);       
    replace_operand(value);
}

INSTRUCTION(INX)
{
    test_result(++regs.X);
}

INSTRUCTION(INY)
{
    test_result(++regs.Y);
}

INSTRUCTION(JMP)
{
    regs.PC = get_next_word();
    if (running.op_mode == IND)
        regs.PC = get_next_word();
}

INSTRUCTION(JSR)
{
    address = get_next_word();
    --regs.PC;
    push(hi(regs.PC));
    push(lo(regs.PC));
    regs.PC = address;
}

INSTRUCTION(LDA)
{
    regs.A = get_operand();
    test_result(regs.A);
}

INSTRUCTION(LDX)
{
    regs.X = get_operand();
    test_result(regs.X);
}

INSTRUCTION(LDY)
{
    regs.Y = get_operand();
    test_result(regs.Y);
}

INSTRUCTION(LSR)
{   
    value = get_operand();
    toggle_flags(C, value & 0x01);
    value >>= 1;
    test_result(value);
    replace_operand(value);
}

INSTRUCTION(NOP)
{
}

INSTRUCTION(ORA)
{
    regs.A |= get_operand();
    test_result(regs.A);
}

INSTRUCTION(PHA)
{
    push(regs.A);
}

INSTRUCTION(PHP)
{
    push(regs.PS);
}

INSTRUCTION(PLA)
{
    regs.A = pop();
    test_result(regs.A);
}

INSTRUCTION(PLP)
{
    value = regs.PS & I;
    regs.PS = pop() & ~B;

    if (regs.PS & I)
        set_signal(SIG_SEI);
    else
        set_signal(SIG_CLI);

    toggle_flags(I, value);

}

INSTRUCTION(ROL)
{
    static BYTE result;
    value = get_operand();
    result = (value << 1) | (get_flags(C)? 0x01 : 0x00);
    toggle_flags(C, value & 0x80);
    test_result(result);
    replace_operand(result);
}

INSTRUCTION(ROR)
{
    static BYTE result;
    value = get_operand();
    result = (value >> 1) | (get_flags(C)? 0x80 : 0x00);
    toggle_flags(C, value & 0x01);
    test_result(result);
    replace_operand(result);
}

INSTRUCTION(RTS)
{
    value = pop();
    regs.PC = word(value, pop());
    ++regs.PC;
}

INSTRUCTION(SBC)
{
    value = get_operand();
    if (are_set(D))
        value = 0x99 - value;
    else
        value = 0xff - value;
    add();
}

INSTRUCTION(SEC)
{
    set_flags(C);
}

INSTRUCTION(SED)
{
    set_flags(D);
}

INSTRUCTION(SEI)
{
    set_signal(SIG_SEI);
}

INSTRUCTION(STA)
{
    put_operand(regs.A);
}

INSTRUCTION(STX)
{
    put_operand(regs.X);
}

INSTRUCTION(STY)
{
    put_operand(regs.Y);
}

INSTRUCTION(TAX)
{
    regs.X = regs.A;
    test_result(regs.X);
}

INSTRUCTION(TAY)
{
    regs.Y = regs.A;
    test_result(regs.Y);
}

INSTRUCTION(TSX)
{
    regs.X = regs.SP;
    test_result(regs.X);
}

INSTRUCTION(TXA)
{
    regs.A = regs.X;
    test_result(regs.A);
}

INSTRUCTION(TXS)
{
    regs.SP = regs.X;
}

INSTRUCTION(TYA)
{
    regs.A = regs.Y;
    test_result(regs.A);
}

static void set_instruction(enum MNEMONIC mnemonic, char* name, 
    void (*handler)(void))
{
    strncpy(instruction_desc[mnemonic].name, name, 3);
    instruction_desc[mnemonic].handler = handler;
}

/**
 * Map each opcode to its instruction and other information.
 *
 * @param[in] code Op code
 * @param[in] map  Map 
 */
void set_map(BYTE code, enum MNEMONIC mnemonic, BYTE size, 
    enum ADDRESS_MODE mode, BYTE clocks)
{
    instruction[code].mnemonic = mnemonic;
    instruction[code].size = size;
    instruction[code].mode = mode;
    instruction[code].clocks = clocks;
}

#define SET_INSTRUCTION(mnemonic) \
    set_instruction(mnemonic, #mnemonic, __ ## mnemonic)
     
void init_instructions()
{
    int i;
       
    SET_INSTRUCTION(ADC);
    SET_INSTRUCTION(AND);
    SET_INSTRUCTION(ASL);
    SET_INSTRUCTION(BCC);
    SET_INSTRUCTION(BCS);
    SET_INSTRUCTION(BEQ);
    SET_INSTRUCTION(BIT);
    SET_INSTRUCTION(BMI);
    SET_INSTRUCTION(BNE);
    SET_INSTRUCTION(BPL);
    SET_INSTRUCTION(BRK);
    SET_INSTRUCTION(BVC);
    SET_INSTRUCTION(BVS);
    SET_INSTRUCTION(CLC);
    SET_INSTRUCTION(CLD);
    SET_INSTRUCTION(CLI);
    SET_INSTRUCTION(CLV);
    SET_INSTRUCTION(CMP);
    SET_INSTRUCTION(CPX);
    SET_INSTRUCTION(CPY);
    SET_INSTRUCTION(DEC);    
    SET_INSTRUCTION(DEX);    
    SET_INSTRUCTION(DEY);    
    SET_INSTRUCTION(EOR);    
    SET_INSTRUCTION(INC);    
    SET_INSTRUCTION(INX);    
    SET_INSTRUCTION(INY);    
    SET_INSTRUCTION(JMP);    
    SET_INSTRUCTION(JSR);    
    SET_INSTRUCTION(LDA);    
    SET_INSTRUCTION(LDX);
    SET_INSTRUCTION(LDY);
    SET_INSTRUCTION(LSR);
    SET_INSTRUCTION(NOP);
    SET_INSTRUCTION(ORA);
    SET_INSTRUCTION(PHA);
    SET_INSTRUCTION(PHP);
    SET_INSTRUCTION(PLA);
    SET_INSTRUCTION(PLP);
    SET_INSTRUCTION(ROL);
    SET_INSTRUCTION(ROR);
    SET_INSTRUCTION(RTI);
    SET_INSTRUCTION(RTS);
    SET_INSTRUCTION(SBC);
    SET_INSTRUCTION(SEC);
    SET_INSTRUCTION(SED);
    SET_INSTRUCTION(SEI);
    SET_INSTRUCTION(STA);
    SET_INSTRUCTION(STX);
    SET_INSTRUCTION(STY);
    SET_INSTRUCTION(TAX);
    SET_INSTRUCTION(TAY);
    SET_INSTRUCTION(TSX);
    SET_INSTRUCTION(TXA);
    SET_INSTRUCTION(TXS);
    SET_INSTRUCTION(TYA);
    

    for (i = 0; i < 256; ++i)
        set_map(i, NOP, 1, IMP, 0);

    set_map(0x00, BRK, 1, IMP, 7);
    set_map(0x01, ORA, 2, ZPIX, 6);
    set_map(0x05, ORA, 2, ZP, 3);
    set_map(0x06, ASL, 2, ZP, 5);
    set_map(0x08, PHP, 1, IMP, 3);
    set_map(0x09, ORA, 2, IMM, 2);
    set_map(0x0A, ASL, 1, ACC, 2);
    set_map(0x0D, ORA, 3, ABS, 4);
    set_map(0x0E, ASL, 3, ABS, 6);
    set_map(0x10, BPL, 2, R, 2);
    set_map(0x11, ORA, 2, ZPIY, 5);
    set_map(0x15, ORA, 2, ZPX, 4);
    set_map(0x16, ASL, 2, ZPX, 6);
    set_map(0x18, CLC, 1, IMP, 2);
    set_map(0x19, ORA, 3, AIY, 4);
    set_map(0x1D, ORA, 3, AIX, 4);
	set_map(0x1E, ASL, 3, AIX, 7);
    set_map(0x20, JSR, 3, ABS, 6);
    set_map(0x21, AND, 2, ZPIX, 6);
    set_map(0x24, BIT, 2, ZP, 3);
    set_map(0x25, AND, 2, ZP, 3);
    set_map(0x26, ROL, 2, ZP, 5);
    set_map(0x28, PLP, 1, IMP, 4);
    set_map(0x29, AND, 2, IMM, 2);
    set_map(0x2A, ROL, 1, ACC, 2);
    set_map(0x2C, BIT, 3, ABS, 4);
    set_map(0x2D, AND, 3, ABS, 4);
    set_map(0x2E, ROL, 3, ABS, 6);
    set_map(0x30, BMI, 2, R, 2);
    set_map(0x31, AND, 2, ZPIY, 5);
    set_map(0x35, AND, 2, ZPX, 4);
	set_map(0x36, ROL, 2, ZPX, 6);
    set_map(0x38, SEC, 1, IMP, 2);
	set_map(0x39, AND, 3, AIY, 4);
    set_map(0x3D, AND, 3, AIX, 4);	
    set_map(0x3E, ROL, 3, AIX, 7);
    set_map(0x40, RTI, 1, IMP, 6);
    set_map(0x41, EOR, 2, ZPIX, 6);
    set_map(0x45, EOR, 2, ZP, 3);
    set_map(0x46, LSR, 2, ZP, 5);
    set_map(0x48, PHA, 1, IMP, 3);
    set_map(0x49, EOR, 2, IMM, 2);
    set_map(0x4A, LSR, 1, ACC, 2);
    set_map(0x4C, JMP, 3, ABS, 3);
    set_map(0x4D, EOR, 3, ABS, 4);
    set_map(0x4E, LSR, 3, ABS, 6);
    set_map(0x50, BVC, 2, R, 2);
    set_map(0x51, EOR, 2, ZPIY, 5);
    set_map(0x55, EOR, 2, ZPX, 4);
    set_map(0x56, LSR, 2, ZPX, 6);
    set_map(0x58, CLI, 1, IMP, 2);
    set_map(0x59, EOR, 3, AIY, 4);
    set_map(0x5D, EOR, 3, AIX, 4);
    set_map(0x5E, LSR, 3, AIX, 7);
    set_map(0x60, RTS, 1, IMP, 6);
    set_map(0x61, ADC, 2, ZPIX, 6);
    set_map(0x65, ADC, 2, ZP, 3);
    set_map(0x66, ROR, 2, ZP, 5);
    set_map(0x68, PLA, 1, IMP, 4);
    set_map(0x69, ADC, 2, IMM, 2);
    set_map(0x6A, ROR, 1, ACC, 2);
    set_map(0x6C, JMP, 3, IND, 5);
    set_map(0x6D, ADC, 3, ABS, 4);
    set_map(0x6E, ROR, 3, ABS, 3);
    set_map(0x70, BVS, 2, R, 2);
    set_map(0x71, ADC, 2, ZPIY, 5);
    set_map(0x75, ADC, 2, ZPX, 4);
    set_map(0x76, ROR, 2, ZPX, 6);
    set_map(0x78, SEI, 1, IMP, 2);
    set_map(0x79, ADC, 3, AIY, 4);
    set_map(0x7D, ADC, 3, AIX, 4);
    set_map(0x7E, ROR, 3, AIX, 7);
    set_map(0x81, STA, 2, ZPIX, 6);
    set_map(0x84, STY, 2, ZP, 3);
    set_map(0x85, STA, 2, ZP, 3);
    set_map(0x86, STX, 2, ZP, 3);
    set_map(0x88, DEY, 1, IMP, 2);
    set_map(0x8A, TXA, 1, IMP, 2);
    set_map(0x8C, STY, 3, ABS, 4);
    set_map(0x8D, STA, 3, ABS, 4);
    set_map(0x8E, STX, 3, ABS, 4);
    set_map(0x90, BCC, 2, R, 2);
    set_map(0x91, STA, 2, ZPIY, 6);
    set_map(0x94, STY, 2, ZPX, 4);
    set_map(0x95, STA, 2, ZPX, 4);
    set_map(0x96, STX, 2, ZPY, 4);    
    set_map(0x98, TYA, 1, IMP, 2);
    set_map(0x99, STA, 3, AIY, 5);
    set_map(0x9A, TXS, 1, IMP, 2);
    set_map(0x9D, STA, 3, AIX, 5);
    set_map(0xA0, LDY, 2, IMM, 2);
    set_map(0xA1, LDA, 2, ZPIX, 6);
    set_map(0xA2, LDX, 2, IMM, 2);
    set_map(0xA4, LDY, 2, ZP, 3);
    set_map(0xA5, LDA, 2, ZP, 3);
    set_map(0xA6, LDX, 2, ZP, 3);
    set_map(0xA8, TAY, 1, IMP, 2);
    set_map(0xA9, LDA, 2, IMM, 2);
    set_map(0xAA, TAX, 1, IMP, 2);
    set_map(0xAC, LDY, 3, ABS, 4);
    set_map(0xAD, LDA, 3, ABS, 4);
    set_map(0xAE, LDX, 3, ABS, 4);
    set_map(0xB0, BCS, 2, R, 2);
    set_map(0xB1, LDA, 2, ZPIY, 5);
    set_map(0xB4, LDY, 2, ZPX, 4);
    set_map(0xB5, LDA, 2, ZPX, 4);
    set_map(0xB6, LDX, 2, ZPY, 4);
    set_map(0xB8, CLV, 1, IMP, 2);
    set_map(0xB9, LDA, 3, AIY, 4);
    set_map(0xBA, TSX, 1, IMP, 2);
    set_map(0xBC, LDY, 3, AIX, 4);
    set_map(0xBD, LDA, 3, AIX, 4);
    set_map(0xBE, LDX, 3, AIY, 4);
	set_map(0xC0, CPY, 2, IMM, 2);
    set_map(0xC1, CMP, 2, ZPIX, 6);
    set_map(0xC4, CPY, 2, ZP, 3);
    set_map(0xC5, CMP, 2, ZP, 3);
    set_map(0xC6, DEC, 2, ZP, 5);
    set_map(0xC8, INY, 1, IMP, 2);
    set_map(0xC9, CMP, 2, IMM, 2);
    set_map(0xCA, DEX, 1, IMP, 2);
    set_map(0xCC, CPY, 3, ABS, 4);
    set_map(0xCD, CMP, 3, ABS, 4);
    set_map(0xCE, DEC, 3, ABS, 6);
    set_map(0xD0, BNE, 2, R, 2);
    set_map(0xD1, CMP, 2, ZPIY, 5);
    set_map(0xD5, CMP, 2, ZPX, 4);
    set_map(0xD6, DEC, 2, ZPX, 6);
    set_map(0xD8, CLD, 1, IMP, 2);
    set_map(0xD9, CMP, 3, AIY, 4);
    set_map(0xDD, CMP, 3, AIX, 4);
    set_map(0xDE, DEC, 3, AIX, 7);
    set_map(0xE0, CPX, 2, IMM, 2);
    set_map(0xE1, SBC, 2, ZPIX, 6);
    set_map(0xE4, CPX, 2, ZP, 3);
    set_map(0xE5, SBC, 2, ZP, 3);
    set_map(0xE6, INC, 2, ZP, 5);
    set_map(0xE8, INX, 1, IMP, 2);
    set_map(0xE9, SBC, 2, IMM, 2);
    set_map(0xEC, CPX, 3, ABS, 4);
    set_map(0xED, SBC, 3, ABS, 4);
    set_map(0xEE, INC, 3, ABS, 6);
    set_map(0xEA, NOP, 1, IMP, 2);
    set_map(0xF0, BEQ, 2, R, 2);
    set_map(0xF1, SBC, 2, ZPIY, 5);
    set_map(0xF5, SBC, 2, ZPX, 4);
    set_map(0xF6, INC, 2, ZPX, 6);
    set_map(0xF8, SED, 1, IMP, 2);
    set_map(0xF9, SBC, 3, AIY, 4);
    set_map(0xFD, SBC, 3, AIX, 4);
    set_map(0xFE, INC, 3, AIX, 7);
}

