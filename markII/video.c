#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "logging.h"
#include "config.h"
#include "video.h"
#include "bus.h"
#include "ROM.h"

struct screen_t {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *drawing_texture;
    Uint16 *pixels;
    int pitch;
    WORD vsync;
    BYTE hsync;
    BYTE *hi_res_page_1;
    BYTE *hi_res_page_2;
    BYTE *text_page_1;
    BYTE *text_page_2;
    bool page_2_select;
    WORD scan_line;
    bool vbl;
    BYTE *character_ROM;
};

struct screen_t screen;

static bool lock_drawing_texture()
{
    bool ret = true;
    if (0 > SDL_LockTexture(screen.drawing_texture, NULL, 
        (void *) &screen.pixels, &screen.pitch)) {
        LOG_ERR("Texture not locked. Error: %s\n", SDL_GetError());
        ret = false;
    }

    return ret;
}

static void unlock_drawing_texture()
{
    SDL_UnlockTexture(screen.drawing_texture);
}


static void init_screen(int w, int h, char *render_scale_quality, 
    char *window_title)
{
    if (0 > SDL_Init(SDL_INIT_VIDEO)) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        exit(1);
    }

    screen.window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (NULL == screen.window) {
        printf("Error creating main window: %s\n", SDL_GetError());
        exit(1);       
    }

    screen.renderer = SDL_CreateRenderer(screen.window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | 
        SDL_RENDERER_TARGETTEXTURE);
    if (NULL == screen.renderer) {
        printf("Error creating main renderer: %s\n", SDL_GetError());
        exit(1);       
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, render_scale_quality);
    SDL_RenderSetLogicalSize(screen.renderer, 280, 192);
    SDL_SetRenderDrawColor(screen.renderer, 0, 0, 0, 0);
    SDL_RenderClear(screen.renderer);

    // Create drawing texture
    screen.drawing_texture = SDL_CreateTexture(screen.renderer, 
        SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_STREAMING, 280, 192);
    
    if (screen.drawing_texture != NULL) {
        LOG_DBG("Drawing texture created.\n");
        if (lock_drawing_texture())
            LOG_DBG("Pitch = %d.\n", screen.pitch);
    } else {
        LOG_ERR("Error creating drawing texture: %s\n", SDL_GetError());
    }

    screen.vsync = 0;
    screen.hsync = 0;
}

static void render_screen()
{
    unlock_drawing_texture();
    SDL_RenderCopy(screen.renderer, screen.drawing_texture, NULL, NULL);
    SDL_RenderPresent(screen.renderer);
    lock_drawing_texture();
}

void draw_pattern(BYTE pattern, BYTE row, BYTE column)
{
    Uint32 color = 65535;
    int pixel = (row * 280) + column * 7;
    int i;

    for (i = 0; i < 7; ++i) {
        screen.pixels[pixel + i] = (pattern & 0x01)? color : 0;
        pattern >>= 1;
    }
}

void draw_character()
{
    WORD display_line = screen.scan_line & 0x3FF;
    BYTE character = screen.text_page_1[display_line + screen.hsync];
    BYTE pattern_offset = screen.scan_line >> 10;
    BYTE pattern = screen.character_ROM[(character << 3) + pattern_offset];
    draw_pattern(~pattern, screen.vsync, screen.hsync);
}

inline unsigned int scan_base(unsigned char line)
{
    return ((line & 0x07) << 10) + 0x28 * (line / 0x40) + 
        ((line & 0x38) << 4);
}

void video_clock(BYTE clocks)
{
    while (--clocks) {
        if (screen.vsync < 192 && screen.hsync < 40) {
            draw_character();  
            //if (screen.page_2_select)
            //    draw_pattern(screen.hi_res_page_2[screen.scan_line + 
            //      screen.hsync],screen.vsync, screen.hsync);
            //else
              //  draw_pattern(screen.hi_res_page_1[screen.scan_line + 
              //      screen.hsync],screen.vsync, screen.hsync);
        }

        if (++screen.hsync == 65) {
            screen.hsync = 0;
            if (++screen.vsync == 262) {
                render_screen();
                screen.vsync = 0;
            }

            if (screen.vsync < 192) {
                screen.vbl = false;
                screen.scan_line = scan_base(screen.vsync);
            } else {
                screen.vbl = true;
            }
        }
    }
}

/**
 * Video soft switches
 */
const BYTE SS_RDVBL     = 0x19;
const BYTE SS_RDPAGE2   = 0x1C;
const BYTE SS_TXTPAGE1  = 0x54;
const BYTE SS_TXTPAGE2  = 0x55;

BYTE ss_vbl(BYTE switch_no, bool read, BYTE value)
{
    return screen.vbl? 0x00 : 0x80;
}

BYTE ss_page_select(BYTE switch_no, bool read, BYTE value)
{
    if (switch_no == SS_RDPAGE2)
        value = screen.page_2_select? 0x80 : 0x00;
    else if (switch_no == SS_TXTPAGE1)
        screen.page_2_select = false;
    else
        screen.page_2_select = true;

    return value;
}

/**
 * Load video configuration setting.
 */

SDL_Texture *base_texture;

struct {
    int     width;
    int     height;
    char    *scale_quality;
    char    *video_ROM;
    char    *window_title;
} video_config;

void configure_video()
{
    char buf[256];
    char *tmp;

    tmp = get_config_string("VIDEO", "SCREEN_SIZE");
    if (NULL != tmp) {
        tmp = split_string(tmp, buf, 'x');
        video_config.width = string_to_int(buf);
        video_config.height = string_to_int(tmp);
    } else {
        LOG_WRN("SCREEN_SIZE not found.  Setting to default.\n");
    }
    LOG_INF("Screen size = %dx%d.\n", video_config.width, video_config.height);
   
    video_config.scale_quality = get_config_string("VIDEO", 
        "RENDER_SCALE_QUALITY");
    if (NULL == video_config.scale_quality) {
        LOG_WRN("RENDER_SCALE_QUALITY not found.  Setting to default.\n");
        video_config.scale_quality = "best";
    }
    LOG_INF("Render scale quality = %s.\n", video_config.scale_quality);

    video_config.window_title = get_config_string("6502", "TITLE");
    if (NULL == video_config.window_title) {
        LOG_WRN("TITLE not found.  Setting to default.\n");
        video_config.window_title = "";
    }
    LOG_INF("Title = \"%s\".\n", video_config.window_title);
     
    video_config.video_ROM = get_config_string("VIDEO", "VIDEO_ROM");
    if (NULL == video_config.video_ROM) 
        LOG_FTL("VIDEO_ROM not found.\n");
}
/**
 * @brief Initialize video subsystem.
 *
 * Set up screen, load video ROM, allocate RAM, and setup soft switches.
 * Call 80-col initialization. 
 */
void init_video()
{   
    struct page_block_t *pb;

    LOG_INF("Initializing screen.\n");
    configure_video();
         
    init_screen(video_config.width, video_config.height, 
        video_config.scale_quality, video_config.window_title);

    pb = get_page_block(0x2000);
    LOG_DBG("Hi res page block buffer = %p.\n", pb->buffer); 
    screen.hi_res_page_1 = &pb->buffer[pb_offset(pb, 0x2000)];
    LOG_DBG("Hi res page 1 address = %p.\n", screen.hi_res_page_1);
    
    screen.hi_res_page_2 = &pb->buffer[pb_offset(pb, 0x4000)];
    LOG_DBG("Hi res page 2 address = %p.\n", screen.hi_res_page_2);
    
    pb = get_page_block(0x0400);
    LOG_DBG("Text page block buffer = %p.\n", pb->buffer);
    screen.text_page_1 = &pb->buffer[pb_offset(pb, 0x0400)];
    LOG_DBG("Text page 1 address = %p.\n", screen.text_page_1); 
    
    screen.scan_line = scan_base(0);

    // Install soft switches
    install_soft_switch(SS_RDVBL, SS_READ, ss_vbl);    
    install_soft_switch(SS_RDPAGE2, SS_READ, ss_page_select);
    install_soft_switch(SS_TXTPAGE1, SS_RDWR, ss_page_select);
    install_soft_switch(SS_TXTPAGE2, SS_RDWR, ss_page_select);

    screen.character_ROM = load_ROM(video_config.video_ROM, 8);
    LOG_DBG("First byte = %02x.\n", screen.character_ROM[0]);
}


