#ifndef __SLOT_H
#define __SLOT_H

void init_slot();
void install_slot_ROM(BYTE slot_no, BYTE *slot_ROM, BYTE *expansion_ROM);
struct soft_switch_t *install_slot_switch(int switch_no, int switch_type,
   soft_switch_accessor_t accessor);
void finalize_slot();

#endif // __SLOT_H
