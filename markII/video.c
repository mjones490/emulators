#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "logging.h"
#include "config.h"
#include "cpu_iface.h"
#include "video.h"
#include "bus.h"
#include "mmu.h"

struct scan_line_t {
    WORD gfx_scan_line;
    WORD txt_scan_line;
    BYTE chr_line;
};

struct screen_t {
    SDL_Window          *window;
    SDL_Renderer        *renderer;
    SDL_Texture         *drawing_texture;
    Uint16              *pixels;
    int                 pitch;
    WORD                vsync;
    BYTE                hsync;
    BYTE                *hi_res_page[2];
    BYTE                *text_page[2];
    bool                page_2_select;
    bool                text_mode;
    bool                mixed;
    bool                vbl;
    bool                hires;
    bool                altchar;
    BYTE                *character_ROM;
    bool                flash;
    BYTE                flash_count;
    struct scan_line_t  scan_line[262];
};

struct screen_t screen;

static bool lock_drawing_texture()
{
    bool ret = true;
    if (0 > SDL_LockTexture(screen.drawing_texture, NULL, 
        (void *) &screen.pixels, &screen.pitch)) {
        LOG_ERR("Texture not locked. Error: %s\n", SDL_GetError());
        ret = false;
    } else {
        memset(screen.pixels, 0, 192 * screen.pitch);
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
        SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_STREAMING, 280 * 2, 192);
    
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

void finalize_video()
{
    LOG_INF("Finalizing video...\n");

    unlock_drawing_texture();
    SDL_DestroyTexture(screen.drawing_texture);
    SDL_DestroyRenderer(screen.renderer);
    SDL_DestroyWindow(screen.window);
    SDL_Quit();
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
    int pixel = (row * 280 * 2) + column * 7;
    int i;

    for (i = 0; i < 7; ++i) {
        screen.pixels[pixel + i] = (pattern & 0x01)? color : 0;
        pattern >>= 1;
    }
}

void plot(int row, int col, BYTE bitmask)
{
    Uint32 color = 65535;
    int pixel = (row * 560) + (col << 1);

    if (bitmask & 0x01)
        screen.pixels[pixel] = color;

    if (bitmask & 0x02)
        screen.pixels[pixel + 1] = color;
}

static void draw_hires()
{
    int i;
    BYTE *page = screen.hi_res_page[screen.page_2_select? 1 : 0];
    BYTE pattern = *(page + screen.scan_line[screen.vsync].gfx_scan_line +
        screen.hsync);
    WORD pixels = 0;

    for (i = 0; i < 7; ++i) {
        pixels = ((pattern & 0x01)? 0xc000 : 0x0000) | (pixels >> 2);
        pattern >>= 1;
    }

    pixels >>= (pattern & 0x01)? 1 : 2;
     
    for (i = 0; i < 8; ++i) {
        plot(screen.vsync, (7 * screen.hsync) + i, (pixels & 0x03));
        pixels >>= 2;
    }
}

static void draw_character()
{
    int i;
    BYTE *page = screen.text_page[screen.page_2_select? 1 : 0];
    BYTE character = *(page + screen.scan_line[screen.vsync].txt_scan_line + 
        screen.hsync);
    BYTE pattern_offset = screen.scan_line[screen.vsync].chr_line;
    BYTE pattern;
     
    if (!screen.altchar && ((character & 0xc0) == 0x40))
        character = (character & 0x3f) | (screen.flash? 0x80 : 0x00); 

    pattern = ~screen.character_ROM[(character << 3) + pattern_offset];
   
    for (i = 0; i < 7; ++i) {
        plot(screen.vsync, (7 * screen.hsync) + i, 3 * (pattern & 0x01));
        pattern >>= 1;
    }
}

static void draw_lores()
{
    int i;
    BYTE *page = screen.text_page[screen.page_2_select? 1 : 0];
    BYTE pattern = *(page + screen.scan_line[screen.vsync].txt_scan_line + 
        screen.hsync);
   
    if (!(screen.vsync & 0x04))
        pattern &= 0x0f;
    else
        pattern >>= 4;

    pattern |= (pattern << 4);
    if (!(screen.hsync & 0x01))
        pattern = (pattern << 1) | (pattern >> 7);

    for (i =0; i < 7; ++i) {
        plot(screen.vsync, (7 * screen.hsync) + i, pattern & 0x03);
        pattern = (pattern << 6) | (pattern >> 2);
    }
}

static void select_mode()
{
    if (screen.text_mode || (screen.mixed && screen.vsync >= 160)) {
        draw_character();
    } else {
        if (screen.hires)
            draw_hires();
        else
            draw_lores();
    }
}

void video_clock(BYTE clocks)
{
    while (--clocks) {
        if (screen.vsync < 192 && screen.hsync < 40) {
            select_mode();  
        }

        if (++screen.hsync == 65) {
            screen.hsync = 0;
            if (++screen.vsync == 262) {
                render_screen();
                screen.vsync = 0;
                if (++screen.flash_count == 15) {
                    screen.flash_count = 0;
                    screen.flash = !screen.flash;
                }   
            }

            if (screen.vsync < 192) {
                screen.vbl = false;
            } else {
                screen.vbl = true;
            }
        }
    }
}

/**
 * Video soft switches
 */
const BYTE SS_CLRALTCHAR    = 0x0e;
const BYTE SS_SETALTCHAR    = 0x0f;
const BYTE SS_RDVBL         = 0x19;
const BYTE SS_RDTEXT        = 0x1a;
const BYTE SS_RDMIXED       = 0x1b;
const BYTE SS_RDPAGE2       = 0x1c;
const BYTE SS_RDHIRES       = 0x1d;
const BYTE SS_RDALTCHAR     = 0x1e;
const BYTE SS_TXTCLR        = 0x50;
const BYTE SS_TXTSET        = 0x51;
const BYTE SS_MIXCLR        = 0x52;
const BYTE SS_MIXSET        = 0x53;
const BYTE SS_TXTPAGE1      = 0x54;
const BYTE SS_TXTPAGE2      = 0x55;
const BYTE SS_LORES         = 0x56;
const BYTE SS_HIRES         = 0x57;

BYTE ss_vbl(BYTE switch_no, bool read, BYTE value)
{
    return screen.vbl? 0x00 : 0x80;
}

static BYTE video_switch_read(BYTE switch_no, bool read, BYTE value)
{
    void *data = get_soft_switch_data(switch_no);
    return *((bool *) data)? 0x80 : 0x00;

}

static BYTE video_switch_clear(BYTE switch_no, bool read, BYTE value)
{
    void *data = get_soft_switch_data(switch_no);
    *((bool *) data) = false;
    return value;
}

static BYTE video_switch_set(BYTE switch_no, bool read, BYTE value)
{
    void *data = get_soft_switch_data(switch_no);
    *((bool *) data) = true;
    return value;
}

static void set_soft_switch(BYTE ss_read, BYTE ss_clear, BYTE ss_set, 
    bool *soft_switch)
{
    install_soft_switch(ss_read, SS_READ, video_switch_read);
    install_soft_switch(ss_clear, SS_RDWR, video_switch_clear);
    install_soft_switch(ss_set, SS_RDWR, video_switch_set);

    set_soft_switch_data(ss_read, soft_switch);
    set_soft_switch_data(ss_clear, soft_switch);
    set_soft_switch_data(ss_set, soft_switch);
}

static void install_video_soft_switches()
{

    LOG_INF("Installing video soft switches.\n");
    
    install_soft_switch(SS_RDVBL, SS_READ, ss_vbl);    
    set_soft_switch(SS_RDPAGE2, SS_TXTPAGE1, SS_TXTPAGE2, 
        &screen.page_2_select);
    set_soft_switch(SS_RDTEXT, SS_TXTCLR, SS_TXTSET, 
        &screen.text_mode);
    set_soft_switch(SS_RDMIXED, SS_MIXCLR, SS_MIXSET, 
        &screen.mixed);
    set_soft_switch(SS_RDHIRES, SS_LORES,SS_HIRES, 
        &screen.hires);
    set_soft_switch(SS_RDALTCHAR, SS_CLRALTCHAR, SS_SETALTCHAR, 
        &screen.altchar);

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

    video_config.window_title = get_config_string("MARKII", "TITLE");
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
    int i;
    struct page_block_t *pb;

    LOG_INF("Initializing screen.\n");
    configure_video();
         
    init_screen(video_config.width, video_config.height, 
        video_config.scale_quality, video_config.window_title);

    pb = get_page_block(0x2000);
    LOG_DBG("Hi res page block buffer = %p.\n", pb->buffer); 
    screen.hi_res_page[0] = &pb->buffer[pb_offset(pb, 0x2000)];
    LOG_DBG("Hi res page 1 address = %p.\n", screen.hi_res_page[0]);
    
    screen.hi_res_page[1] = &pb->buffer[pb_offset(pb, 0x4000)];
    LOG_DBG("Hi res page 2 address = %p.\n", screen.hi_res_page[1]);
    
    pb = get_page_block(0x0400);
    LOG_DBG("Text page block buffer = %p.\n", pb->buffer);
    screen.text_page[0] = &pb->buffer[pb_offset(pb, 0x0400)];
    LOG_DBG("Text page 1 address = %p.\n", screen.text_page[0]); 
    
    screen.text_page[1] = &pb->buffer[pb_offset(pb, 0x0800)];
    LOG_DBG("Text page 2 address = %p.\n", screen.text_page[1]); 
    
    // Install soft switches
    install_video_soft_switches();
    
    screen.text_mode = true;
    screen.mixed = true;
    screen.page_2_select = false;

    screen.character_ROM = load_ROM(video_config.video_ROM, 8);
    
    // Compute scanlines
    for(i = 0; i < 262; ++i) { 
        screen.scan_line[i].chr_line = i & 0x07;
        screen.scan_line[i].txt_scan_line = 0x28 * (i / 0x40) + 
            ((i & 0x38) << 4);
        screen.scan_line[i].gfx_scan_line = 
            (screen.scan_line[i].chr_line << 10) + 
            screen.scan_line[i].txt_scan_line;
    /*    LOG_DBG("Display line %d = %x, %4x, %4x\n", i,
            screen.scan_line[i].chr_line,
            screen.scan_line[i].txt_scan_line,
            screen.scan_line[i].gfx_scan_line);
*/
    }

    add_device(video_clock);

}




