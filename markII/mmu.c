/**
 * @file mmu.c
 * @brief Memory Management Unit.  Handles loading of ROM files, standard RAM and
 * ROM accesses, bank switching, and auxilary memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <types.h>
#include "bus.h"
#include "logging.h"
#include "config.h"

#define PAGES(p) word(0x00, (p))
/****************************************
 * ROM file routines
 */

/**
 * Structure used to hold parts of a ROM entry specified in the configuarion
 * file.
 */
struct ROM_descripter_t {
    char file_name[256];    ///< Config value containing the file name
    BYTE offset;            ///< Offset (in pages) into file for start of ROM
    BYTE page_cnt;          ///< Number of pages
    struct ROM_descripter_t *next; ///< Next ROM_descripter_t object
};

/**
 * Splits character string into list of file names, offsets, and page counts.
 *
 * @param[in] ROM_set String to split
 * @return Pointer to allocated ROM_descripter_t object
 * @note Call free_ROM to deallocate the list.
 */
static struct ROM_descripter_t *parse_ROM(char *ROM_set)
{
    char buf[256];
    char *tmp = buf;
    char tmp2[256];
    struct ROM_descripter_t head;
    struct ROM_descripter_t *rd = &head;
    head.next = NULL;

    do {
        ROM_set = split_string(ROM_set, tmp, ';');
        if (0 == *tmp)
            continue;
      
        rd->next = malloc(sizeof(struct ROM_descripter_t));
        rd = rd->next;
        memset(rd, 0, sizeof(struct ROM_descripter_t));
        LOG_DBG("Allocated ROM address = %p\n", rd);

        // Get ROM filename
        tmp = split_string(tmp, tmp2, ':');
        strncpy(rd->file_name, get_config_string("ROM", tmp2), 255);
    
        // Get ROM offset and page_cnt
        tmp = split_string(tmp, tmp2, ':');
        rd->offset = string_to_hex(tmp2);
        rd->page_cnt = string_to_hex(tmp);
    } while (*ROM_set);

    return head.next;
}

/**
 * Deallocates list of ROM_descripter_t.
 * @param[in] rd First item in list
 */
static void free_ROM(struct ROM_descripter_t *rd)
{
    struct ROM_descripter_t *tmp;
    struct ROM_descripter_t *next;
    for (tmp = rd; NULL != tmp; tmp = next) {
        LOG_DBG("Freeing ROM address = %p\n", tmp);
        next = tmp->next;
        free(tmp);
        tmp = next;
    }
}

/**
 * Load a ROM buffer from setups described in .cfg file.
 * @param[in] ROM_name Name of ROM configurateion
 * @param[in] requested_pages Number of requested pages
 */
BYTE *load_ROM(char *ROM_name, BYTE requested_pages)
{
    struct ROM_descripter_t *rd;
    struct ROM_descripter_t *tmp;
    char *ROM_set;
    int fd;
    BYTE *buffer;
    off_t seek;
    ssize_t loaded;
    WORD total_pages = 0;
    BYTE* load_address;
    WORD actual_pages;

    // Get ROM line
    LOG_INF("Loading ROM %s.\n", ROM_name);
    ROM_set = get_config_string("ROM", ROM_name);
    if (ROM_set == NULL) {
        LOG_WRN("Could not find %s.\n", ROM_name);
        return NULL;
    }

    rd = parse_ROM(ROM_set);
    for (tmp = rd; tmp != NULL; tmp = tmp->next)
        total_pages += tmp->page_cnt;

    actual_pages = (total_pages > requested_pages)? 
        total_pages : requested_pages;

    LOG_INF("%d total pages to load into %d pages.\n", total_pages, 
        requested_pages);

    // Allocate bigger of requested or total number of pages.
    //  load ROM at bottom of buffer.
    buffer = create_page_buffer(actual_pages);
    load_address = buffer + PAGES(actual_pages - total_pages);
  
    for (tmp = rd; tmp != NULL; tmp = tmp->next) { 
        LOG_DBG("Loading %s starting at page $%02X for $%02X pages\n", 
            tmp->file_name, tmp->offset, tmp->page_cnt);

        fd = open(tmp->file_name, O_RDONLY, NULL);
        if (fd == -1)
            LOG_FTL("Error %d trying to open file.\n", errno);

        seek = lseek(fd, PAGES(tmp->offset), SEEK_SET);
        if (seek == -1)
            LOG_FTL("Error %d seeking to page $%02X.\n", errno, tmp->offset);

        loaded = read(fd, load_address, PAGES(tmp->page_cnt));
        if (loaded == -1)
            LOG_FTL("Error %d loading ROM file.\n", errno);

        LOG_INF("%d bytes loaded.\n", loaded);
        load_address += PAGES(tmp->page_cnt);
        close(fd);
    } 

    free_ROM(rd);

    return buffer;
}

/****************************
 * Standard RAM
 */

/**
 * Handles standard RAM access.
 * @param[in] address Address of byte to access
 * @param[in] read Read RAM when true
 * @param[in] value Value to write
 * @returns Value read
 */
static BYTE RAM_accessor(WORD address, bool read, BYTE value)
{
    struct page_block_t *pb = get_page_block(address);
   
    if (NULL != pb->buffer) {
        if (read)
            value = pb->buffer[pb_offset(pb, address)];
        else
            pb->buffer[pb_offset(pb, address)] = value;
    } else {
        value = 0;
    }

    return value;
}

/**
 * Initialize standard RAM.
 */
static void init_RAM()
{
    struct page_block_t *pb;
    
    // Create 16k (0x40 pages) of RAM
    pb = create_page_block(0, 0xc0);
    pb->buffer = create_page_buffer(pb->total_pages);
    pb->accessor = RAM_accessor;
    install_page_block(pb);
    
}

/*******************
 * Standard ROM
 */

/**
 * Handles standard ROM access.
 * @param[in] address Address of byte to access
 * @param[in] read Read ROM when true
 * @param[in] value Value to write
 * @returns Value read
 */
static BYTE ROM_accessor(WORD address, bool read, BYTE value)
{
    struct page_block_t *pb = get_page_block(address);
   
    if (NULL != pb->buffer) {
        value = pb->buffer[pb_offset(pb, address)];
    } else {
        value = 0;
    }

    return value;
}

/**
 * Initialize standard RAM.
 */
static void init_ROM()
{
    char *ROM_key;
    struct page_block_t *pb;
    
    // create ROM
    ROM_key = get_config_string("MARKII", "MAIN_ROM");
    pb = create_page_block(0xc1, 0x3f);
    pb->buffer = load_ROM(ROM_key, 0x3F);
    pb->accessor = ROM_accessor;
    install_page_block(pb);
}

/*****************************************
 * Alternate zero page and stack routines
 */

const BYTE SS_SETSTDZP  = 0x08; ///< Select standard zero page
const BYTE SS_SETALTZP  = 0x09; ///< Select alternate zero page
const BYTE SS_RDALTZP   = 0x16; ///< Read which zero page is selected

/**
 * Determine which zero page/stack to access.
 * @param[in] switch_no Switch number to access
 * @param[in] read Read switch when true.  Otherwise write.
 * @param[in] value Value to write
 * @returns Value read 
 */
static BYTE zp_soft_switch(BYTE switch_no, bool read, BYTE value)
{
    struct page_block_t *pb = get_page_block(0);
    BYTE *buffer = (BYTE *) get_soft_switch(switch_no)->data;

    if (SS_RDALTZP == switch_no)
        value = (pb->buffer == buffer)? 0x80 : 0x00;
    else
        pb->buffer = buffer;

    return value;
}

/**
 * Initialize alternate stack and zero page.
 */
static void init_alt_zp()
{
    struct page_block_t *pb;
    struct soft_switch_t *ss;

    // Create Alt zero page and stack (0x02 pages)
    pb = create_page_block(0, 2);
    install_page_block(pb);
    
    ss = install_soft_switch(SS_SETSTDZP, SS_WRITE, zp_soft_switch);
    ss->data = pb->buffer;
    
    ss = install_soft_switch(SS_SETALTZP, SS_WRITE, zp_soft_switch);
    ss->data = pb->buffer;
    
    ss = install_soft_switch(SS_RDALTZP, SS_READ, zp_soft_switch);
    ss->data = pb->buffer;
}

/**
 * @section bankswitching Bank switching routines
 */

static bool bs_read_enabled = false;   ///< Read bank switched RAM when true
static bool bs_write_enabled = false;  ///< Write bank switched RAM when false

static BYTE *D000_buffer[2]; //< Page D0 buffers

/**
 * Handles bank switched RAM access.
 * @param[in] address Address of byte to access
 * @param[in] read Read RAM when true
 * @param[in] value Value to write
 * @returns Value read
 */
static BYTE bank_switch_accessor(WORD address, bool read, BYTE value)
{
    struct page_block_t *pb = get_page_block(address);
    BYTE *RAM_buffer = (BYTE *) pb->data;
            
    if (read) {
        if (bs_read_enabled)
            value = RAM_buffer[pb_offset(pb, address)];
        else
            value = pb->buffer[pb_offset(pb, address)];
    } else {
        if (bs_write_enabled)
            RAM_buffer[pb_offset(pb, address)] = value;
    }

    return value;
}

/**
 * Determine which ROM or RAM bank to access.
 * @param[in] switch_no Switch number to access
 * @param[in] read Read switch when true.  Otherwise write.
 * @param[in] value Value to write
 * @returns Value read 
 */
static BYTE bank_switch_soft_switch(BYTE switch_no, bool read, BYTE value)
{
    struct page_block_t *pb = get_page_block(0xd000);

    bs_write_enabled = switch_no & 0x01;
    bs_read_enabled = ((switch_no & 0x03) == 0x03) || 
        ((switch_no & 0x03) == 0x00);

    pb->data = D000_buffer[(switch_no & 0x08)? 0 : 1];
    return value;
}

/**
 * Initialize bank switching
 */
static void init_bank_switch_memory()
{
    int i;
    struct page_block_t *pb;

    pb = create_page_block(0xd0, 0x10);
    pb->accessor = bank_switch_accessor;
    install_page_block(pb);
    D000_buffer[0] = create_page_buffer(0x10);
    D000_buffer[1] = create_page_buffer(0x10);
    pb->data = D000_buffer[1];
    
    pb = create_page_block(0xe0, 0x20);
    pb->accessor = bank_switch_accessor;
    install_page_block(pb);
    pb->data = create_page_buffer(0x20);

    for (i = 0x80; i <= 0x8f; ++i)
        install_soft_switch(i, SS_RDWR, bank_switch_soft_switch);    
}

/*****************************
 * Auxiliary memory
 */

static bool aux_read_enabled = false;   ///< Aux RAM read enabled
static bool aux_write_enabled = false;  ///< Aux RAM write enabled

const BYTE SS_RDMAINRAM = 0x02; ///< Set main RAM for reading
const BYTE SS_RDCARDRAM = 0x03; ///< Set aux RAM for reading
const BYTE SS_WRMAINRAM = 0x04; ///< Set main RAM for writing
const BYTE SS_WRCARDRAM = 0x05; ///< Set aux RAM for writing
const BYTE SS_RDRAMRD   = 0x13; ///< Get status of main/aux RAM reading 
const BYTE SS_RDRAMWRT  = 0x14; ///< Get status of main/aux RAM writing

/**
 * Handles auxiliary RAM access.
 * @param[in] address Address of byte to access
 * @param[in] read Read RAM when true
 * @param[in] value Value to write
 * @returns Value read
 */
static BYTE aux_RAM_accessor(WORD address, bool read, BYTE value)
{
    struct page_block_t *pb = get_page_block(address);
    BYTE *buffer = (BYTE *) pb->data;

    if (read) {
        if (aux_read_enabled)
            value = buffer[pb_offset(pb, address)];
        else
            value = pb->buffer[pb_offset(pb, address)];
    } else {
        if (aux_write_enabled)
            buffer[pb_offset(pb, address)] = value;
        else
            pb->buffer[pb_offset(pb, address)] = value;
    }

    return value;
}

/**
 * Determine which RAM bank (standard or auxiliary) to access.
 * @param[in] switch_no Switch number to access
 * @param[in] read Read switch when true.  Otherwise write.
 * @param[in] value Value to write
 * @returns Value read 
 */
static BYTE aux_mem_soft_switch(BYTE switch_no, bool read, BYTE value)
{

    if (SS_RDMAINRAM == switch_no)   
        aux_read_enabled = false;
    else if (SS_RDCARDRAM == switch_no)
        aux_read_enabled = true;
    else if (SS_WRMAINRAM == switch_no)
        aux_write_enabled = false;
    else if (SS_WRCARDRAM == switch_no)
        aux_write_enabled = true;
    else if (SS_RDRAMRD == switch_no)
        value = aux_read_enabled? 0x80 : 0x00;
    else
        value = aux_write_enabled? 0x80 : 0x00;

    return value;
}
/**
 * Initialize auxiliary memory.
 */
static void init_aux_mem()
{
    struct page_block_t *pb;

    pb = create_page_block(0x02, 0xbd);
    pb->accessor = aux_RAM_accessor;
    install_page_block(pb);
    pb->data = create_page_buffer(0xbd);

    install_soft_switch(SS_RDMAINRAM, SS_WRITE, aux_mem_soft_switch);
    install_soft_switch(SS_RDCARDRAM, SS_WRITE, aux_mem_soft_switch);
    install_soft_switch(SS_WRMAINRAM, SS_WRITE, aux_mem_soft_switch);
    install_soft_switch(SS_WRCARDRAM, SS_WRITE, aux_mem_soft_switch);
    install_soft_switch(SS_RDRAMRD, SS_READ, aux_mem_soft_switch);
    install_soft_switch(SS_RDRAMWRT, SS_READ, aux_mem_soft_switch);
}

/**
 * Initialize mmu routines
 */
void init_mmu()
{
    init_RAM();
    init_ROM();
    init_alt_zp();
    init_bank_switch_memory();
    init_aux_mem();
}
