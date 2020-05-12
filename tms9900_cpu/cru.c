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

struct accessor_t {
    cru_accessor_t handler;
    struct accessor_t *next;
};

static struct accessor_t *first;

void set_cru_accessor(cru_accessor_t handler)
{
    struct accessor_t *next = malloc(sizeof(struct accessor_t));
    next->next = first;
    next->handler = handler;
    first = next;
}

WORD cru_accessor(WORD soft_address, bool read, WORD value, BYTE size)
{
    struct accessor_t *next = first;
    while (next) {
        value = next->handler(soft_address, read, value, size);
        next = next->next;
    }

    return value;
}

