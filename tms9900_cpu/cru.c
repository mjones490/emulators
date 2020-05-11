#include <stdlib.h>
#include "shell.h"
#include "cru.h"

static inline WORD shift_word(WORD value, int shift_cnt)
{
    if (shift_cnt < 0)
        value >>= -shift_cnt;
    else if (shift_cnt > 0)
        value <<= shift_cnt;

    return value;
}   

struct hw_val_t to_hardware(WORD soft_address, WORD value, 
    BYTE size, WORD hard_address)
{
    struct hw_val_t hw_value;

    size &= 0x0f;
    if (size == 0)
        size = 16;

    hw_value.bitmask = shift_word((1 << size) - 1, soft_address - hard_address);
    hw_value.value = shift_word(value, soft_address - hard_address) & hw_value.bitmask;

    return hw_value;
}

WORD to_software(WORD hard_address, WORD value, 
    BYTE size, WORD soft_address)
{
    WORD bitmask;

    size &= 0x0f;
    if (size == 0)
        size = 16;

    bitmask = shift_word((1 << size) - 1, soft_address - hard_address);
    return shift_word(value & bitmask, -(soft_address - hard_address));
}

cru_accessor_t first_accessor = NULL;

cru_accessor_t set_cru_accessor(cru_accessor_t next_accessor)
{
    cru_accessor_t last_accessor = first_accessor;
    first_accessor = next_accessor;
    return last_accessor;
}

WORD cru_accessor(WORD soft_address, bool read, WORD value, BYTE size)
{
    if (first_accessor != NULL)
        value = first_accessor(soft_address, read, value, size);
    return value;
}

