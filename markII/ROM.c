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
 * @param[in] First item in list
 */
static void free_ROM(struct ROM_descripter_t *rd)
{
    struct ROM_descripter_t *tmp;
    for (tmp = rd; NULL != tmp; tmp = tmp->next)
        free(tmp);
}

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
