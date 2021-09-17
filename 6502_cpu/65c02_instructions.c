#define M65C02
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "6502_cpu.h"
#include "instructions.h"

#define MNEMONIC_COUNT  (END - ADC)

#define MAKE_MNEMONIC_STRING(mnemonic) \
    #mnemonic,
     
void set_6502_instructions();

static const char *mode_format[] = {
    [IMM]  = "#$%02x",
    [ABS]  = "$%02x%02x",
    [AIX]  = "$%02x%02x,x",
    [AIY]  = "$%02x%02x,y",
    [ZP]   = "$%02x",
    [ZPX]  = "$%02x,x",
    [ZPY]  = "$%02x,y",
    [ZPIX] = "($%02x,x)",
    [ZPIY] = "($%02x),y",
    [R]    = "$%04x",
    [IND]  = "($%02x%02x)",
    [ACC]  = "",
    [IMP]  = "",
    [ABSX] = "(%02x%02x,x)",
    [ZPB] = "$%02x",
    [ZPR] = "$%02x,$%04x",
    [IZP] = "($%02x)"
};

void disasm_instr(WORD *address)
{
    int i;
    BYTE code = get_byte(*address);
    enum MNEMONIC mnemonic = cpu_get_mnemonic(code);
    const char *name = instruction_desc[mnemonic].name;
    enum ADDRESS_MODE mode = cpu_get_address_mode(code);
    int inst_size = cpu_get_instruction_size(code);
    BYTE opr[2];
    char ops[16];

    if (0 == name[0]) {
        mode = IMP;
        inst_size = 1;
    }

    printf("%04x:  %02x ", (*address)++, code);
    opr[0] = 0;
    opr[1] = 0;

    if (2 <= inst_size) {
        opr[0] = get_byte((*address)++);
        printf("%02x ", opr[0]);
        if (3 == inst_size) {
            opr[1] = get_byte((*address)++);
            printf("%02x ", opr[1]);
        } else {
            printf("   ");
        }
    } else {
        printf("      ");
    }

    if (0 == name[0])
        printf("???? ");
    else {
        for (i = 0; i < 3; ++i)
            printf("%c", name[i] == 0? ' ' : tolower(name[i]));
        if (mode == ZPB || mode == ZPR)
            printf("%d ", (code & 0x70) >> 4);
        else
            printf("  ");
    }

    ops[0] = 0;

    if (R == mode)
        sprintf(ops, mode_format[mode], (*address + 
            (WORD)((opr[0] & 0x80)? word(opr[0], 0xff) : opr[0])) & 0xffff);
    else if (ZPR == mode)
        sprintf(ops, mode_format[mode], opr[0], (*address + 
            (WORD)((opr[1] & 0x80)? word(opr[1], 0xff) : opr[1])) & 0xffff);
    else if (2 == inst_size)
        sprintf(ops, mode_format[mode], opr[0]);
    else if (3 == inst_size)
        sprintf(ops, mode_format[mode], opr[1], opr[0]);
    
    printf("%s", ops);
    for (i = strlen(ops); i < 11; ++i)
        printf(" ");
}

void set_instruction(enum MNEMONIC mnemonic, char* name, 
    void (*handler)(void))
{  
    strncpy(instruction_desc[mnemonic].name, name, 4);
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

enum MNEMONIC cpu_get_mnemonic(BYTE code)
{
    return instruction[code].mnemonic;
}

static inline void bit_branch(bool d)
{
    value = get_next_byte();
    address = regs.PC + ((value & 0x80)? word(value, 0xff) : value);
    if (d) {
        if (hi(address) != hi(regs.PC))
            ++running.clocks;
        regs.PC = address; 
    }
}

static inline BYTE bit_mask()
{
    return 0x01 << ((running.code & 0x70) >> 4);
}

BYTE indirect_zero_page(enum ACCESS_TYPE access, BYTE value)
{
    if (access != REPLACE)
        running.op_address = get_word((WORD) get_next_byte());
            
    return access_operand(access, value);
}

INSTRUCTION(PHY)
{
    push(regs.Y);
}

INSTRUCTION(PLY)
{
    regs.Y = pop();
    test_result(regs.Y);
}

INSTRUCTION(PHX)
{
    push(regs.X);
}

INSTRUCTION(PLX)
{
    regs.X = pop();
    test_result(regs.X);
}

INSTRUCTION(STZ)
{
    put_operand(0);
}

INSTRUCTION(BRA)
{
    branch(true);
}

INSTRUCTION(JMP)
{
    regs.PC = get_next_word();
    if (running.op_mode == IND)
        regs.PC = get_next_word();
    else if (running.op_mode == ABSX)
        regs.PC = get_word(regs.PC + regs.X);
}

INSTRUCTION(SMB)
{
    value = get_operand() | bit_mask();
    replace_operand(value);
}

INSTRUCTION(RMB)
{
    value = get_operand() & ~bit_mask();
    replace_operand(value);
}

INSTRUCTION(TSB)
{
    value = get_operand();
    toggle_flags(Z, (value & regs.A) > 0);
    replace_operand(value | regs.A);
}

INSTRUCTION(TRB)
{
    value = get_operand();
    toggle_flags(Z, (value & regs.A) > 0);
    replace_operand(value & ~regs.A);
}

INSTRUCTION(BBS)
{
    value = get_byte(get_next_byte());
    bit_branch((value & bit_mask()) > 0);
}

INSTRUCTION(BBR)
{
    value = get_byte(get_next_byte());
    bit_branch((value & bit_mask()) == 0);
}

static bool wait_state = false;
static bool stopped = false;
INSTRUCTION(WAI)
{
    regs.PC--;
    wait_state = true;
}

INSTRUCTION(STP)
{
    regs.PC--;
    stopped = true;
}

void interrupt(BYTE signal)
{
    if (wait_state) {
        regs.PC++;
        wait_state = false;
    }

    if (signal == SIG_RESET) {
        regs.SP -= 3;
        regs.PC = get_word(0xFFFC);
        stopped = false;
        set_flags(I);
        clear_flags(D);
        return;
    }  

    if (stopped)
        return;

    if (signal == SIG_NMI) {
        push(hi(regs.PC));
        push(lo(regs.PC));
        push(regs.PS);
        regs.PC = get_word(0xFFFA);
        set_flags(I);
        clear_flags(D);
        return;
    } 

    if ((!get_flags(I) || get_flags(B)) && signal == SIG_IRQ) {
        push(hi(regs.PC));
        push(lo(regs.PC));
        push(regs.PS);
        regs.PC = get_word(0xFFFE);
        set_flags(I);
        clear_flags(D);
        return;
    }
}

void init_instructions()
{
    int i;
    instruction_desc = malloc(sizeof(struct instruction_desc_t) * (MNEMONIC_COUNT+1));

    set_address_mode(ZPB, zero_page);
    set_address_mode(IZP, indirect_zero_page);

    set_6502_instructions();

    SET_INSTRUCTION(PHY);
    SET_INSTRUCTION(PLY);
    SET_INSTRUCTION(PHX);
    SET_INSTRUCTION(PLX);
    SET_INSTRUCTION(STZ);
    SET_INSTRUCTION(BRA);
    SET_INSTRUCTION(JMP);
    SET_INSTRUCTION(SMB);
    SET_INSTRUCTION(RMB);
    SET_INSTRUCTION(TSB);
    SET_INSTRUCTION(TRB);
    SET_INSTRUCTION(BBS);
    SET_INSTRUCTION(BBR);
    SET_INSTRUCTION(WAI);
    SET_INSTRUCTION(STP);

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

    set_map(0x5A, PHY, 1, IMP, 3);
    set_map(0x7A, PLY, 1, IMP, 4);
    set_map(0xDA, PHX, 1, IMP, 3);
    set_map(0xFA, PLX, 1, IMP, 4);
    set_map(0x64, STZ, 2, ZP, 3);
    set_map(0x74, STZ, 2, ZPX, 4);
    set_map(0x9C, STZ, 3, ABS, 4);
    set_map(0x9E, STZ, 3, AIX, 5);
    set_map(0x80, BRA, 2, R, 2);
    set_map(0x04, TSB, 2, ZP, 5);
    set_map(0x0C, TSB, 3, ABS, 6);
    set_map(0x14, TRB, 2, ZP, 5);
    set_map(0x1C, TRB, 3, ABS, 6);
    set_map(0xCB, WAI, 1, IMP, 3);
    set_map(0xDB, STP, 1, IMP, 3);
    
    set_map(0x7C, JMP, 3, ABSX, 6);
    set_map(0xB2, LDA, 2, IZP, 5);
    set_map(0x92, STA, 2, IZP, 5);
    set_map(0xD2, CMP, 2, IZP, 5);
    set_map(0x12, ORA, 2, IZP, 5);
    set_map(0x32, AND, 2, IZP, 5);
    set_map(0x52, EOR, 2, IZP, 5);
    set_map(0x72, ADC, 2, IZP, 5);
    set_map(0xF2, SBC, 2, IZP, 5);
    set_map(0x1A, INC, 1, ACC, 2);
    set_map(0x3A, DEC, 1, ACC, 2);

    for (i = 0; i < 8; i++) {
        int mask = i << 4;
        set_map(0x87 + mask, SMB, 2, ZPB, 5);
        set_map(0x07 + mask, RMB, 2, ZPB, 5);
        set_map(0x8f + mask, BBS, 3, ZPR, 5);
        set_map(0x0f + mask, BBR, 3, ZPR, 5);
    }
}

