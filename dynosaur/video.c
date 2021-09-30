#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "types.h"
#include "logging.h"
#include "Stuff.h"
#include "config.h"
#include "dynosaur.h"
#include "bus.h"

struct {
    SDL_Window      *window;
    SDL_Renderer    *renderer;
    SDL_Texture     *texture;
    Uint16          *pixels;
    Uint16          location;
    struct {
        BYTE        screen[24][40];  // 0000
        BYTE        color_table[32]; // 03c0
        BYTE        free_space[32];  // 03e0
        BYTE        pattern_table[256][8]; // 0400
        BYTE        sprite_table[16][8]; // 0C00
        BYTE        padding[128];
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

void render_sprite(BYTE sprite_no)
{
    BYTE *sprite_table = video.vdp_ram.sprite_table[sprite_no];

    // Sprite pattern can't start in zero page.
    if (sprite_table[1] == 0)
        return;
     
    WORD address = word(sprite_table[0], sprite_table[1]);
    BYTE sp_row = sprite_table[2];
    WORD sp_col = word(sprite_table[4], sprite_table[5]);
    BYTE width = sprite_table[6];
    BYTE height = sprite_table[7];
    BYTE color_byte;
    BYTE color;

    for (BYTE i = 0; i < height; i++) {
        WORD row = 320 * (BYTE) (sp_row + i);
        if ((BYTE)(sp_row + i) >= 192) {
            address += (width + 1) / 2;
            continue;
        }
        for (BYTE j = 0; j < width; j+= 2) {
            WORD col = (sp_col + j);
            color_byte = ram_accessor(address, true, 0);
            color = color_byte >> 4;
            if (color > 0 && (col < 320))
                video.pixels[row + col] = video.color[color];
            color = color_byte & 0x0f;
            col++;
            if (color > 0 && col < 320 && ((j + 1) < width))
                video.pixels[row + col] = video.color[color];
            address++;
        }
    }
}

void render_sprites()
{
    for (int i = 0; i < 16; i++)
        render_sprite(i);
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


    render_sprites();

    SDL_UnlockTexture(video.texture);

    SDL_RenderCopy(video.renderer, video.texture, NULL, NULL);
    SDL_RenderPresent(video.renderer);

}

BYTE vdp_ram_accessor(WORD address, bool read, BYTE value)
{
    if (read)
        value = *((BYTE *) &video.vdp_ram + (address - video.location));
    else
        *((BYTE *) &video.vdp_ram + (address - video.location)) = value;

    return value;
}

static void load_tables()
{
    struct section_struct *section;
    struct key_struct *key;
    struct list_head *list;
    struct list_head *first;
    BYTE   code;
    BYTE hex[8];
    SDL_PixelFormat *format;
    
    format = SDL_AllocFormat(SDL_PIXELFORMAT_RGB444);
   
    section = get_config_section("VIDEO");
    if (section == NULL) {
        LOG_ERR("Could not find VIDEO configuration section");
        return;
    }

    key = section->key;
    LIST_FOR_EVERY(list, &section->key->list, first) {
        key = GET_ELEMENT(struct key_struct, list, list);
        if (1 == sscanf(key->name, "C%02hhX", &code)) {
            LOG_DBG("%s=%s\n", key->name, key->value);
            sscanf(key->value, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", 
                &hex[0], &hex[1], &hex[2], &hex[3],
                &hex[4], &hex[5], &hex[6], &hex[7]);
            memcpy(video.vdp_ram.pattern_table[code], hex, 8);
        } else if (1 == sscanf(key->name, "COLOR%1hhX", &code)) {
            LOG_DBG("%s=%s\n",  key->name, key->value);
            sscanf(key->value, "%02hhX%02hhX%02hhX", &hex[0], &hex[1], &hex[2]);
            video.color[code]  = SDL_MapRGB(format, hex[0], hex[1], hex[2]);
        }
    }
    SDL_FreeFormat(format);
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

    video.location = get_config_hex("VIDEO", "LOCATION");
    if (0 == video.location) {
        LOG_WRN("Video RAM location not found. Setting to default.\n");
        video.location = 0x8000;
    }
    LOG_INF("Video RAM located at 0x%04x.\n", video.location);
    load_tables();
}

void log_video_section(char *description, void *location)
{
    LOG_INF("%s at %04x.\n", description, (video.location + (location - (void *)&video.vdp_ram)));
}

void init_video(bool full_init)
{
    LOG_INF("Initilizing video.\n");
    load_config();
    
    if (full_init) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            LOG_ERR("Error initializing SDL: %s", SDL_GetError());
            return;
         }

        video.window = SDL_CreateWindow(video.title, SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, video.width, video.height, SDL_WINDOW_SHOWN);
        if (video.window == NULL) {
            LOG_ERR("Error creating SDL window: %s", SDL_GetError());
            return;
        }
    }

    SDL_RaiseWindow(video.window);
    video.renderer = SDL_CreateRenderer(video.window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (video.window == NULL) {
        LOG_ERR("Error creating SDL renderer: %s", SDL_GetError());
        return;
    }

    SDL_SetRenderDrawColor(video.renderer, 0, 0, 0, 0);
    SDL_RenderClear(video.renderer);
    SDL_RenderPresent(video.renderer);

    video.texture = SDL_CreateTexture(video.renderer, 
        SDL_PIXELFORMAT_RGB444, SDL_TEXTUREACCESS_STREAMING, 320, 192);

    if (video.texture == NULL) { 
        LOG_ERR("Error creating SDL texture: %s", SDL_GetError());
        return;
    }

    memset(video.vdp_ram.color_table, 0x17, 32);
    attach_bus(vdp_ram_accessor, hi(video.location), sizeof(video.vdp_ram) / 256);
    
    LOG_INF("Video RAM size %d pages.\n", sizeof(video.vdp_ram) / 256);
    log_video_section("Screen", &video.vdp_ram.screen);
    log_video_section("Color table", &video.vdp_ram.color_table);
    log_video_section("Pattern table", &video.vdp_ram.pattern_table);
    log_video_section("Sprite table", &video.vdp_ram.sprite_table);

    refresh_video();
}

void finalize_video(bool full_finalize)
{
    LOG_INF("Finalizing video.\n");

    SDL_DestroyTexture(video.texture);
    SDL_DestroyRenderer(video.renderer);

    if (full_finalize) {
        SDL_DestroyWindow(video.window);
        SDL_Quit();
    }
}
