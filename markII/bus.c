#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include "bus.h"
#include "logging.h"

const int PAGE_SIZE = 0x100;

struct page_block_t *page_block[0x100];

BYTE bus_accessor(WORD address, bool read, BYTE value)
{
//    printf("Page = %02x\n", hi(address));
    if (NULL != page_block[hi(address)] && 
        NULL != page_block[hi(address)]->accessor)
        value = page_block[hi(address)]->accessor(address, read, value);
    else
        value = 0x00;
    
    return value;
}

struct page_block_t *create_page_block(BYTE first_page, int total_pages)
{
    struct page_block_t *pb;
    LOG_INF("Creating page block starting at page %02x, ending at %02x. "
        "Total pages: %02x.\n", first_page, first_page + total_pages - 1,
        total_pages);
    pb = malloc(sizeof(struct page_block_t));
    if (NULL == pb)
        LOG_FTL("Unable to alocate page block.\n");

    memset(pb, 0, sizeof(struct page_block_t));
    pb->first_page = first_page;
    pb->total_pages = total_pages;
    return pb;
}

BYTE *create_page_buffer(int total_pages)
{
    BYTE *buffer;
    LOG_INF("Allocating %04x byte buffer.\n", total_pages * PAGE_SIZE);
    buffer = malloc(total_pages * PAGE_SIZE);
    memset(buffer, 0, total_pages * PAGE_SIZE);
    return buffer;
}
           
bool install_page_block(struct page_block_t *pb)
{
    int i;
    struct page_block_t *prev_pb = page_block[pb->first_page];

    if (NULL != prev_pb) {
        if (NULL != prev_pb->accessor && NULL == pb->accessor)
            pb->accessor = prev_pb->accessor;

        if (NULL != prev_pb->buffer && NULL == pb->buffer)
            pb->buffer = prev_pb->buffer + 
                ((pb->first_page - prev_pb->first_page) * PAGE_SIZE);
    }

    for (i = 0; i < pb->total_pages; ++i)
        page_block[i + pb->first_page] = pb;
    
    return true;
}

struct page_block_t *get_page_block(WORD address)
{
    return page_block[hi(address)];
}

struct page_block_t *soft_switch_pb;

struct soft_switch_t soft_switch[0x100];

BYTE soft_switch_accessor(WORD address, bool read, BYTE value)
{
    BYTE switch_no = address & 0x00FF;

    if (read && NULL != soft_switch[switch_no].read_accessor)
        value = soft_switch[switch_no].read_accessor(switch_no, read, value);
    else if (!read && NULL != soft_switch[switch_no].write_accessor)
        soft_switch[switch_no].write_accessor(switch_no, read, value);

    return value;
}

void install_soft_switch(BYTE switch_no, int switch_type, 
    soft_switch_accessor_t accessor)
{
    if (switch_type & SS_READ)
        soft_switch[switch_no].read_accessor = accessor;

    if (switch_type & SS_WRITE)
        soft_switch[switch_no].write_accessor = accessor;
}

void init_soft_switches()
{
    LOG_INF("Initializing soft switches.\n");
    memset(soft_switch, 0, sizeof(soft_switch));
    soft_switch_pb = create_page_block(0xC0, 1);
    soft_switch_pb->accessor = soft_switch_accessor;
    install_page_block(soft_switch_pb);

}

void init_bus()
{
    LOG_INF("Initializing bus.\n");
    LOG_DBG("Size of page_block is %p\n", sizeof(page_block));
    memset(page_block, 0, sizeof(page_block));
    init_soft_switches();
}

