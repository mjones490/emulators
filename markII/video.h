#ifndef __VIDEO_H_
#define __VIDEO_H_
#include <types.h>
#include <SDL.h>

void init_video();
void video_clock(BYTE clocks);
void finalize_video();

#endif
