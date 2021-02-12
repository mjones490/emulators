#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <malloc.h>
#include "dynosaur.h"
#include "logging.h"

BYTE *ram;

accessor_t bus[256];
port_accessor_t port[256];

BYTE ram_accessor(WORD address, bool read, BYTE value)
{
    if (read)
        value = ram[address];
    else
        ram[address] = value;

    return value;
}

BYTE bus_accessor(WORD address, bool read, BYTE value)
{
    if (bus[hi(address)])
        return bus[hi(address)](address, read, value);

    return 0;
}

void attach_bus(accessor_t accessor, BYTE start_page, int num_pages)
{
    WORD i;
    LOG_DBG("Attaching %d pages to bus at %02x.\n", num_pages, start_page);
    for (i = 0; i < num_pages; i++) {
        if ((i + start_page) > 255)
            break;
        bus[i + start_page] = accessor;
    }
}

BYTE port_accessor(BYTE port_no, bool read, BYTE value)
{
    if (port[port_no])
        return port[port_no](port_no, read, value);

    LOG_DBG("Port %02x not attached.\n", port_no);
    return 0;
}

void attach_port(port_accessor_t accessor, BYTE port_no)
{
    port[port_no] = accessor;
}

void init_bus()
{
    if (dyn_config.ram_size == 0)
        dyn_config.ram_size = 256;
    LOG_INF("Inititializing bus.\n");
    LOG_INF("Allocating %d bytes of RAM.\n", 256 * dyn_config.ram_size);
    ram = malloc(256 * dyn_config.ram_size);
    memset(ram, 0xff, 256 * dyn_config.ram_size);
    memset(bus, 0x00, sizeof(accessor_t) * 256);
    memset(port, 0, sizeof(port_accessor_t) * 256);
    attach_bus(ram_accessor, 0, dyn_config.ram_size);
}

void finalize_bus()
{
    LOG_INF("Finalizing bus.\n");
    free(ram);
}
