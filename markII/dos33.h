#ifndef __DOS33_H
#define __DOS33_H
//#include "../6502.h"
#include "disk.h"

BYTE read_byte(struct drive_t *drive);
void write_byte(struct drive_t *drive, BYTE byte);
void write_sync_bytes(struct drive_t *drive, int count);
void write_sector(struct drive_t *drive, BYTE *sector);
void seek_sector(struct drive_t *drive, int sector_no);
void read_sector(struct drive_t *drive, BYTE sector[]);
void format_sector(struct drive_t *drive, BYTE volume, 
    BYTE track, BYTE sector);
void format_track(struct drive_t *drive, BYTE volume, BYTE track);
void format_disk(struct drive_t *drive, BYTE volume);
void move_rw_head(struct drive_t *drive, bool next_track);

void encode44(struct drive_t *drive, BYTE byte);
BYTE decode44(struct drive_t *drive);
void init_denibble();
void prenibble(BYTE *inbuf);
void postnibble(BYTE *outbuf);
#endif
