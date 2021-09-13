#ifndef __MODE_H
#define __MODE_H
#include "6502_cpu_enums.h"
#include "cpu_types.h"

enum ACCESS_TYPE {
    GET,
    PUT,
    REPLACE
};

BYTE immediate(enum ACCESS_TYPE access, BYTE value);
BYTE accumulator(enum ACCESS_TYPE access, BYTE value);
BYTE absolute(enum ACCESS_TYPE access, BYTE value);
BYTE absolute_X(enum ACCESS_TYPE access, BYTE value);
BYTE absolute_Y(enum ACCESS_TYPE access, BYTE value);
BYTE zero_page(enum ACCESS_TYPE access, BYTE value);
BYTE zero_page_X(enum ACCESS_TYPE access, BYTE value);
BYTE zero_page_Y(enum ACCESS_TYPE access, BYTE value);
BYTE zero_page_indirect_X(enum ACCESS_TYPE access, BYTE value);
BYTE zero_page_indirect_Y(enum ACCESS_TYPE access, BYTE value);

typedef BYTE (*mode_accessor_t)(enum ACCESS_TYPE, BYTE);
extern mode_accessor_t mode_accessor[20];

BYTE access_operand(enum ACCESS_TYPE access, BYTE value);
void set_address_mode(enum ADDRESS_MODE mode, mode_accessor_t accessor);

static inline BYTE get_operand()
{
    return mode_accessor[running.op_mode](GET, 0);
}

static inline void put_operand(BYTE value)
{
    mode_accessor[running.op_mode](PUT, value);
}

static inline void replace_operand(BYTE value)
{
    mode_accessor[running.op_mode](REPLACE, value);
}


#endif
