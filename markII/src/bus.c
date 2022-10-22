/**
 * @file bus.c
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <Stuff.h>
#include "bus.h"
#include "logging.h"

const int PAGE_SIZE = 0x100; ///< Size of one page.  256 bytes

struct page_block_t *page_block[0x100]; ///< Main array of page blocks

/**
 * Main bus accessor.  Determines if page block is installed, and forwards
 * the access to that page block if it is.
 * @param[in] address Address to be accessed
 * @param[in] read Read address if true.  Write if not.
 * @param[in] value Value to write
 * @returns Value read.
 */
BYTE bus_accessor(WORD address, bool read, BYTE value)
{
    if (NULL != page_block[hi(address)] && 
        NULL != page_block[hi(address)]->accessor)
        value = page_block[hi(address)]->accessor(address, read, value);
    else
        value = 0x00;
    
    return value;
}

struct list_head page_buffers; ///< List of page buffers

/**
 * Create and set up page block.
 * @param[in] first_page First page number of new page block.
 * @param[in] total_pages Number of pages in block.
 * @returns New page block.
 */
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
    list_add(&pb->list, &page_buffers);
    return pb;
}

struct buffer_list_t {
    BYTE *buffer;           ///< Page buffer
    struct list_head list;  ///< Buffer list.
};

void destroy_page_block(struct page_block_t *pb)
{
    list_remove(&pb->list);
    free(pb);
}

void free_page_block_list()
{
    struct page_block_t *pb;
    struct list_head *current;
    struct list_head *next;

    LOG_INF("Freeing page blocks.\n");
    LIST_FOR_EACH_SAFE(current, next, &page_buffers) {
        pb = GET_ELEMENT(struct page_block_t, list, current);
        destroy_page_block(pb);
    }
}

struct list_head buffers; ///< List of allocated buffers

/**
 * Creates a buffer of pages for use by RAM and ROM routines.  Adds it
 * to the buffer list.
 * @param[in] total_pages Number of pages to allocate.
 * @returns Allocated buffer.
 */
BYTE *create_page_buffer(int total_pages)
{
    struct buffer_list_t *new_list;
    LOG_INF("Allocating %04x byte buffer.\n", total_pages * PAGE_SIZE);

    new_list = malloc(sizeof(struct buffer_list_t));
    new_list->buffer = malloc(total_pages * PAGE_SIZE);
    LOG_DBG("Buffer allocated at %p\n", new_list->buffer);
    memset(new_list->buffer, 0, total_pages * PAGE_SIZE);
    
    list_add_tail(&new_list->list, &buffers);
    return new_list->buffer;
}

/**
 * Walks buffer list and frees each buffer. 
 */
void free_page_buffers()
{   
    struct list_head *current;
    struct list_head *next;
    struct buffer_list_t *item;

    LOG_INF("Freeing buffers..\n");
    LIST_FOR_EACH_SAFE(current, next, &buffers) {
        item = GET_ELEMENT(struct buffer_list_t, list, current);    
        LOG_DBG("Freeing buffer at %p\n", item->buffer);
        list_remove(current); 
        free(item->buffer);
        free(item);
    }
}

/**
 * Installes a page block into the main page block array.  If a page block
 * has already been installed at in the first page of the page block being
 * installed, and there is no accessor defined in the new page block, 
 * duplicate the accessor.  If there is no page buffer, duplicate the buffer.
 * Then point the relevant entries in the main page block array to the new
 * page block.
 * @param[in] pb Page block to install.
 * @returns True on success.
 */
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

/**
 * Get the page block for the address.
 * @param[i] address Adress for which to retreive the page block
 * @returns Page block
 */
struct page_block_t *get_page_block(WORD address)
{
    return page_block[hi(address)];
}

/****************************
 * Soft switches
 */

/**
 * Get the soft_swtich structure for requested switch.
 * @param[in] switch_no Soft switch number
 * @returns Struct for soft switch
 */
struct soft_switch_t *get_soft_switch(BYTE switch_no)
{
    struct page_block_t *pb = get_page_block(0xc000);
    struct soft_switch_t *soft_switch = 
        (struct soft_switch_t *) pb->data;
    return &soft_switch[((WORD) switch_no) & 0x00ff];
}

static BYTE soft_switch_accessor(WORD address, bool read, BYTE value)
{
    BYTE switch_no = lo(address);
    struct soft_switch_t *soft_switch = get_soft_switch(switch_no);
  
    if (read) {
        if (NULL != soft_switch->read_accessor)
            value = soft_switch->read_accessor(switch_no, read, value);
        else
            LOG_DBG("Soft switch %02x not set for read.\n", switch_no);
    } else {
        if (!read && NULL != soft_switch->write_accessor)
            soft_switch->write_accessor(switch_no, read, value);
        else
            LOG_DBG("Soft switch %02x not set for write.\n", switch_no);
    }
    
    return value;
}

/**
 * Install soft switch
 * @param[in] switch_no Switch number
 * @param[in] switch_type Type.  SS_READ, SS_WRITE, SS_RDWR
 * @param[in] accessor Accessor function for switch
 * @ returns Structure of soft_switch
 */
struct soft_switch_t *install_soft_switch(BYTE switch_no, int switch_type, 
    soft_switch_accessor_t accessor)
{
    struct soft_switch_t *soft_switch = get_soft_switch(switch_no);

    LOG_INF("Setting soft switch %02x.\n", switch_no);

    if (switch_type & SS_READ)
        soft_switch->read_accessor = accessor;

    if (switch_type & SS_WRITE)
        soft_switch->write_accessor = accessor;

    soft_switch->data = NULL;

    return soft_switch;
}

/**
 * Initialize soft switch routines.
 */
void init_soft_switches()
{
    struct page_block_t *pb;
    LOG_INF("Initializing soft switches.\n");
    pb = create_page_block(0xC0, 1);
    pb->accessor = soft_switch_accessor;
    pb->data = malloc(0x100 * sizeof(struct soft_switch_t));
    memset(pb->data, 0, 0x100 * sizeof(struct soft_switch_t));
    install_page_block(pb);

}

void init_bus()
{
    LOG_INF("Initializing bus.\n");
    LOG_DBG("Size of page_block is %p\n", sizeof(page_block));
    memset(page_block, 0, sizeof(page_block));
    INIT_LIST_HEAD(&buffers);
    INIT_LIST_HEAD(&page_buffers);
    init_soft_switches();
}

