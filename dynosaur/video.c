#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "types.h"
#include "logging.h"

struct {
    SDL_Window      *window;
    SDL_Renderer    *renderer;
    SDL_Texture     *texture;
    Uint16          *pixels;
} video;

BYTE charset[256][8] = {
    { 0x00, 0x3c, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42 }, // A
    { 0x00, 0x7c, 0x42, 0x42, 0x7c, 0x42, 0x42, 0x7c }, // B
    { 0x00, 0x3c, 0x42, 0x40, 0x40, 0x40, 0x42, 0x3c }, // C
    { 0x00, 0x7c, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7c }, // D
    { 0x00, 0x7e, 0x40, 0x40, 0x7c, 0x40, 0x40, 0x7e }, // E
    { 0x00, 0x7e, 0x40, 0x40, 0x7c, 0x40, 0x40, 0x40 }, // F
    { 0x00, 0x3c, 0x42, 0x40, 0x40, 0x46, 0x42, 0x3c }, // G
    { 0x00, 0x42, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42 }, // H

    { 0x00, 0x3e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x3e }, // I 
    { 0x00, 0x38, 0x08, 0x08, 0x08, 0x08, 0x48, 0x30 }, // J

};

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

void init_video()
{
    LOG_INF("Initilizing video.\n");
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERR("Error initializing SDL: %s", SDL_GetError());
        return;
    }

    video.window = SDL_CreateWindow("Dynamic", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 320 * 3, 192 * 3, SDL_WINDOW_SHOWN);
    if (video.window == NULL) {
        LOG_ERR("Error creating SDL window: %s", SDL_GetError());
        return;
    }

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
        SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_STREAMING, 320, 192);

    if (video.texture == NULL) { 
        LOG_ERR("Error creating SDL texture: %s", SDL_GetError());
        return;
    }
        
    int pitch;
    if (SDL_LockTexture(video.texture, NULL, (void *) &video.pixels, &pitch))
        LOG_ERR("Error locking SDL texture: %s", SDL_GetError());

    int i, j;
    for (i = 0; i < 5; i++)
        for (j = 0; j < 10; j++)
            draw_char(i + 3, i + j + 3, j);

    SDL_UnlockTexture(video.texture);

    SDL_RenderCopy(video.renderer, video.texture, NULL, NULL);
    SDL_RenderPresent(video.renderer);
}

void finalize_video()
{
    LOG_INF("Finalizing video.\n");

    SDL_DestroyWindow(video.window);
    SDL_Quit();
}
