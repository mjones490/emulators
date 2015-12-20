#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "dos33.h"

BYTE lbuf[256];
BYTE sbuf[86];

BYTE nibble[] = {
    0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6,
    0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,

    0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC,
    0xBD, 0xBE, 0xBF, 0xCB, 0xCD, 0xCE, 0xCF, 0xD3,

    0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE,
    0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,

    0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
    0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

BYTE denibble[128];
BYTE read_byte(struct drive_t *drive)
{
    BYTE byte = 0;

    while ((byte & 0x80) == 0) {
        byte = (byte << 1) | read_bit(drive);
    }

    return byte;
}

void write_byte(struct drive_t *drive, BYTE byte)
{
    int i;

    for (i = 0; i < 8; ++i) {
        write_bit(drive, byte >> 7);
        byte <<= 1;
    }

}

void move_rw_head(struct drive_t *drive, bool next_track)
{
    int cur_phase = (drive->half_track_no << 1) & 0x07;
  /* 
    read_byte(drive);
    read_byte(drive);
    read_byte(drive);
    read_byte(drive);
    read_byte(drive);
*/
    set_phase(drive, cur_phase);
    
    if (next_track) 
        set_phase(drive, (cur_phase + 3) & 7);
    else 
        set_phase(drive, (cur_phase - 1) & 7);
/*
    read_byte(drive);
    read_byte(drive);
    read_byte(drive);
    read_byte(drive);
    read_byte(drive);
*/
}

void write_sync_bytes(struct drive_t *drive, int count)
{
    int i;

    for (i = 0; i < count; ++i) {
        write_bit(drive, 0);
        write_bit(drive, 0);
        write_byte(drive, 0xFF);
    }
}

void write_sector(struct drive_t *drive, BYTE *sector)
{
    int i;
    BYTE prev_nibble = 0;

    memset(sbuf, 0, sizeof(sbuf));
    memset(lbuf, 0, sizeof(lbuf));

    prenibble(sector);

    memset(sector, 0, sizeof(lbuf));
    
    write_sync_bytes(drive, 8);
    write_byte(drive, 0xD5);
    write_byte(drive, 0xAA);
    write_byte(drive, 0xAD);

    for (i = sizeof(sbuf) - 1; i >= 0; --i) {        
        write_byte(drive, nibble[sbuf[i] ^ prev_nibble]);
        prev_nibble = sbuf[i];
    }

    for (i = 0; i < sizeof(lbuf); ++i) {
        write_byte(drive, nibble[lbuf[i] ^ prev_nibble]);
        prev_nibble = lbuf[i];
    }

    write_byte(drive, nibble[prev_nibble]);

    write_byte(drive, 0xDE);
    write_byte(drive, 0xAA);
    write_byte(drive, 0xEB);
    write_sync_bytes(drive, 8);
}

void seek_sector(struct drive_t *drive, int sector_no)
{
    BYTE volume;
    BYTE track;
    BYTE sector;
    BYTE checksum;

    while(read_byte(drive) != 0xFF);

    while (true) {
        while (read_byte(drive) != 0xD5);
        if (read_byte(drive) != 0xAA) continue;
        if (read_byte(drive) != 0x96) continue;

        volume = decode44(drive);
        track = decode44(drive);
        sector = decode44(drive);
        checksum = decode44(drive);

    //    printf("volume = %d  track = %d  sector = %d\n",
      //      volume, track, sector);

        if (checksum != (BYTE)(volume ^ track ^ sector))
            continue;

        if (read_byte(drive) != 0xDE) continue;
        if (read_byte(drive) != 0xAA) continue;
        
        if (sector_no ==  sector) {
            break;      
        }
    }   
}

void read_sector(struct drive_t *drive, BYTE sector[])
{
    BYTE prev_nibble = 0;
    int i;
    BYTE c = 0;
    bool good_read = false;

    memset(sbuf, 0, sizeof(sbuf));
    memset(lbuf, 0, sizeof(lbuf));
    
    while (read_byte(drive) != 0xFF);
    while (read_byte(drive) != 0xD5);

    while (true) { 
        if (read_byte(drive) != 0xAA)
            break;

        if (read_byte(drive) != 0xAD)
            break;

        for (i = sizeof(sbuf) - 1; i >= 0; --i) {
            sbuf[i] = denibble[read_byte(drive) & 0x7F] ^ prev_nibble;
            prev_nibble = sbuf[i];
        }

        for (i = 0; i < sizeof(lbuf); ++i) {
            lbuf[i] = denibble[read_byte(drive) & 0x7F] ^ prev_nibble;
            prev_nibble = lbuf[i];
        }
    
        c = read_byte(drive);
        
        if (read_byte(drive) != 0xDE)
            break;

        if (read_byte(drive) != 0xAA)
            break;

        if (read_byte(drive) != 0xEB)
            break;

        if ((denibble[c & 0x7F] ^ prev_nibble) != 0) {
            printf("Bad checksum.\n");
            break;
        }

        good_read = true;

        postnibble(sector);

    } 
    
    if (!good_read) 
        printf("Err.\n");

}

BYTE zeros[256]; 

void format_sector(struct drive_t *drive, BYTE volume, BYTE track, BYTE sector)
{
    // Address
    write_byte(drive, 0xD5);
    write_byte(drive, 0xAA);
    write_byte(drive, 0x96);
    encode44(drive, volume);
    encode44(drive, track);
    encode44(drive, sector);
    encode44(drive, volume ^ track ^ sector);
    write_byte(drive, 0xDE);
    write_byte(drive, 0xAA);
    write_byte(drive, 0xEB);

    // Data
    write_sector(drive, zeros);
    write_sync_bytes(drive, 8);
}

void format_track(struct drive_t *drive, BYTE volume, BYTE track)
{
    int i;
    
    memset(zeros, 0x00, sizeof(lbuf));
    
    write_sync_bytes(drive, 0x80);

    for (i = 0; i < 16; ++i) 
        format_sector(drive, volume, track, i);
}

void format_disk(struct drive_t *drive, BYTE volume)
{
    int i;

    for (i = 0; i < drive->track_cnt; i += 2) {
        if (i > 0) {
            move_rw_head(drive, true);
            move_rw_head(drive, true);
        }
        format_track(drive, volume, i / 2);
    }
}

/***************************************************************
 * Nibblizing routines
 ***************************************************************/
void encode44(struct drive_t *drive, BYTE byte)
{ 
    write_byte(drive, (byte >> 1) | 0xAA);
    write_byte(drive, byte | 0xAA);
}

BYTE decode44(struct drive_t *drive)
{
    return (read_byte(drive) << 1 | 1) & read_byte(drive);
}

void init_denibble()
{
    int i;
    for (i = 0; i < sizeof(nibble); ++i)
        denibble[nibble[i] & 0x7F] = i;
}

BYTE lbuf[256];
BYTE sbuf[86];

#define SWAPBITS(B) (((B & 0x01) << 1) | (((B & 0x02) >> 1)))

void prenibble(BYTE *inbuf)
{
    int i;
    int j = 0;

    for (i = sizeof(lbuf) + 1; i >= 0; --i) {
        lbuf[i % sizeof(lbuf)] = inbuf[i % sizeof(lbuf)] >> 2;
        sbuf[j] = (sbuf[j] << 2) | SWAPBITS(inbuf[i]);
        if (++j == sizeof(sbuf))
            j = 0;
    }
}

void postnibble(BYTE *outbuf)
{
    int i;
    int j = sizeof(sbuf) - 1;
    
    for (i = 0; i < sizeof(lbuf); ++i) {
        outbuf[i] = (lbuf[i] << 2) | SWAPBITS(sbuf[j]);
        sbuf[j] >>= 2;
        if (j == 0)
            j = sizeof(sbuf);
        --j;
    }
}

