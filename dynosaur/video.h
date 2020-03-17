#ifndef __VIDEO_H
#define __VIDEO_H

BYTE *init_video();
void finalize_video();
void refresh_video();

#define VDP_RAM_SIZE ((24*40)+(256*8))
#endif
