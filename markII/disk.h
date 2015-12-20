#ifndef __DISK_H
#define __DISK_H

#include <types.h>
 
/*!
 * Stepper phase magnet
 */
struct stepper {
    bool state;             //< State of this phase 
    struct stepper* left;   //< Stepper to left 
    struct stepper* right;  //< Stepper to right
};

struct map_t {
    BYTE data[0];
};

/**
 * Describes the disk drive.
 */
struct drive_t {
    DWORD           bit_no;         ///< Current bit number
    size_t          track_size;     ///< Size in bytes of the tracks
    int             track_cnt;      ///< Number of tracks
    BYTE            *cur_track;     ///< Current track
    int             half_track_no;  ///< Current half-track
    int             fd;             ///< Disk File descripter
    size_t          map_size;       ///< Size in bytes of disk file
    struct map_t    *map;           ///< Disk map
    struct stepper  phase[4];       ///< Drive steppers
    struct stepper  *cur_stepper;   ///< Current stepper
    union {                         ///< Read head
        struct {
            BYTE low;
            BYTE hi;
        } byte;
        WORD shifter;
    }               rw_head;
    BYTE            write_clock;    ///< Write clock
    bool            motor_on;       ///< State of drive motor
    bool            read_mode;      ///< Read/Write mode
    DWORD           clocks;         ///< CPU clocks
    BYTE            data_latch;     ///< Data latch
    BYTE            rw_latch;       ///< Read/write latch
    bool            strobe;
    DWORD           spin_down_clocks; ///< Spin-down clocks
    bool            verbose;        ///< Logging verbosity
    int             drive_no;       ///< This drive number
};

void init_disk();
void finalize_disk();
struct drive_t *get_drive(int drive_no);
void init_drive(int drive_no);

void init_steppers(struct drive_t* drive);

void set_phase(struct drive_t *drive, int phase_switch);
void create_disk(struct drive_t *drive, int tracks, size_t track_size,
    char *map_filename);
void load_disk(struct drive_t *drive, char *map_filename);
void unload_disk(struct drive_t *drive);
BYTE write_bit(struct drive_t *drive, BYTE bit);
BYTE read_bit(struct drive_t *drive);
void load_half_track(struct drive_t *drive);

#endif
