#ifndef __TYPES_H
#define __TYPES_H

// Types
#include <stdbool.h>

typedef unsigned short  WORD;
typedef unsigned char   BYTE;
typedef unsigned int    DWORD;

// Bus access
typedef BYTE (*accessor_t)(WORD address, bool read, BYTE value);

// Port access
typedef BYTE (*port_accessor_t)(BYTE port, bool read, BYTE value);

static inline WORD word(BYTE lo, BYTE hi)
{
    return hi << 8 | lo;
}

static inline BYTE hi(WORD word)
{
    return word >> 8;
}

static inline BYTE lo(WORD word)
{
    return word & 0xff;
}

static inline bool msb(WORD op)
{
    return (op & 0x8000) != 0;
}

#endif // __TYPES_H
