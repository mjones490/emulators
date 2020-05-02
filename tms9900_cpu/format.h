#ifndef __FORMAT_H
#define __FORMAT_H

struct operands_t {
    WORD    src;
    WORD    dest;
    char    disp;
    BYTE    cnt;
};

#define FMT_I       0
#define FMT_II      1
#define FMT_III     2
#define FMT_IV      3
#define FMT_V       4
#define FMT_VI      5
#define FMT_VII     6
#define FMT_VIII    7
#define FMT_IX      8
#define FMT_X       9

struct operands_t format_I(WORD code);
struct operands_t format_II(WORD code);
struct operands_t format_III(WORD code);
struct operands_t format_IV(WORD code);
struct operands_t format_V(WORD code);
struct operands_t format_VI(WORD code);
struct operands_t format_VIII(WORD code);

#endif
