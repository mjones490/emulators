#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "shell.h"
#include "6502_cpu.h"
#include "cpu_types.h"
#include "cpu_types.h"

/**********************************************************
    Disassemble command
 **********************************************************/

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
    [IMP]  = ""
};

#define MAKE_MNEMONIC_STRING(mnemonic) \
    #mnemonic,
     
static const char *mnemonic_string[] = {
    MAKE_MNEMONIC_STRING(ADC)    ///< Add with Carry
    MAKE_MNEMONIC_STRING(AND)    ///< Logical AND
    MAKE_MNEMONIC_STRING(ASL)    ///< Arithmetic Shift Left
    MAKE_MNEMONIC_STRING(BCC)    ///< Branch if Carry Clear
    MAKE_MNEMONIC_STRING(BCS)    ///< Branch if Carry Set
    MAKE_MNEMONIC_STRING(BEQ)    ///< Branch if Equal
    MAKE_MNEMONIC_STRING(BIT)    ///< Bit Test
    MAKE_MNEMONIC_STRING(BMI)    ///< Branch if Minus
    MAKE_MNEMONIC_STRING(BNE)    ///< Branch if Not Equal
    MAKE_MNEMONIC_STRING(BPL)    ///< Branch if Positive
    MAKE_MNEMONIC_STRING(BRK)    ///< Force Interrupt
    MAKE_MNEMONIC_STRING(BVC)    ///< Branch if Overflow Clear
    MAKE_MNEMONIC_STRING(BVS)    ///< Branch if Overflow Set
    MAKE_MNEMONIC_STRING(CLC)    ///< Clear Carry Flag
    MAKE_MNEMONIC_STRING(CLD)    ///< Clear Decimal Mode
    MAKE_MNEMONIC_STRING(CLI)    ///< Clear Interupt Disable    
    MAKE_MNEMONIC_STRING(CLV)    ///< Clear Overflow Flag
    MAKE_MNEMONIC_STRING(CMP)    ///< Compare
    MAKE_MNEMONIC_STRING(CPX)    ///< Compare X
    MAKE_MNEMONIC_STRING(CPY)    ///< Compare Y
    MAKE_MNEMONIC_STRING(DEC)    ///< Decrement
    MAKE_MNEMONIC_STRING(DEX)    ///< Decrement X
    MAKE_MNEMONIC_STRING(DEY)    ///< Decrement Y
    MAKE_MNEMONIC_STRING(EOR)    ///< Exclusive OR
    MAKE_MNEMONIC_STRING(INC)    ///< Increment
    MAKE_MNEMONIC_STRING(INX)    ///< Increment X
    MAKE_MNEMONIC_STRING(INY)    ///< Increment Y
    MAKE_MNEMONIC_STRING(JMP)    ///< Unconditional Jump
    MAKE_MNEMONIC_STRING(JSR)    ///< Jump to Subroutine
    MAKE_MNEMONIC_STRING(LDA)    ///< Load Accumulator
    MAKE_MNEMONIC_STRING(LDX)    ///< Load X
    MAKE_MNEMONIC_STRING(LDY)    ///< Load Y
    MAKE_MNEMONIC_STRING(LSR)    ///< Logical Shift Right
    MAKE_MNEMONIC_STRING(NOP)    ///< No Operation
    MAKE_MNEMONIC_STRING(ORA)    ///< OR Accumulator
    MAKE_MNEMONIC_STRING(PHA)    ///< Push Accumulator
    MAKE_MNEMONIC_STRING(PHP)    ///< Push Processor Status
    MAKE_MNEMONIC_STRING(PLA)    ///< Pull Accumulator
    MAKE_MNEMONIC_STRING(PLP)    ///< Pull Processor Status
    MAKE_MNEMONIC_STRING(ROL)    ///< Rotate Left
    MAKE_MNEMONIC_STRING(ROR)    ///< Rodate Right
    MAKE_MNEMONIC_STRING(RTI)    ///< Return from Interrupt
    MAKE_MNEMONIC_STRING(RTS)    ///< Return from Subroutine
    MAKE_MNEMONIC_STRING(SBC)    ///< Subtract with Borrow
    MAKE_MNEMONIC_STRING(SEC)    ///< Set Carry Flag
    MAKE_MNEMONIC_STRING(SED)    ///< Set Decimal Mode
    MAKE_MNEMONIC_STRING(SEI)    ///< Set Interrupt Disable
    MAKE_MNEMONIC_STRING(STA)    ///< Store Accumulator
    MAKE_MNEMONIC_STRING(STX)    ///< Store X
    MAKE_MNEMONIC_STRING(STY)    ///< Store Y
    MAKE_MNEMONIC_STRING(TAX)    ///< Transfer Accumulator to X
    MAKE_MNEMONIC_STRING(TAY)    ///< Transfer Accumulator to Y
    MAKE_MNEMONIC_STRING(TXA)    ///< Transfer X to Accumulator
    MAKE_MNEMONIC_STRING(TYA)    ///< Transfer Y to Accumualtor
    MAKE_MNEMONIC_STRING(TSX)    ///< Transfer Stack Pointer to X
    MAKE_MNEMONIC_STRING(TXS)    ///< Transfer X to Stack Pointer
};

void disasm_instr(WORD *address)
{
    int i;
    BYTE code = get_byte(*address);
    enum MNEMONIC mnemonic = cpu_get_mnemonic(code);
    const char *name = mnemonic_string[mnemonic];
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
        printf("???");
    else
        for (i = 0; i < 3; ++i)
            printf("%c", tolower(name[i]));
    printf(" ");
    
    ops[0] = 0;

    if (R == mode)
        sprintf(ops, mode_format[mode], (*address + 
            (WORD)((opr[0] & 0x80)? word(opr[0], 0xff) : opr[0])) & 0xffff);
    else if (2 == inst_size)
        sprintf(ops, mode_format[mode], opr[0]);
    else if (3 == inst_size)
        sprintf(ops, mode_format[mode], opr[1], opr[0]);
    
    printf("%s", ops);
    for (i = strlen(ops); i < 11; ++i)
        printf(" ");
}

static int disassemble(int argc, char **argv)
{
    int matched;
    int start;
    int end;
    static WORD address;
    static WORD extend = 1;

    if (1 < argc) { 
        matched = sscanf(argv[1], "%4x-%4x", &start, &end);
        if (matched) {
            address = start;
            if (matched == 1) { 
                end = address;
                extend = 1;
            } else {
                extend = end - address;
            }
        }
    } else {
        end = address + extend;
    }

    do {
        disasm_instr(&address);
        printf("\n");
    } while (address < end);

    return 0;
}

/**********************************************************
   Registers command
 **********************************************************/
void show_registers()
{
    printf("a = %02x ", cpu_get_reg_A());
    printf("x = %02x ", cpu_get_reg_X());
    printf("y = %02x ", cpu_get_reg_Y());
    printf("sp = %02x ", cpu_get_reg_SP());
    printf("pc = %04x ", cpu_get_reg_PC());
    printf("ps = %02x ", cpu_get_reg_PS());
    printf("\n");
}

void set_register(char *reg_name, WORD value)
{
    if (0 == strncmp(reg_name, "a", 1))
        cpu_set_reg_A(lo(value));
    else if (0 == strncmp(reg_name, "x", 1))
        cpu_set_reg_X(lo(value));
    else if (0 == strncmp(reg_name, "y", 1))
        cpu_set_reg_Y(lo(value));
    else if (0 == strncmp(reg_name, "sp", 1))
        cpu_set_reg_SP( lo(value));
    else if (0 == strncmp(reg_name, "pc", 2))
        cpu_set_reg_PC(value);
}

static int registers(int argc, char **argv)
{
    int tmp;
    char *reg_name;

    if (argc == 3) {
        reg_name = argv[1];
        if (1 == sscanf(argv[2], "%4x", &tmp))
            set_register(reg_name, (WORD) tmp);    
    }

    show_registers();
    return 0;
}

/**********************************************************
   Step command
 **********************************************************/
static int step(int argc, char **argv)
{
    WORD address;
    int clocks;
    clocks = cpu_execute_instruction();
    address = cpu_get_prev_PC();
    disasm_instr(&address);
    printf("\t; ");
    if (0 == clocks)
        printf("Bad instruction\n");
    else
        printf("%d clocks\n", clocks);
    show_registers();
    return 0;
}

static int go(int argc, char **argv)
{
    int tmp;

    if (1 < argc) {
        if (1 == sscanf(argv[1], "%4x", &tmp))
            cpu_set_reg_PC((WORD) tmp);
    }

    cpu_clear_signal(SIG_HALT);
    return 0;        
}

static int assert_interrupt(int argc, char **argv)
{
    if (0 == strncmp(argv[0], "nmi", 4))
        cpu_toggle_signal(SIG_NMI);
    else if (0 == strncmp(argv[0], "irq", 4))
        cpu_toggle_signal(SIG_IRQ);
    else if (0 == strncmp(argv[0], "reset", 6))
        cpu_toggle_signal(SIG_RESET);
    else if (0 == strncmp(argv[0], "halt", 5))
        cpu_toggle_signal(SIG_HALT);
    return 0;
}

void cpu_shell_load_commands()
{
    shell_add_command("registers", "View/change 6502 register.", 
        registers, false);
    shell_add_command("disassemble", "Disassemble code.", disassemble, true);
    shell_add_command("step", "Step through code.", step, true);
    shell_add_command("go", "Start program.", go, false);
    shell_add_command("nmi", "Assert NMI.", assert_interrupt, false);
    shell_add_command("irq", "Assert IRQ.", assert_interrupt, false);
    shell_add_command("reset", "Reset CPU.", assert_interrupt, false);
    shell_add_command("halt", "Halt CPU", assert_interrupt, false);
}

