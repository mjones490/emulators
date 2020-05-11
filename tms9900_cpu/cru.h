#ifndef __CRU_H
#define __CRU_H

struct hw_val_t {
    WORD value;
    WORD bitmask;
};

struct hw_val_t to_hardware(WORD soft_address, WORD value, 
    BYTE size, WORD hard_address);

WORD to_software(WORD hard_address, WORD value, 
    BYTE size, WORD soft_address);

typedef WORD (*cru_accessor_t)(WORD soft_address, bool read, WORD value, BYTE size);
WORD cru_accessor(WORD soft_address, bool read, WORD value, BYTE size);

cru_accessor_t set_cru_accessor(cru_accessor_t next_accessor);
#endif
