#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "types.h"
#include "logging.h"
#include "Stuff.h"
#include "config.h"

struct {
    SDL_Window      *window;
    SDL_Renderer    *renderer;
    SDL_Texture     *texture;
    Uint16          *pixels;
    struct {
        BYTE        screen[24][40];  // 8000
        BYTE        color_table[16]; // 83c0
        BYTE        free_space[48];  // 83d0
        BYTE        pattern_table[256][8]; // 8400
    } vdp_ram;
    Uint16          color[16];
    Uint32          timer;
    char            *title;
    int             width;
    int             height;
} video;


void draw_char(int row, int col, BYTE char_no)
{
    int i, j;
    BYTE pattern;
    BYTE foreground = video.vdp_ram.color_table[char_no >> 3] >> 4;
    BYTE background = video.vdp_ram.color_table[char_no >> 3] & 0x0f;
    row *= 8;
    col *= 8;
    for (i = 0; i < 8; i++) {
        pattern = video.vdp_ram.pattern_table[char_no][i];
        for (j = 0; j < 8; j++) {
            video.pixels[((row + i) * 320) + (j + col)] =  video.color[(pattern & 0x80) ? foreground : background];
            pattern <<= 1;
        }
    }

}

void refresh_video()
{
    int pitch;
    Uint32 current_timer = SDL_GetTicks();

    if ((current_timer - video.timer) < 34)
        return;

    video.timer = current_timer;

    if (SDL_LockTexture(video.texture, NULL, (void *) &video.pixels, &pitch))
        LOG_ERR("Error locking SDL texture: %s", SDL_GetError());

    memset(video.pixels, 0, 192 * pitch);

    int i, j, ch;
    for (i = 0; i < 24; i++)
        for (j = 0; j < 40; j++) {
            ch = video.vdp_ram.screen[i][j];
            draw_char(i, j, ch);
        }

    SDL_UnlockTexture(video.texture);

    SDL_RenderCopy(video.renderer, video.texture, NULL, NULL);
    SDL_RenderPresent(video.renderer);

}

static void load_char_set()
{
    struct section_struct *section;
    struct key_struct *key;
    struct list_head *list;
    struct list_head *first;
    BYTE   char_code;
    BYTE hex[8];
    
    section = get_config_section("VIDEO");
    if (section == NULL) {
        LOG_ERR("Could not find VIDEO configuration section");
        return;
    }

    key = section->key;
    LIST_FOR_EVERY(list, &section->key->list, first) {
        key = GET_ELEMENT(struct key_struct, list, list);
        if (1 == sscanf(key->name, "C%02hhX", &char_code)) {
            LOG_DBG("%s=%s\n", key->name, key->value);
            sscanf(key->value, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", 
                &hex[0], &hex[1], &hex[2], &hex[3],
                &hex[4], &hex[5], &hex[6], &hex[7]);
            memcpy(video.vdp_ram.pattern_table[char_code], hex, 8);
        }
    }
}

static void load_config()
{
    char buf[256];
    char *tmp;

    tmp = get_config_string("VIDEO", "SCREEN_SIZE");
    if (NULL != tmp) {
        tmp = split_string(tmp, buf, 'x');
        video.width = string_to_int(buf);
        video.height = string_to_int(tmp);
    } else {
        LOG_WRN("SCREEN_SIZE not found.  Setting to default.\n");
    }
    LOG_INF("Screen size = %dx%d.\n", video.width, video.height);
 
    video.title = get_config_string("VIDEO", "TITLE");
    if (NULL == video.title) {
        LOG_WRN("TITLE not found.  Setting to default.\n");
        video.title = "";
    }
    LOG_INF("Title = \"%s\".\n", video.title);
    load_char_set();
}

BYTE *init_video()
{
    SDL_PixelFormat *format;
    LOG_INF("Initilizing video.\n");
    load_config();
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERR("Error initializing SDL: %s", SDL_GetError());
        return 0;
    }

    video.window = SDL_CreateWindow(video.title, SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, video.width, video.height, SDL_WINDOW_SHOWN);
    if (video.window == NULL) {
        LOG_ERR("Error creating SDL window: %s", SDL_GetError());
        return 0;
    }

    SDL_RaiseWindow(video.window);
    video.renderer = SDL_CreateRenderer(video.window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (video.window == NULL) {
        LOG_ERR("Error creating SDL renderer: %s", SDL_GetError());
        return 0;
    }

    SDL_SetRenderDrawColor(video.renderer, 0, 0, 0, 0);
    SDL_RenderClear(video.renderer);
    SDL_RenderPresent(video.renderer);

    video.texture = SDL_CreateTexture(video.renderer, 
        SDL_PIXELFORMAT_RGB444, SDL_TEXTUREACCESS_STREAMING, 320, 192);

    if (video.texture == NULL) { 
        LOG_ERR("Error creating SDL texture: %s", SDL_GetError());
        return 0;
    }

    format = SDL_AllocFormat(SDL_PIXELFORMAT_RGB444);
    video.color[0]  = SDL_MapRGB(format, 0x00, 0x00, 0x00); // Transparent
    video.color[1]  = SDL_MapRGB(format, 0x00, 0x00, 0x00); // Black
    video.color[2]  = SDL_MapRGB(format, 0x21, 0xc8, 0x42); // Green
    video.color[3]  = SDL_MapRGB(format, 0x5e, 0xdc, 0x78); // Light green
    video.color[4]  = SDL_MapRGB(format, 0x54, 0x55, 0xed); // Dark blue
    video.color[5]  = SDL_MapRGB(format, 0x7d, 0x76, 0xfc); // Ligh blue
    video.color[6]  = SDL_MapRGB(format, 0xd4, 0x52, 0x4d); // Dark red
    video.color[7]  = SDL_MapRGB(format, 0x42, 0xeb, 0xf5); // Cyan
    video.color[8]  = SDL_MapRGB(format, 0xfc, 0x55, 0x54); // Red
    video.color[9]  = SDL_MapRGB(format, 0xff, 0x79, 0x78); // Light red
    video.color[10] = SDL_MapRGB(format, 0xd4, 0xc1, 0x54); // Dark yellow
    video.color[11] = SDL_MapRGB(format, 0xe6, 0xce, 0x80); // Light yellow
    video.color[12] = SDL_MapRGB(format, 0x21, 0xb0, 0x3b); // Dark green
    video.color[13] = SDL_MapRGB(format, 0xc9, 0x5b, 0xba); // Magenta
    video.color[14] = SDL_MapRGB(format, 0xcc, 0xcc, 0xcc); // Gray
    video.color[15] = SDL_MapRGB(format, 0xff, 0xff, 0xff); // White
    SDL_FreeFormat(format);

    memset(video.vdp_ram.color_table, 0x17, 32);
    refresh_video();

    return (BYTE *) &video.vdp_ram;
}

void finalize_video()
{
    LOG_INF("Finalizing video.\n");

    SDL_DestroyWindow(video.window);
    SDL_Quit();
}
