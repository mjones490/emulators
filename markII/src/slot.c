/**
 * @file slot.c
 * @brief Handles slot ROM installation and selection.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "logging.h"
#include "config.h"
#include "bus.h"
#include "mmu.h"

const BYTE SS_SETSLOTCXROM = 0x06;  ///< Select slot ROM
const BYTE SS_SETINTCXROM  = 0x07;  ///< Select internal ROM
const BYTE SS_RDCXROM = 0x15;       ///< Get slot ROM selection state

/**
 * Hold slot and epansion ROM pointers.
 */
struct slot_t {
    BYTE *slot_ROM;         ///< Slot ROM
    BYTE *expansion_ROM;    ///< 2k expansion ROM
};

/**
 * Slot slection state
 */
struct slot_data_t {
    bool selected;      ///< Slot ROM selected
    BYTE current_slot;  ///< Current expansion ROM slot number
};

/**
 * Select or deselct slot ROM.  Set expansion ROM to internal.
 * @param[in] switch_no Switch number to access
 * @param[in] read Read switch when true.  Otherwise write.
 * @param[in] value Value to write
 * @returns Value read 
 */
static BYTE CXROM_switch(BYTE switch_no, bool read, BYTE value)
{
    struct page_block_t *pb = get_page_block(0xC800);
    struct slot_data_t *slot_data = (struct slot_data_t *) pb->data;
    if (switch_no == SS_SETSLOTCXROM) {
        slot_data->selected = true;  
        slot_data->current_slot = 0;
    } else if (switch_no == SS_SETINTCXROM) {
        slot_data->selected = false;
    } else {
        value = (slot_data->selected)? 0x80 : 0x00;
    }

    return value;    
}

/**
 * Handles slot ROM access.  Selects slot as current slot.
 * @param[in] address Address of byte to access
 * @param[in] read Read RAM when true
 * @param[in] value Value to write
 * @returns Value read
 */
static BYTE slot_ROM_accessor(WORD address, bool read, BYTE value)
{
    struct page_block_t *pb = get_page_block(0xc800);
    struct slot_data_t *slot_data = (struct slot_data_t *) pb->data;
    pb = get_page_block(address);
    struct slot_t *slot = (struct slot_t *) pb->data;
    BYTE current_slot = hi(address) & 0x07;

    if (slot_data->current_slot != current_slot) {
        slot_data->current_slot = current_slot;
    }

    if (slot_data->selected && NULL != slot->slot_ROM) {
        value = slot->slot_ROM[pb_offset(pb, address)];
    } else {
        value = pb->buffer[pb_offset(pb, address)];
    }

    return value;
}

/**
 * Handles expansion ROM access.  Deselects then last byte is accessed.
 * @param[in] address Address of byte to access
 * @param[in] read Read RAM when true
 * @param[in] value Value to write
 * @returns Value read
 */
static BYTE expansion_ROM_accessor(WORD address, bool read, BYTE value)
{
    struct page_block_t *pb = get_page_block(0xc800);
    struct slot_data_t *slot_data = (struct slot_data_t *) pb->data;
    struct slot_t *slot;
    BYTE *buffer = pb->buffer;
    
    if (address == 0xcfff)
        slot_data->current_slot = 0;

    address = pb_offset(pb, address);
    
    if (slot_data->selected && 0 != slot_data->current_slot) {
        pb = get_page_block(word(0x00, slot_data->current_slot | 0xc0));
        slot = (struct slot_t *) pb->data;
        if (NULL != slot->expansion_ROM)
            buffer = slot->expansion_ROM;
    }

    value = buffer[address];
    return value;
}

/**
 * Translate the actual switch number in to the slot switch offset.  Call
 * the slot soft switch accessor in the standard way.
 * @param[in] switch_no Switch number to access
 * @param[in] read Read switch when true.  Otherwise write.
 * @param[in] value Value to write
 * @returns Value read 
 */
static BYTE slot_soft_switch_accessor(BYTE switch_no, bool read, BYTE value)
{
    BYTE slot_switch_no = switch_no & 0x0f;
    struct soft_switch_t *ss = 
        (struct soft_switch_t *) get_soft_switch(switch_no);
    struct soft_switch_t *slot_switch = (struct soft_switch_t *) ss->data;

    if (read && NULL != slot_switch->read_accessor)
        value = slot_switch->read_accessor(slot_switch_no, read, value);
    else if (!read && NULL != slot_switch->write_accessor)
        value = slot_switch->write_accessor(slot_switch_no, read, value);

   return value;
}

/**
 * Translate the slot soft switch number to the actual switch number.  
 * Install the soft switch, with the data pointing to the accessors
 * for the slot switch.
 * @param[in] switch_no Slot soft switch offset
 * @param[in] switch_type SS_READ, SS_WRITE, SS_RDWR
 * @param[in] accessor Switch accessor function pointer
 */
struct soft_switch_t *install_slot_switch(int switch_no, int switch_type,
   soft_switch_accessor_t accessor)
{
    struct page_block_t *pb = get_page_block(0xc800);
    struct slot_data_t *slot_data = (struct slot_data_t *) pb->data;
    struct soft_switch_t *ss = NULL;
    struct soft_switch_t *slot_switch = NULL;

    if (0 != slot_data->current_slot) {
        LOG_INF("Installing slot %d switch %02x.\n", slot_data->current_slot,
            switch_no);
        switch_no += 0x80 + (slot_data->current_slot << 4);
        ss = install_soft_switch(switch_no, switch_type, 
            slot_soft_switch_accessor);
        slot_switch = malloc(sizeof(struct soft_switch_t));
        if (switch_type & SS_READ)
            slot_switch->read_accessor = accessor;
        if (switch_type & SS_WRITE)
            slot_switch->write_accessor = accessor;
    }

    ss->data = slot_switch;

    return ss;
}

/**
 * Set up page block for given slot with slot ROM and expansion ROM.
 * Set current_slot to slot_no so that subsequent slot soft switches can
 * be easily set.
 * @param[in] slot_no Slot number.
 * @param[in] slot_ROM Pointer to 256 byte slot ROM buffer.
 * @param[in] expansion_ROM Pointer to 2k expansion ROM buffer. 
 */
void install_slot_ROM(BYTE slot_no, BYTE *slot_ROM, BYTE *expansion_ROM)
{
    struct page_block_t *pb = get_page_block(0xc800);
    struct slot_data_t *slot_data = (struct slot_data_t *) pb->data;
    struct slot_t *slot;
    WORD address = word(0x00, 0xc0 + slot_no);

    pb = get_page_block(address);
    slot = (struct slot_t *) pb->data;
    slot->slot_ROM = slot_ROM;
    slot->expansion_ROM = expansion_ROM;   
    slot_data->current_slot = slot_no;
}

/**
 * Set up page blocks for individual slots and 2k expansion ROM.
 * Install ROM selection soft switches.
 */
void init_slot()
{
    int i;
    struct page_block_t *pb;
    struct slot_t *slot;
    struct slot_data_t *slot_data;
    
    LOG_INF("Initiallizing slot routines.\n");

    for (i = 0xC1; i < 0xC8; ++i) {
        pb = create_page_block(i, 1);
        pb->accessor = slot_ROM_accessor;
        slot = malloc(sizeof(struct slot_t));   
        memset(slot, 0, sizeof(struct slot_t));
        pb->data = slot;
        install_page_block(pb);
    }

    pb = create_page_block(0xc8, 0x08);
    pb->accessor = expansion_ROM_accessor;
    slot_data = malloc(sizeof(struct slot_data_t));
    slot_data->selected = false;
    pb->data = slot_data;
    install_page_block(pb);
 
    install_soft_switch(SS_SETSLOTCXROM, SS_WRITE, CXROM_switch);             
    install_soft_switch(SS_SETINTCXROM, SS_WRITE, CXROM_switch);             
    install_soft_switch(SS_RDCXROM, SS_READ, CXROM_switch);             
}

/**
 * Free each of the slot data and expansion ROM data.
 */
void finalize_slot()
{
    int i, j;
    BYTE ss_base;
    struct page_block_t *pb;
    struct soft_switch_t *ss;
 
    LOG_INF("Finalizing slot routines.\n");
       
    for (i = 0xc1; i < 0xc8; ++i) {
        pb = get_page_block(word(0x00, i));
        ss_base = 0x80 + ((i & 0x0f) << 4);
        for (j = 0; j < 0x0f; ++j) {
            ss = get_soft_switch(ss_base + j);
            if (NULL != ss->data)
                free(ss->data);
        }
        free(pb->data);
    }

    pb = get_page_block(0xc800);
    free(pb->data);
}


