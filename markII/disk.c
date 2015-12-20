/**
 * @file disk.c
 * @brief Code to emulate Disk ][ hardware
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
#include <errno.h>
#include "disk.h"
#include "logging.h"

/**
 * Determines by the states of the current stepper and the new stepper
 * and where they are in relation to each other.  Then decides if the
 * head should move, and if so, which direstion.
 * @param[in] drive         Drive in which steppers reside
 * @param[in] this_stepper  Stepper which just changed state
 * @return Direction (1 or -1) the drive head should move
 */
int step_phase(struct drive_t *drive, struct stepper *this_stepper)
{
    int direction = 0;

    if (drive->cur_stepper == this_stepper->left->left) {
        // If current stepper is opposite of this stepper, no change
        direction = 0;
     } else if (this_stepper->state && !drive->cur_stepper->state) {
        // If this stepper is turned on and the current stepper is off,
        //  move to this stepper.
        if (drive->cur_stepper == this_stepper->left)
            direction = 1;
        else
            direction = -1;

        drive->cur_stepper = this_stepper;
    } else if (this_stepper == drive->cur_stepper && !this_stepper->state && 
        (this_stepper->left->state != this_stepper->right->state)) {
        // If this stepper is off, and either the left or right
        //  stepper is on, move to the on stepper.
        if (this_stepper->left->state) {
            drive->cur_stepper = this_stepper->left;
            direction = -1;
        } else {
            drive->cur_stepper = this_stepper->right;
            direction = 1;
        }
    }

    return direction;
}

/**
 * Determine by the phase_switch which stepper changes state.  Calls 
 * step_phase and sets half-track.
 * @param[in]   drive Drive to set the phase on
 * @param[in]   phase_switch Any one of the 8 soft-swtitches for phase
 * @see step_phase
 * @see load_half_track
 */
void set_phase(struct drive_t *drive, int phase_switch)
{
    bool state;
    int direction;
    int prev_track = drive->half_track_no;
    struct stepper *prev_stepper = drive->cur_stepper;

    state = phase_switch & 0x01;
    phase_switch = (phase_switch >> 1) & 3;

    drive->phase[phase_switch].state = state;
    direction = step_phase(drive, &drive->phase[phase_switch]);
    drive->half_track_no += direction;
    if (drive->half_track_no < 0 || drive->half_track_no > drive->track_cnt) {
        if (drive->verbose)
            LOG_DBG("Click!\n");
        drive->half_track_no -= direction;
        drive->cur_stepper = prev_stepper;
    }
    
    if (prev_track != drive->half_track_no) {   
        load_half_track(drive);
    }
}

/**
 * Initialize the steppers and connect them.
 * @param[in] Drive in which to init steppers.
  */
void init_steppers(struct drive_t* drive)
{
    // Init steppers
    drive->phase[0].state = false;
    drive->phase[0].left = &drive->phase[3];
    drive->phase[0].right = &drive->phase[1];
    drive->phase[1].state = false;
    drive->phase[1].left = &drive->phase[0];
    drive->phase[1].right = &drive->phase[2];
    drive->phase[2].state = false;
    drive->phase[2].left = &drive->phase[1];
    drive->phase[2].right = &drive->phase[3];
    drive->phase[3].state = false;
    drive->phase[3].left = &drive->phase[2];
    drive->phase[3].right = &drive->phase[0];
    drive->cur_stepper = &drive->phase[0];
    
}

/****************************************************************
 * Read/Write head code
 ****************************************************************/

///* Calculate the current byte number
#define CURRENT_BYTE(BIT) (BIT >> 3)
///* Calculate the next byte number
#define NEXT_BYTE(BIT, LENGTH) ((CURRENT_BYTE(BIT) + 1) % (LENGTH))

/**
 * Set the current disk track to beginning of track specified in half-
 * track number and load the read head the next byte.
 * @param[in] drive Drive whose half-track is to be loaded
 *
 */
void load_half_track(struct drive_t* drive)
{
    while (drive->write_clock != 0)
        read_bit(drive);
    drive->cur_track = drive->map->data + ((drive->half_track_no >> 1) *
        drive->track_size);

    if (drive->verbose)
        LOG_DBG("Loading half-track %d...\n", drive->half_track_no);  
    drive->rw_head.byte.hi = 
        drive->cur_track[CURRENT_BYTE(drive->bit_no)];
    drive->rw_head.byte.low = 
        drive->cur_track[NEXT_BYTE(drive->bit_no, drive->track_size)];
    drive->rw_head.shifter <<= 1; 
}

/**
 * Get the current bit underneath the read head and "spin" to the next bit.
 * If the next bit is in the next byte, load the next byte.  If the next
 * byte is at the beginning of the track, reset to beggining of track.
 * @param[in] drive Drive with the bit.
 * @return Bit read (least significant bit in byte)
 */
BYTE read_bit(struct drive_t* drive)
{
    BYTE bit = drive->rw_head.byte.hi & 0x01;

    // Store hi byte
    if (0x0007 == (drive->bit_no & 0x0007) && drive->write_clock != 0)
        drive->cur_track[CURRENT_BYTE(drive->bit_no)] = drive->rw_head.byte.hi;

    // "Spin" disk
    drive->bit_no = (drive->bit_no + 1) % (drive->track_size * 8);
    drive->rw_head.shifter <<= 1;
    drive->write_clock <<= 1;

    // Get low byte
    if (0x0007 == (drive->bit_no & 0x0007))
        drive->rw_head.byte.low = 
            drive->cur_track[NEXT_BYTE(drive->bit_no, drive->track_size)];

   return bit;
}

/**
 * Write the bit passed to the disk, and "spin" to the next bit.
 * @param[in] drive Drive to write bit.
 * @param[in] bit Bit to be written.
 * @return Bit just written.
 */
BYTE write_bit(struct drive_t* drive, BYTE bit)
{
    drive->rw_head.byte.hi = (drive->rw_head.byte.hi & 0xFE) | (bit & 0x01);
    drive->write_clock |= 0x01;
    return read_bit(drive); 
}

/**
 * Map the disk file into memory.  Set up drive structure.
 * @param[in] drive Drive to map.
 */
void map_disk(struct drive_t* drive)
{
    struct stat sb;
  
    if (drive->verbose)
        LOG_INF("Mapping disk...\n");
     
    if (fstat(drive->fd, &sb) == -1) {
        LOG_ERR("Err.\n");
        return;
    }

    drive->map = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, 
        MAP_SHARED, drive->fd, 0);
    
    if (drive->map == MAP_FAILED) {
        LOG_ERR("Err: %d\n", errno);
        return;
    }
    
    drive->map_size = sb.st_size;
    drive->track_cnt = 35;
    drive->track_size = drive->map_size / drive->track_cnt;
    if (drive->verbose)
        LOG_INF("Track size = %d\n", drive->track_size);
            drive->track_cnt = 70;
    drive->half_track_no = 0;
    load_half_track(drive); 
}

/**
 * Open the disk file and map it in memory.
 * @param[in] drive Drive to load.
 * @param[in] map_filename Full path of file to map in.
 * @see map_disk
 */ 
void load_disk(struct drive_t *drive, char *map_filename)
{

    if (drive->verbose)
        LOG_INF("Loading disk map file %s.\n", map_filename);
    
    drive->fd = open(map_filename, O_RDWR, S_IRUSR);
    if (drive->fd == -1) {
        LOG_ERR("Err.\n");
        return;
    }

    map_disk(drive);
}

/**
 * Create a file large enough to hold a new disk map.  
 * @param[in] drive Drive to load.
 * @param[in] tracks Number of tracks.
 * @param[in] track_size Size in bytes of the tracks.
 * @param[in] map_filename Full path of file to be created.
 * @see load_disk
 */
void create_disk(struct drive_t *drive, int tracks, size_t track_size,
    char *map_filename)
{
    BYTE *data;
    size_t total_size = tracks * track_size;
    ssize_t size_written;

    drive->fd = creat(map_filename, S_IRUSR | S_IWUSR);
    if (drive->fd == -1) {
        LOG_ERR("Err.\n");
        return;
    }

    data = malloc(total_size);

    size_written = write(drive->fd, data, total_size);
    if (size_written < total_size) {
        LOG_ERR("Err.\n");
        return;
    }

    free(data);
    close(drive->fd);
    load_disk(drive, map_filename);
}

void unload_disk(struct drive_t *drive)
{
    if (drive->map != NULL) { 
        if (drive->verbose)
            LOG_INF("Unloading disk...\n");
        munmap(drive->map, drive->map_size);
        close(drive->fd);
        drive->map = NULL;
    } else {
        LOG_INF("No disk to unload.\n");
    }
}
