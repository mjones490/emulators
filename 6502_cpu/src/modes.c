#include "modes.h"
#include "cpu_types.h"

BYTE immediate(enum ACCESS_TYPE access, BYTE value)
{
    return get_next_byte();
}

BYTE accumulator(enum ACCESS_TYPE access, BYTE value)
{
    if (access == GET)
        value = regs.A;
    else
        regs.A = value;

   return value;
}

BYTE access_operand(enum ACCESS_TYPE access, BYTE value)
{
    if (access == GET)
        value = get_byte(running.op_address);
    else
        put_byte(running.op_address, value);

    return value;
}

BYTE absolute(enum ACCESS_TYPE access, BYTE value)
{
    if (access != REPLACE)
       running.op_address = get_next_word();       
    return access_operand(access, value);
}

/**
 * @brief Indicates when base address plus an offset is in a different page 
 * space than the base address alone.
 * @param[in] base Base address
 * @param[in] offset Offset from base address
 * @return True if offset is in a different page
 */
static inline WORD page_cross(WORD base, BYTE offset) 
{
    if (((base + offset) & 0xFF00) != (base & 0xFF00))
        ++running.clocks;
    return base + offset;
}        


BYTE absolute_X(enum ACCESS_TYPE access, BYTE value)
{
    if (access != REPLACE) 
       running.op_address = page_cross(get_next_word(), regs.X); 
    return access_operand(access, value);            
}

BYTE absolute_Y(enum ACCESS_TYPE access, BYTE value)
{
    if (access != REPLACE) 
       running.op_address = page_cross(get_next_word(), regs.Y); 
    return access_operand(access, value);            
}

BYTE zero_page(enum ACCESS_TYPE access, BYTE value)
{
    if (access != REPLACE) 
        running.op_address = (WORD) get_next_byte();
    return access_operand(access, value);
}

BYTE zero_page_X(enum ACCESS_TYPE access, BYTE value)
{
    if (access != REPLACE) 
        running.op_address = ((WORD) get_next_byte() + regs.X) & 0xFF;
    return access_operand(access, value);
}

BYTE zero_page_Y(enum ACCESS_TYPE access, BYTE value)
{
    if (access != REPLACE) 
        running.op_address = ((WORD) get_next_byte() + regs.Y) & 0xFF;
    return access_operand(access, value);
}

BYTE zero_page_indirect_Y(enum ACCESS_TYPE access, BYTE value)
{
    if (access != REPLACE)
        running.op_address = page_cross(get_word((WORD) get_next_byte()),
            regs.Y);
    return access_operand(access, value);
}

BYTE zero_page_indirect_X(enum ACCESS_TYPE access, BYTE value)
{
    if (access != REPLACE)
        running.op_address = get_word(((WORD) get_next_byte() + regs.X) & 0xFF); 
    return access_operand(access, value);
}

typedef BYTE (*mode_accessor_t)(enum ACCESS_TYPE, BYTE);

mode_accessor_t mode_accessor[20];

void set_address_mode(enum ADDRESS_MODE mode, mode_accessor_t accessor)
{
    mode_accessor[mode] = accessor;
}

