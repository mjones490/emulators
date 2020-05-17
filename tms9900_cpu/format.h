#ifndef __FORMAT_H
#define __FORMAT_H

struct operands_t {
    WORD    src;
    WORD    dest;
    char    disp;
    BYTE    cnt;
    BYTE    vector;
};

#define FMT_I       0 // opcode b td d ts s (mov r0, @>1000)
#define FMT_II      1   // opcode disp (jmp $+10)
#define FMT_III     2   // opcode d ts s (xor @>1000, r1)
#define FMT_IV      3   // opcode c ts s (ldcr *r6+,8) 
#define FMT_V       4   // opcode c w (sla r5,4)
#define FMT_VI      5   // opcode ts s (inc r7)
#define FMT_VII     6   // opcode (rtwp) (no format function)
#define FMT_VIII    7   // opcode w (li r1,>2000)
#define FMT_IX      8   // opcode disp ts s (xop @>1000(r4),12) 

#define FMT_X       9   // opcode disp (sbo 5) (use format_II)
#define FMT_XI      10  // opcode  (lwpi >0480) (no format instruction)
#define FMT_XII     11  // opcode r (stst r0) (use format_VIII)

struct operands_t format_I(WORD code);
struct operands_t format_II(WORD code);
struct operands_t format_III(WORD code);
struct operands_t format_IV(WORD code);
struct operands_t format_V(WORD code);
struct operands_t format_VI(WORD code);
struct operands_t format_VIII(WORD code);
struct operands_t format_IX(WORD code);

#endif
