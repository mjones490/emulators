/**
 * @file video.c
 * Video
 */

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

/**
 * Holds translations of display lines to starting addresses in memory
 * and character generation ROM.
 */
struct scan_line_t {
    WORD gfx_scan_line;  ///< Graphics scan line
    WORD txt_scan_line;  ///< Text scan line
    BYTE chr_line;       ///< Character generation ROM line
};

Uint32 color[16];

/**
 * Data for current state of the display
 */
struct screen_t {
    SDL_Window          *window;            ///< Main window
    SDL_Renderer        *renderer;          ///< Screen renderer
    SDL_Texture         *drawing_texture;   ///< Drawing texture
    Uint16              *pixels;            ///< Pointer to raw pixels 
    int                 pitch;              ///< Width of pixels in bytes
    WORD                vsync;              ///< Vertical sync counter
    BYTE                hsync;              ///< Horizontal sync counter
    BYTE                *hi_res_page[2];    ///< Hires page RAM
    BYTE                *text_page[2];      ///< Text/lores page RAM
    bool                page_2_select;      ///< Page 2 is selected
    bool                text_mode;          ///< Text mode
    bool                mixed;              ///< Mixed (graphics and text) mode
    bool                vbl;                ///< In vertical blank state
    bool                hires;              ///< Hi res graphics mode
    bool                altchar;            ///< Alt character set selected
    BYTE                *character_ROM;     ///< Character generator ROM
    bool                flash;              ///< Flash state
    BYTE                flash_count;        ///< Flash frame counter
    struct scan_line_t  scan_line[262];     ///< Scan/display line xlate table
    Uint32              last_render_time;   ///< Last time screen was rendered
    bool                is_dirty;
};

struct screen_t screen; ///< Current screen state

/**
 * @brief Lock drawing texture so pixels can be accessed.
 */
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

/**
 * Unlock drawing texture so screen can be drawn.
 */
static void unlock_drawing_texture()
{
    SDL_UnlockTexture(screen.drawing_texture);
}

/**
 * Render the screen at start of VBL.
 */
static inline void render_screen()
{
    unlock_drawing_texture();
    SDL_RenderCopy(screen.renderer, screen.drawing_texture, NULL, NULL);
    screen.is_dirty = true;
    lock_drawing_texture();
}

/**
 * Update the screen if a new one has been rendered and enough
 * time has passed.
 */
static inline void update_screen()
{
    Uint32 timer = SDL_GetTicks();
    Uint32 timer_diff = timer - screen.last_render_time;

    if (screen.is_dirty && 17 <= timer_diff) {
        screen.is_dirty = false;
        screen.last_render_time = timer;
        SDL_RenderPresent(screen.renderer);
        ++screen.flash_count;
    }
}

/**
 * Plot two pixels to the screen.
 * @param[in] row Display row to plot to
 * @param[in] col Display column of first pixel
 * @param[in] bitmask Bitmask representing the pixels.  First pixel is bit 0,
 *  second pixes is bit 1.
 */
inline void plot(int row, int col, BYTE color_ndx)
{
    int pixel;
    
    if (col >= 0) {    
        pixel = (row * 560) + col;
        screen.pixels[pixel] = color[color_ndx];
    }
}

inline void wplot(int row, int col, BYTE color_ndx)
{
    plot(row, col, color_ndx);
    plot(row, col + 1, color_ndx);
}

const Uint32 hi_color[] = {3, 12, 6, 9}; 
/**
 * Plot seven hi-res pixels to screen.
 */
static void draw_hires()
{
    static WORD vsync;
    int i;
    BYTE *page = screen.hi_res_page[screen.page_2_select? 1 : 0];
    BYTE pattern = *(page + screen.scan_line[screen.vsync].gfx_scan_line +
        screen.hsync);
    int o = (pattern & 0x80)? 2 : 0;
    int base_col = 14 * screen.hsync;

    static Uint32 color_0;
    static Uint32 color_1;
    static bool prev_pixel;
    static bool cur_pixel;

    if (vsync != screen.vsync) {
        vsync = screen.vsync;
        prev_pixel = false;
    }
     

    for (i = 0; i < 7; ++i) {
        color_0 = color_1;
        color_1 = 0;
        cur_pixel = pattern & 0x01;
        if (cur_pixel) {
            if (prev_pixel) {
               color_1 = 15;
               color_0 = 15;
            } else        
               color_1 = hi_color[o + ((screen.hsync + i) & 0x01)];            
            //color_0 = color_1;
        } else {
            if (prev_pixel) {
                //if (color_0 != 15)
                    color_1 = color_0;
                //else 
                  //  color_1 = 0;
            }
            else color_1 = 0;
            color_0 = 0;
        }
     
        if (color_0 > 0)
            wplot(screen.vsync, o + base_col + (2 * i) - 2, color_0); 
        wplot(screen.vsync, o + base_col + (2 * i), color_1); 
        prev_pixel = cur_pixel;
        pattern >>= 1;
    }
}

/**
 * Plot one row of pixels for one character to the screen.
 */
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
        if (pattern & 0x01) {
            wplot(screen.vsync, (14 * screen.hsync) + i * 2, 15);
        }
        pattern >>= 1;
    }
}

/**
 * Plot one row of pixels for one lo-res block to the screen.
 */
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

    //pattern |= (pattern << 4);
    //if (!(screen.hsync & 0x01))
    //    pattern = (pattern << 1) | (pattern >> 7);

    for (i =0; i < 14; ++i) {
        plot(screen.vsync, (14 * screen.hsync) + i, pattern);
      //  pattern = (pattern << 7) | (pattern >> 1);
    }
}

/**
 * Determine mode and call the proper plotting function.
 */
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

/**
 * For each cpu clock, process one byte of video memory.  Handle sync counters
 * and blanking signals.  Render screen during vertical blanking.
 * param[in] clocks Number of clocks to process.
 */
void video_clock(BYTE clocks)
{
    while (--clocks) {
        if (screen.vsync < 192 && screen.hsync < 40) {
            select_mode();  
        }

        if (++screen.hsync == 65) {
            screen.hsync = 0;
            if (++screen.vsync == 262) {
                screen.vsync = 0;
            }

            if (screen.vsync < 192) {
                screen.vbl = false;
            } else if (!screen.vbl) {
                screen.vbl = true;
                render_screen();
                if (screen.flash_count == 15) {
                    screen.flash_count = 0;
                    screen.flash = !screen.flash;
                }   
            }
        }

        update_screen();
    }
}

/**
 * Video soft switches
 */
const BYTE SS_CLRALTCHAR    = 0x0e; ///< Clear alternate character set
const BYTE SS_SETALTCHAR    = 0x0f; ///< Select alternate character set
const BYTE SS_RDVBL         = 0x19; ///< Read vertical blanking state
const BYTE SS_RDTEXT        = 0x1a; ///< Reat text mode selection state
const BYTE SS_RDMIXED       = 0x1b; ///< Read mixed mode selection state
const BYTE SS_RDPAGE2       = 0x1c; ///< Read page 2 selection state
const BYTE SS_RDHIRES       = 0x1d; ///< Read hi-res graphics mode state
const BYTE SS_RDALTCHAR     = 0x1e; ///< Read alternate charcter set state
const BYTE SS_TXTCLR        = 0x50; ///< Clear text mode
const BYTE SS_TXTSET        = 0x51; ///< Set text mode
const BYTE SS_MIXCLR        = 0x52; ///< Clear mixed mode
const BYTE SS_MIXSET        = 0x53; ///< Set mixed mode
const BYTE SS_TXTPAGE1      = 0x54; ///< Select page one
const BYTE SS_TXTPAGE2      = 0x55; ///< Select page two
const BYTE SS_LORES         = 0x56; ///< Select lo-res graphics
const BYTE SS_HIRES         = 0x57; ///< Select hi-res graphics

/**
 * Read vertical blanking state
 * @param[in] switch_no Switch number to access
 * @param[in] read Read switch when true.  Otherwise write.
 * @param[in] value Value to write
 * @returns Value read 
 */
BYTE ss_vbl(BYTE switch_no, bool read, BYTE value)
{
    return screen.vbl? 0x00 : 0x80;
}

/**
 * Read state of video switch
 * @param[in] switch_no Switch number to access
 * @param[in] read Read switch when true.  Otherwise write.
 * @param[in] value Value to write
 * @returns Value read 
 */
static BYTE video_switch_read(BYTE switch_no, bool read, BYTE value)
{
    void *data = get_soft_switch(switch_no)->data;
    return *((bool *) data)? 0x80 : 0x00;

}

/**
 * Clear video switch
 * @param[in] switch_no Switch number to access
 * @param[in] read Read switch when true.  Otherwise write.
 * @param[in] value Value to write
 * @returns Value read 
 */
static BYTE video_switch_clear(BYTE switch_no, bool read, BYTE value)
{
    void *data = get_soft_switch(switch_no)->data;
    *((bool *) data) = false;
    return value;
}

/**
 * Set video switch
 * @param[in] switch_no Switch number to access
 * @param[in] read Read switch when true.  Otherwise write.
 * @param[in] value Value to write
 * @returns Value read 
 */
static BYTE video_switch_set(BYTE switch_no, bool read, BYTE value)
{
    void *data = get_soft_switch(switch_no)->data;
    *((bool *) data) = true;
    return value;
}

/**
 * @name Initialization
 * @{
 */

/**
 * Set up indvidual video soft switch to use the read, set, and clear
 * functions.
 * @param[in] ss_read Switch number to read switch state
 * @param[in] ss_clear Switch number to clear switch state
 * @param[in] ss_set Switch number to set switch state
 * @param[in] soft_switch Address of boolean varialble representing switch
 *              state
 */
static void set_soft_switch(BYTE ss_read, BYTE ss_clear, BYTE ss_set, 
    bool *soft_switch)
{
    struct soft_switch_t *ss;

    ss = install_soft_switch(ss_read, SS_READ, video_switch_read);
    ss->data = soft_switch;

    ss = install_soft_switch(ss_clear, SS_RDWR, video_switch_clear);
    ss->data = soft_switch;

    ss = install_soft_switch(ss_set, SS_RDWR, video_switch_set);
    ss->data = soft_switch;
}

/**
 * @brief Install all video soft switches.
 */
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


SDL_Texture *base_texture;

struct {
    int     width;
    int     height;
    char    *scale_quality;
    char    *video_ROM;
    char    *window_title;
} video_config;

/**
 * @brief Load video configuration setting.
 */
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
 * Initialize SDL.  Setup main window, renderer, and drawing texture.
 * @param[in] w Width of main window
 * @param[in] h Height of main window
 * @param[in] render_scale_quality Quality of scaling pixel size to main window
 * @param[in] window_title String to display at top of main window.
 */
static void init_screen(int w, int h, char *render_scale_quality, 
    char *window_title)
{
    SDL_PixelFormat *format;

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
        SDL_PIXELFORMAT_RGB444, SDL_TEXTUREACCESS_STREAMING, 280 * 2, 192);
    
    if (screen.drawing_texture != NULL) {
        LOG_DBG("Drawing texture created.\n");
        if (lock_drawing_texture())
            LOG_DBG("Pitch = %d.\n", screen.pitch);
    } else {
        LOG_ERR("Error creating drawing texture: %s\n", SDL_GetError());
    }

    format = SDL_AllocFormat(SDL_PIXELFORMAT_RGB444);
    color[0] = SDL_MapRGB(format, 0x00, 0x00, 0x00);    ///< Black
    color[1] = SDL_MapRGB(format, 0xcc, 0x00, 0x33);    ///< Magenta
    color[2] = SDL_MapRGB(format, 0x00, 0x00, 0x99);    ///< Dark Blue
    color[3] = SDL_MapRGB(format, 0xcc, 0x33, 0xcc);    ///< Purble 
    color[4] = SDL_MapRGB(format, 0x00, 0x66, 0x33);    ///< Dark Green
    color[5] = SDL_MapRGB(format, 0x66, 0x66, 0x66);    ///< Dark Grey
    color[6] = SDL_MapRGB(format, 0x33, 0x33, 0xff);    ///< Med Blue
    color[7] = SDL_MapRGB(format, 0x66, 0x99, 0xff);    ///< Light Blue
    color[8] = SDL_MapRGB(format, 0x99, 0x66, 0x00);    ///< Brown
    color[9] = SDL_MapRGB(format, 0xff, 0x66, 0x00);    ///< Orange 
    color[10] = SDL_MapRGB(format, 0x99, 0x99, 0x99);   ///< Light Grey
    color[11] = SDL_MapRGB(format, 0xff, 0x99, 0x99);   ///< Pink 
    color[12] = SDL_MapRGB(format, 0x00, 0xcc, 0x00);   ///< Light Green 
    color[13] = SDL_MapRGB(format, 0xff, 0xff, 0x00);   ///< Yellow
    color[14] = SDL_MapRGB(format, 0x33, 0xff, 0x99);   ///< Aqua
    color[15] = SDL_MapRGB(format, 0xff, 0xff, 0xff);   ///< White 
    SDL_FreeFormat(format);

    screen.vsync = 0;
    screen.hsync = 0;
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

/**
 * Unlock drawing texture and release SDL resources.
 */
void finalize_video()
{
    LOG_INF("Finalizing video...\n");

    unlock_drawing_texture();
    SDL_DestroyTexture(screen.drawing_texture);
    SDL_DestroyRenderer(screen.renderer);
    SDL_DestroyWindow(screen.window);
    SDL_Quit();
}

/*@}*/



