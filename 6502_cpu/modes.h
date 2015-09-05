#ifndef __MODE_H
#define __MODE_H
#include "6502_cpu_enums.h"
#include "cpu_types.h"

enum ACCESS_TYPE {
    GET,
    PUT,
    REPLACE
};

typedef BYTE (*mode_accessor_t)(enum ACCESS_TYPE, BYTE);
extern mode_accessor_t mode_accessor[16];

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
