/**
 * @file disk_iface.c
 */ 
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "disk.h"
#include "logging.h"
#include "config.h"
#include "bus.h"
#include "cpu_iface.h"
#include "mmu.h"
#include "slot.h"

/****************************************************************
 * 6502 Interface code
 ****************************************************************/
struct drive_t* drive;
struct drive_t ddrive[2];

/** @name Soft Switches
 * Soft switches used by disk interface
 */
///@{
const BYTE SS_PHASE0OFF = 0x00; ///< Stepper motor phase 0 off.
const BYTE SS_PHASE0ON  = 0x01; ///< Stepper motor phase 0 on.
const BYTE SS_PHASE1OFF = 0x02; ///< Stepper motor phase 1 off.
const BYTE SS_PHASE1ON  = 0x03; ///< Stepper motor phase 1 on.
const BYTE SS_PHASE2OFF = 0x04; ///< Stepper motor phase 2 off.
const BYTE SS_PHASE2ON  = 0x05; ///< Stepper motor phase 2 on.
const BYTE SS_PHASE3OFF = 0x06; ///< Stepper motor phase 3 off.
const BYTE SS_PHASE3ON  = 0x07; ///< Stepper motor phase 3 on.
const BYTE SS_MOTOROFF  = 0x08; ///< Turn motor off.
const BYTE SS_MOTORON   = 0x09; ///< Turn motor on.
const BYTE SS_DRV0EN    = 0x0a; ///< Engage drive 0.
const BYTE SS_DRV1EN    = 0x0b; ///< Engage drive 1.
const BYTE SS_Q6L       = 0x0c; ///< Strobe data latch for I/O.
const BYTE SS_Q6H       = 0x0d; ///< Load data latch.
const BYTE SS_Q7L       = 0x0e; ///< Prepare latch for output.
const BYTE SS_Q7H       = 0x0f; ///< Prepare latch for input.
///@}


BYTE drive_motor(BYTE switch_no, bool read, BYTE value)
{
    if (SS_MOTOROFF == switch_no)
        drive->motor_on = false;
    else 
        drive->motor_on = true;
    
    LOG_INF("Drive %d motor is %s.\n", drive->drive_no, 
        drive->motor_on? "on" : "off");
    LOG_DBG("Bit number = %08x\n", (unsigned int) drive->bit_no);

    return 0;
}

BYTE drive_select(BYTE switch_no, bool read, BYTE value)
{
    int drive_no = switch_no & 0x01;
    LOG_INF("Select drive %d.\n", drive_no);
    LOG_DBG("Port = %x.\n", switch_no);
    drive = &ddrive[drive_no];
    drive->motor_on = ddrive[drive_no ^ 0x01].motor_on;
    ddrive[drive_no ^ 0x01].motor_on = false;
    return 0;
}

BYTE phase_switch(BYTE switch_no, bool read, BYTE value)
{
    set_phase(drive, switch_no & 0x07);
    return 0;
}

BYTE port_data = 0;

int read_clocks;
int shifts;

BYTE drive_latches(BYTE switch_no, bool read, BYTE value)
{
    struct map_header_t *header;
    
    if (SS_Q6L == switch_no) {
        drive->strobe = true;
        value = drive->rw_latch;
    } else if (SS_Q7L == switch_no) {
        drive->read_mode = true;
        value = drive->rw_latch;
        LOG_INF("Prepared for input.\n");
    } else if (SS_Q7H == switch_no) {
        drive->read_mode = false;
        drive->rw_latch = value;
        LOG_INF("Prepared for output.\n");
        return 0;
    } else {
        if (drive->read_mode) {
            header = (struct map_header_t *) drive->map->header;
            drive->rw_latch = header->write_protected? 0x80 : 0x00;
        } else {
            drive->rw_latch = value;
        }
    } 

    return value;
}

const int max_spin_down_clocks = 5000;

void spin_drive(struct drive_t *drive, BYTE cpu_clocks)
{
    static bool read_mode = true;

    if (drive->motor_on) 
        drive->spin_down_clocks = 1024 * max_spin_down_clocks;

    if (drive->spin_down_clocks > 10) { 
        drive->clocks += cpu_clocks;
        drive->spin_down_clocks -= cpu_clocks;
        while (drive->clocks >= 4) {
            if (read_mode) {
                drive->data_latch = (drive->data_latch << 1) |
                    read_bit(drive);
            } else {
                write_bit(drive, drive->data_latch >> 7);
                drive->data_latch <<= 1;
            }
            drive->clocks -= 4;
        }
 
        read_mode = drive->read_mode;       
        if (drive->strobe) {
            if (read_mode) 
                drive->rw_latch &= 0x7f;
            else
                drive->data_latch = drive->rw_latch;
            drive->strobe = false;
        }
                
        if (read_mode && (drive->data_latch & 0x80)) {
            drive->rw_latch = drive->data_latch;
            drive->data_latch = 0;
        }

        if (drive->spin_down_clocks <= 10)
            LOG_INF("Drive %d spun down\n", drive->drive_no);        
    }
}

void drive_clock(BYTE cpu_clocks)
{
    if (cpu_clocks > 0) {
        spin_drive(&ddrive[0], cpu_clocks);
        spin_drive(&ddrive[1], cpu_clocks);
    }
}

/**
 * @name Initialization and Configuration
 * @{
 */

char *disk_1_mapfile = NULL;
char *disk_2_mapfile = NULL;
int slot_no;

/**
 * Read disk configuration from markII.cfg.
 */
bool load_disk_config()
{
    disk_1_mapfile = get_config_string("DISKII", "DISK_1_MAPFILE");
    if (disk_1_mapfile == NULL) {
        LOG_ERR("Could not find DISK_1_MAPFILE.\n");
        return false;
    }

    disk_2_mapfile = get_config_string("DISKII", "DISK_2_MAPFILE");
    if (disk_2_mapfile == NULL) {
        LOG_ERR("Could not find DISK_2_MAPFILE.\n");
        return false;
    }

    slot_no = get_config_int("DISKII", "SLOT_NUMBER");
    if (0 == slot_no)
        slot_no = 6;
    LOG_INF("Disk II slot number is %d.\n", slot_no);
    
    return true;
}

/**
 * Initialize drive
 * @param[in] drive_no Number of drive
 */
void init_drive(int drive_no)
{
    init_steppers(&ddrive[drive_no]);
    ddrive[drive_no].drive_no = drive_no;
    ddrive[drive_no].verbose = true;
    ddrive[drive_no].half_track_no = 0;
    ddrive[drive_no].clocks = 0;
    ddrive[drive_no].read_mode = true;
}

/**
 * Configure. Setup slot ROM.  Initialize drives. Set switches. Load
 * disk maps.
 */
void init_disk()
{
    char *boot_ROM_name = get_config_string("DISKII", "BOOT_ROM");
    BYTE *boot_ROM = load_ROM(boot_ROM_name, 1);
    BYTE ss_phase;
    
    LOG_INF("Init DiskII...\n");
    if (!load_disk_config()) {
        LOG_ERR("Error reading DiskII configuration.\n");
        return;
    }
    
    install_slot_ROM(slot_no, boot_ROM, NULL);
    
    init_drive(0);
    init_drive(1);

    // Set up switches
    for (ss_phase = SS_PHASE0OFF; ss_phase <= SS_PHASE3ON; ++ss_phase)
        install_slot_switch(ss_phase, SS_RDWR, phase_switch);
    
    install_slot_switch(SS_MOTOROFF, SS_RDWR, drive_motor);
    install_slot_switch(SS_MOTORON, SS_RDWR, drive_motor);
    install_slot_switch(SS_DRV0EN, SS_RDWR, drive_select);
    install_slot_switch(SS_DRV1EN, SS_RDWR, drive_select);

    install_slot_switch(SS_Q6L, SS_RDWR, drive_latches);
    install_slot_switch(SS_Q6H, SS_RDWR, drive_latches);
    install_slot_switch(SS_Q7L, SS_RDWR, drive_latches);
    install_slot_switch(SS_Q7H, SS_RDWR, drive_latches);

    load_disk(&ddrive[0], disk_1_mapfile);
    load_disk(&ddrive[1], disk_2_mapfile);
    drive = &ddrive[0];
    add_device(drive_clock);
}

/**
 * Unload disk maps.
 */
void finalize_disk()
{
    unload_disk(&ddrive[0]);
    unload_disk(&ddrive[1]);
}

/**
 * Get drive data.
 * @param[in] drive_no Drive number.
 */
struct drive_t *get_drive(int drive_no)
{
    struct drive_t *drive = NULL;
    
    if (drive_no < 0 || drive_no > 1)
        LOG_ERR("Bad drive number %d\n", drive_no);
    else
        drive = &ddrive[drive_no];
         
    return drive;
}

/*@}*/

