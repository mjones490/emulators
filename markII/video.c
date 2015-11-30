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

struct screen_t {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *drawing_texture;
    Uint16 *pixels;
    int pitch;
    WORD vsync;
    BYTE hsync;
    BYTE *hi_res_page_1;
    WORD scan_line;
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

inline unsigned int scan_base(unsigned char line)
{
    return ((line & 0x07) << 10) + 0x28 * (line / 0x40) + 
        ((line & 0x38) << 4);
}

void video_clock(BYTE clocks)
{
    while (--clocks) {
        if (screen.vsync < 192 && screen.hsync < 40)
            draw_pattern(screen.hi_res_page_1[screen.scan_line + screen.hsync],
                screen.vsync, screen.hsync);

        if (++screen.hsync == 65) {
            screen.hsync = 0;
            if (++screen.vsync == 262) {
                render_screen();
                screen.vsync = 0;
            }

            if (screen.vsync < 192)
                screen.scan_line = scan_base(screen.vsync);
        }
    }
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
    
    /* 
    video_config.video_ROM = get_config_string("VIDEO", "VIDEO_ROM");
    if (NULL == video_config.video_ROM) 
        LOG_FTL("VIDEO_ROM not found.\n");
        */
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
    LOG_DBG("Hi res page 1 page block buffer = %p.\n", pb->buffer); 
    screen.hi_res_page_1 = &pb->buffer[pb_offset(pb, 0x2000)];
    LOG_DBG("Hi res page 1 address = %p.\n", screen.hi_res_page_1);
    
    screen.scan_line = scan_base(0);
     
    // Init base texture
    //LOG_INF("Initializing base texture.\n");
    //base_texture = init_base_texture();

    //draw_test_surface();
/*
    // Load video ROM.
    hd = load_ROM(video_config.video_ROM, 8);
    text_40.texture = init_ROM_texture(base_texture, hd->heap);
    text_40.scale = 1;

    GR.texture = init_GR_texture(base_texture);
    GR.scale = 1;
    
    // Allocate RAM pages
    LOG_INF("Setting video page RAM.\n");
    hd = alloc_heap(0x04);
    page_RAM[0] = set_page_descripter(hd, 0x0000, 
        video_RAM_access, video_RAM_access);
    set_page_block(&block_set.blk_0400, page_RAM[0]);
    
    current_page_RAM = page_RAM[0];
    hd = alloc_heap(0x04);
    page_RAM[1] = set_page_descripter(hd, 0x0000, 
       video_RAM_access, video_RAM_access);
    set_page_block(&block_set.blk_0800, page_RAM[1]);

    // Create row base map    
    text_40.row_map = malloc(24 * sizeof(WORD));
    for (i = 0; i < 24; ++i) {
        text_40.row_map[i] = ((i /8) * 0x28) + ((i % 8) * 0x80);
        LOG_DBG("Row = %d  Base Address = $%04X.\n", i, text_40.row_map[i]);
    }
   
    // Set up soft switches
    ss_page_2 = set_soft_switch(RDPAGE2, TXTPAGE2, TXTPAGE1, false, 
        page_switch, SS_RW | SS_NOTIFY_SET | SS_NOTIFY_CLR);
    ss_alt_char = set_soft_switch(RDALTCHAR, SETALTCHAR, CLRALTCHAR, false,
        NULL, SS_WO);    
    ss_mixed = set_soft_switch(RDMIXED, MIXSET, MIXCLR, false, NULL, SS_RW);
    ss_text = set_soft_switch(RDTEXT, TXTSET, TXTCLR, true, NULL, SS_RW);
    ss_hires = set_soft_switch(RDHIRES, HIRES, LORES, false, NULL, SS_RW); 
    init_device(video_refresh_clock);

    init_80_col();
*/
}


