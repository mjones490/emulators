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
    BYTE            vdp_ram[24][40];
    Uint32          timer;
    char            *title;
    int             width;
    int             height;
} video;

BYTE charset['Z' - ' '][8];

const size_t charset_size = sizeof(charset) / 8;

void draw_char(int row, int col, BYTE char_no)
{
    int i, j;
    BYTE pattern;
    row *= 8;
    col *= 8;
    for (i = 0; i < 8; i++) {
        pattern = charset[char_no][i];
        for (j = 0; j < 8; j++) {
            video.pixels[((row + i) * 320) + (j + col)] = (pattern & 0x80) ? 65535 : 0;
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
            ch = video.vdp_ram[i][j];
            if (ch >= ' ' && ch <= 'Z')
                draw_char(i, j, ch - ' ');
        }

    SDL_UnlockTexture(video.texture);

    SDL_RenderCopy(video.renderer, video.texture, NULL, NULL);
    SDL_RenderPresent(video.renderer);

}

static void load_char_set()
{
    struct section_struct *section;
    struct key_struct *key;
    struct key_struct *start;
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
            memcpy(charset[char_code - ' '], hex, 8);
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
        SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_STREAMING, 320, 192);

    if (video.texture == NULL) { 
        LOG_ERR("Error creating SDL texture: %s", SDL_GetError());
        return 0;
    }

    refresh_video();

    return video.vdp_ram;
}

void finalize_video()
{
    LOG_INF("Finalizing video.\n");

    SDL_DestroyWindow(video.window);
    SDL_Quit();
}
