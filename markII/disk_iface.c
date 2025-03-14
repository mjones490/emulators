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

// Disk config
char *disk_1_mapfile = NULL;
char *disk_2_mapfile = NULL;
int slot_no;

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

/****************************************************************
 * 6502 Interface code
 ****************************************************************/
struct drive_t* drive;
struct drive_t ddrive[2];

#define PHASE_0_OFF         0x00
#define PHASE_0_ON          0x01
#define PHASE_1_OFF         0x02
#define PHASE_1_ON          0x03
#define PHASE_2_OFF         0x04
#define PHASE_2_ON          0x05
#define PHASE_3_OFF         0x06
#define PHASE_3_ON          0x07
#define DRIVE_MOTOR_OFF     0x08
#define DRIVE_MOTOR_ON      0x09
#define DRIVE_0_ENGAGE      0x0A
#define DRIVE_1_ENGAGE      0x0B
#define STROBE_DATA_LATCH   0x0C
#define LOAD_DATA_LATCH     0x0D
#define INPUT_LATCH         0x0E
#define OUTPUT_LATCH        0x0F

BYTE drive_motor(BYTE port, bool read, BYTE value)
{
    if (port == 0xE8)
        drive->motor_on = false;
    else 
        drive->motor_on = true;
    
    LOG_INF("Drive %d motor is %s.\n", drive->drive_no, 
        drive->motor_on? "on" : "off");
    LOG_DBG("Bit number = %08x\n", (unsigned int) drive->bit_no);

    return 0;
}

BYTE drive_select(BYTE port, bool read, BYTE value)
{
    int drive_no = port & 0x01;
    LOG_INF("Select drive %d.\n", drive_no);
    LOG_DBG("Port = %x.\n", port);
    drive = &ddrive[drive_no];
    drive->motor_on = ddrive[drive_no ^ 0x01].motor_on;
    ddrive[drive_no ^ 0x01].motor_on = false;
    return 0;
}

BYTE phase_switch(BYTE port, bool read, BYTE value)
{
    set_phase(drive, port & 0x07);
    return 0;
}

BYTE port_data = 0;

int read_clocks;
int shifts;

BYTE drive_latches(BYTE port, bool read, BYTE value)
{
    struct map_header_t *header;
    switch (port) {
    case STROBE_DATA_LATCH:
        drive->strobe = true;
        value = drive->rw_latch;
        break;

    case INPUT_LATCH:
        drive->read_mode = true;
        value = drive->rw_latch;
        LOG_INF("Prepared for input.\n");
        break;

    case OUTPUT_LATCH:
        drive->read_mode = false;
        drive->rw_latch = value;
        LOG_INF("Prepared for output.\n");
        return 0;

    case LOAD_DATA_LATCH:
        if (drive->read_mode) {
            header = (struct map_header_t *) drive->map->header;
            drive->rw_latch = header->write_protected? 0x80 : 0x00;
        } else {
            drive->rw_latch = value;
        }
        break;
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

void init_drive(int drive_no)
{
    init_steppers(&ddrive[drive_no]);
    ddrive[drive_no].drive_no = drive_no;
    ddrive[drive_no].verbose = true;
    ddrive[drive_no].half_track_no = 0;
    ddrive[drive_no].clocks = 0;
    ddrive[drive_no].read_mode = true;
}

void set_port(BYTE switch_no, soft_switch_accessor_t accessor,
    soft_switch_accessor_t dummy)
{
    install_slot_switch(switch_no, SS_RDWR, accessor);
}

void init_disk()
{
    char *boot_ROM_name = get_config_string("DISKII", "BOOT_ROM");
    BYTE *boot_ROM = load_ROM(boot_ROM_name, 1);
    
    LOG_INF("Init DiskII...\n");
    if (!load_disk_config()) {
        LOG_ERR("Error reading DiskII configuration.\n");
        return;
    }
    
    install_slot_ROM(slot_no, boot_ROM, NULL);
    
    init_drive(0);
    init_drive(1);

    // Set up switches
    set_port(PHASE_0_OFF, phase_switch, phase_switch);
    set_port(PHASE_0_ON, phase_switch, phase_switch);
    set_port(PHASE_1_OFF, phase_switch, phase_switch);
    set_port(PHASE_1_ON, phase_switch, phase_switch);
    set_port(PHASE_2_OFF, phase_switch, phase_switch);
    set_port(PHASE_2_ON, phase_switch, phase_switch);
    set_port(PHASE_3_OFF, phase_switch, phase_switch);
    set_port(PHASE_3_ON, phase_switch, phase_switch);
    set_port(DRIVE_MOTOR_OFF, drive_motor, drive_motor);
    set_port(DRIVE_MOTOR_ON, drive_motor, drive_motor);
    set_port(DRIVE_0_ENGAGE, drive_select, drive_select);
    set_port(DRIVE_1_ENGAGE, drive_select, drive_select);
    set_port(STROBE_DATA_LATCH, drive_latches, drive_latches);
    set_port(LOAD_DATA_LATCH, drive_latches, drive_latches);
    set_port(INPUT_LATCH, drive_latches, drive_latches);
    set_port(OUTPUT_LATCH, drive_latches, drive_latches);

    load_disk(&ddrive[0], disk_1_mapfile);
    load_disk(&ddrive[1], disk_2_mapfile);
    drive = &ddrive[0];
    add_device(drive_clock);
}

void finalize_disk()
{
    unload_disk(&ddrive[0]);
    unload_disk(&ddrive[1]);
}

struct drive_t *get_drive(int drive_no)
{
    struct drive_t *drive = NULL;
    
    if (drive_no < 0 || drive_no > 1)
        LOG_ERR("Bad drive number %d\n", drive_no);
    else
        drive = &ddrive[drive_no];
         
    return drive;
}
