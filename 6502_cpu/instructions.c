#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "instructions.h"
#include "modes.h"

struct instruction_desc_t *instruction_desc;
struct instruction_t instruction[256];

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

     
void set_6502_instructions()
{
       
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
}

