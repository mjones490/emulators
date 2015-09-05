#include <stdlib.h>
#include <stdio.h>
#include "types.h"
#include "instructions.h"
#include "modes.h"

/*
inline void get_absoluteX_address()
{
    get_absolute_address();
    op_address += regs.X;
}

inline BYTE get_absoluteX()
{
    get_absoluteX_address();
    return get_byte(op_address);
}

inline void put_absoluteX(BYTE value)
{
    get_absoluteX_address();
    put_op(value);
}

inline void get_zp_address()
{
    op_address = (WORD) get_next_byte();
}

inline BYTE get_zp()
{
    get_zp_address();
    return get_byte(op_address);
}

inline void put_zp(BYTE value)
{
    get_zp_address();
    put_op(value);
}
*/
//---------------------------------------------
/*void test_cpu()
{
    WORD myword;
    BYTE mybyte;
    int i;

    printf("6502 CPU, by Mark\n");

    // sizeof check
    printf("sizeof(BYTE) = %d\n", sizeof(BYTE));
    printf("sizeof(WORD) = %d\n", sizeof(WORD));

    // word utils
    myword = word(0x02, 0x65);
    printf("myword = %04X\n", myword);
    printf("hi = %02X\n", hi(myword));
    printf("lo = %02X\n", lo(myword));

    // put and get bytes
    put_byte(0x0123, 0x42);
    mybyte = get_byte(0x0123);
    printf("Read byte %02X\n", mybyte);

    put_byte(0x124, 0x12);
    put_byte(0x125, 0x34);
    myword = get_word(0x124); 
    printf("Read word %04X\n", myword);

    // get next bytes and words
    put_byte(0x126, 0x56);

    regs.PC = 0x0123;
    for (i = 0; i < 4; ++i) {
        mybyte = get_next_byte();
        printf("Read byte %02X\n", mybyte);        
    }

    regs.PC = 0x0123;
    for (i = 0; i < 2; ++i) {
        myword = get_next_word();
        printf("Read word %04X\n", myword);        
    }

    // stack check
    regs.SP = 0xFF;
    push(0x01);
    push(0x02);
    push(0x03);
    for (i = 0; i < 3; ++i) {
        mybyte = pop();
        printf("Value popped = %02X\n", mybyte);
    }

    // address mode checks
    //init_address_modes();
    
    printf("\nTest IMM...\n");
    put_byte(0x300, 0x21);
    regs.PC = 0x300;
    mybyte = get_operand(IMM);
    printf("Got byte %02X\n", mybyte);        

    printf("\nTest ACC...\n");
    put_operand(ACC, 0x70);
    printf("Value of A = %02X\n", regs.A);
    mybyte = get_operand(ACC);  
    printf("Got byte %02X\n", mybyte);  
    replace_operand(mybyte >> 2);      
    printf("Value of A = %02X\n", regs.A);

    printf("\nTest ABS...\n");
    put_byte(0x301, 0x04);
    put_byte(0x300, 0x05); // 300 = 0405
    put_byte(0x303, 0x04); 
    put_byte(0x302, 0x10); // 302 = 0410
    put_byte(0x305, 0x04);
    put_byte(0x304, 0x15); // 304 = 0415   
    put_byte(0x405, 0xC5);
    put_byte(0x415, 0x64);
    regs.PC = 0x300;
    mybyte = get_operand(ABS);
    printf("Got byte %02X\n", mybyte);
    put_operand(ABS, mybyte + 1);
    mybyte = get_byte(0x410);
    printf("Read byte %02X\n", mybyte);
    mybyte = get_operand(ABS);
    printf("Got byte %02X\n", mybyte);
    replace_operand(mybyte + 5);
    mybyte = get_byte(0x415);
    printf("Read byte %02X\n", mybyte);
    
    printf("\nTest AIX...\n");
    put_byte(0x425, 0x88);
    put_byte(0x455, 0x20);     
    regs.PC = 0x300;
    regs.X = 0x20;
    mybyte = get_operand(AIX);
    printf("Got byte %02X\n", mybyte);
    regs.X = 0x30;
    put_operand(AIX, mybyte - 1);
    mybyte = get_byte(0x440);
    printf("Read byte %02X\n", mybyte);
    regs.X = 0x40;
    mybyte = get_operand(AIX);
    printf("Got byte %02X\n", mybyte);
    replace_operand(mybyte << 1);
    mybyte = get_byte(0x455);
    printf("Read byte %02X\n", mybyte);
            
    printf("\nTest AIY...\n");
    put_byte(0x429, 0x11); 
    put_byte(0x4D7, 0x44);     
    regs.Y = 0x24;
    mybyte = get_operand(AIX);
    printf("Got byte %02X\n", mybyte);
    regs.Y = 0x42;
    put_operand(AIX, mybyte + 9);
    mybyte = get_byte(0x452);
    printf("Read byte %02X\n", mybyte);
    regs.Y = 0xC2;
    mybyte = get_operand(AIX);
    printf("Got byte %02X\n", mybyte);
    replace_operand(mybyte << 1);
    mybyte = get_byte(0x4D7);
    printf("Read byte %02X\n", mybyte);
            
    printf("\nTest ZP...\n");
    put_byte(0x0010, 0x80);
    put_byte(0x0015, 0xAA);
    regs.PC = 0x302;
    mybyte = get_operand(ZP);
    printf("Got byte %02X\n", mybyte);
    put_operand(ZP, 0x33);
    mybyte = get_byte(0x04);
    printf("Read byte %02X\n", mybyte);
    mybyte = get_operand(ZP);
    printf("Got byte %02X\n", mybyte);
    replace_operand(~mybyte);
    mybyte = get_byte(0x15);
    printf("Read byte %02X\n", mybyte);
    
    printf("\nTest ZPX...\n");
    put_byte(0x0005, 0x57);
    regs.PC = 0x304;
    regs.X = 0xF0;
    mybyte = get_operand(ZPX);
    printf("Got byte %02X\n", mybyte);

    printf("\nTest ZPY...\n");
    regs.Y = 0x11;
    mybyte = get_operand(ZPY);
    printf("Got byte %02X\n", mybyte);
    
}*/
