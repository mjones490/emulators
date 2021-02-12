#ifndef __BUS_H
#define __BUS_H
#include "types.h"

BYTE ram_accessor(WORD address, bool read, BYTE value);
BYTE bus_accessor(WORD address, bool read, BYTE value);
void attach_bus(accessor_t accessor, BYTE start_page, int num_pages);
BYTE port_accessor(BYTE port_no, bool read, BYTE value);
void attach_port(port_accessor_t accessor, BYTE port_no);
void init_bus();
void finalize_bus();

#endif
