#ifndef __ENUMS_H
#define __ENUMS_H

/** @brief 6502 Addressing modes
 */
enum ADDRESS_MODE {
    ACC,    ///< Accumulator
    IMM,    ///< Immediate
    IMP,    ///< Implied
    IND,    ///< Indirect
    ABS,    ///< Absolute
    ZP,     ///< Zero Page
    R,      ///< Relative
    AIX,    ///< Absolute X Indexed
    AIY,    ///< Absolute Y Indexed
    ZPX,    ///< Zero Page X Indexed
    ZPY,    ///< Zero Page Y Indexed
    ZPIX,   ///< Zero Page Indexed Indirect
    ZPIY,   ///< Zero Page Indirect Indexed
#ifdef M65C02
    ABSX,   ///< Absolute X
    ZPB,    ///< Zero Page Bit
    ZPR,    ///< Zero Page,Relative
    IZP     ///< Indirect Zero Page
#endif
};

enum MNEMONIC {
    ADC,    ///< Add with Carry
    AND,    ///< Logical AND
    ASL,    ///< Arithmetic Shift Left
    BCC,    ///< Branch if Carry Clear
    BCS,    ///< Branch if Carry Set
    BEQ,    ///< Branch if Equal
    BIT,    ///< Bit Test
    BMI,    ///< Branch if Minus
    BNE,    ///< Branch if Not Equal
    BPL,    ///< Branch if Positive
    BRK,    ///< Force Interrupt
    BVC,    ///< Branch if Overflow Clear
    BVS,    ///< Branch if Overflow Set
    CLC,    ///< Clear Carry Flag
    CLD,    ///< Clear Decimal Mode
    CLI,    ///< Clear Interupt Disable    
    CLV,    ///< Clear Overflow Flag
    CMP,    ///< Compare
    CPX,    ///< Compare X
	CPY,    ///< Compare Y
    DEC,    ///< Decrement
    DEX,    ///< Decrement X
    DEY,    ///< Decrement Y
    EOR,    ///< Exclusive OR
    INC,    ///< Increment
    INX,    ///< Increment X
    INY,    ///< Increment Y
    JMP,    ///< Unconditional Jump
    JSR,    ///< Jump to Subroutine
    LDA,    ///< Load Accumulator
    LDX,    ///< Load X
    LDY,    ///< Load Y
    LSR,    ///< Logical Shift Right
    NOP,    ///< No Operation
    ORA,    ///< OR Accumulator
    PHA,    ///< Push Accumulator
    PHP,    ///< Push Processor Status
    PLA,    ///< Pull Accumulator
    PLP,    ///< Pull Processor Status
    ROL,    ///< Rotate Left
    ROR,    ///< Rodate Right
    RTI,    ///< Return from Interrupt
    RTS,    ///< Return from Subroutine
    SBC,    ///< Subtract with Borrow
	SEC,    ///< Set Carry Flag
    SED,    ///< Set Decimal Mode
    SEI,    ///< Set Interrupt Disable
    STA,    ///< Store Accumulator
    STX,    ///< Store X
    STY,    ///< Store Y
    TAX,    ///< Transfer Accumulator to X
    TAY,    ///< Transfer Accumulator to Y
    TXA,    ///< Transfer X to Accumulator
    TYA,    ///< Transfer Y to Accumualtor
    TSX,    ///< Transfer Stack Pointer to X
    TXS,    ///< Transfer X to Stack Pointer
#ifdef M65C02
    PHX,
    PLX,
    PHY,
    PLY,
    STZ,
    BRA,
    SMB,
    RMB,
    TSB,
    TRB,
    BBS,
    BBR,
#endif
    END
};

#define SIG_NONE    0x00
#define SIG_IRQ     0x01
#define SIG_NMI     0x02
#define SIG_CLI     0x04
#define SIG_SEI     0x08
#define SIG_RESET   0x10
#define SIG_HALT    0x20

#endif
