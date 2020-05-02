#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "shell.h"
#include "cpu_types.h"
#include "format.h"

static inline WORD get_address(BYTE t, BYTE r, bool byte)
{
    WORD address;

    if (t == 0) {
        address = get_register_address(r);
    } else if (t == 1 || t == 3) {
        address = get_register_value(r);
        if (t == 3) 
            set_register_value(r, address + (byte? 1 : 2));
    } else {
        address = get_next_word();
        if (r > 0)
            address += get_register_value(r);
    }

    return address;
}

struct operands_t format_I(WORD code)
{
    struct operands_t ops;
    memset(&ops, 0, sizeof(ops));

    bool byte = (code >> 12) & 0x0001;
    BYTE td = (code >> 10) & 0x0003;
    BYTE rd = (code >> 6) & 0x000f;
    BYTE ts = (code >> 4) & 0x0003;
    BYTE rs = code & 0x000f;

    ops.src = get_address(ts, rs, byte);
    ops.dest = get_address(td, rd, byte);

    return ops;
}

struct operands_t format_II(WORD code)
{
    struct operands_t ops;
    memset(&ops, 0, sizeof(ops));

    ops.disp = code & 0xff;

    return ops;
}

struct operands_t format_III(WORD code)
{
    struct operands_t ops;
    memset(&ops, 0, sizeof(ops));

    BYTE rd = (code >> 6) & 0x0f;
    BYTE ts = (code >> 4) & 0x03;
    BYTE rs = code & 0x0f;

    ops.dest = get_register_address(rd);
    ops.src = get_address(ts, rs, false);

    return ops;
}

struct operands_t format_IV(WORD code)
{
    struct operands_t ops;
    memset(&ops, 0, sizeof(ops));

    BYTE ts = (code >> 4) & 0x03;
    BYTE rs = code & 0x0f;

    ops.cnt = (code >> 6) & 0x0f;
    ops.src = get_address(ts, rs, false);

    return ops;
}

struct operands_t format_V(WORD code)
{
    struct operands_t ops;
    memset(&ops, 0, sizeof(ops));

    BYTE rs = code & 0x0f;

    ops.cnt = (code >> 4) & 0x0f;
    ops.src = get_register_address(rs);

    return ops;
}

struct operands_t format_VI(WORD code)
{
    struct operands_t ops;
    memset(&ops, 0, sizeof(ops));

    BYTE ts = (code >> 4) & 0x03;
    BYTE rs = code & 0x0f;

    ops.src = get_address(ts, rs, false);

    return ops;
}

struct operands_t format_VIII(WORD code)
{
    struct operands_t ops;
    memset(&ops, 0, sizeof(ops));

    BYTE rd = code & 0x0f;

    ops.dest = get_register_address(rd);

    return ops;
}

