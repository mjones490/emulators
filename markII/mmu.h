#ifndef __ROM_H
#define __ROM_H

#include <types.h>

BYTE *load_ROM(char *ROM_name, BYTE requested_pages);
void init_mmu();

#endif // __ROM_H
