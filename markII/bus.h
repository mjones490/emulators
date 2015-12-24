#ifndef __BUS_H
#define __BUS_H
#include <stdlib.h>
#include <6502_cpu.h>

struct page_block_t {
    accessor_t accessor;
    BYTE first_page;
    int total_pages;
    BYTE *buffer;
    void *data;
};

typedef BYTE (*soft_switch_accessor_t)(BYTE switch_no, bool read, BYTE value);

struct soft_switch_t
{
    soft_switch_accessor_t read_accessor;
    soft_switch_accessor_t write_accessor; 
    void                   *data;
};

#define SS_READ  1
#define SS_WRITE 2
#define SS_RDWR  3

BYTE bus_accessor(WORD address, bool read, BYTE value);
struct page_block_t *create_page_block(BYTE first_page, int total_pages);
BYTE *create_page_buffer(int total_pages);
void free_page_buffers();
bool install_page_block(struct page_block_t *pb);
void init_bus();
void set_soft_switch_data(BYTE soft_switch_no, void *data);
void *get_soft_switch_data(BYTE soft_switch_no);
void install_soft_switch(BYTE switch_no, int switch_type, 
    soft_switch_accessor_t accessor);
struct page_block_t *get_page_block(WORD address);

  
static inline WORD pb_offset(struct page_block_t *pb, WORD address)
{
    return address - word(0, pb->first_page);
}
           
#endif
