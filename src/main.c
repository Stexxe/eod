#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_main.h>
#include <emscripten/html5.h>
#include <stdio.h>
#include "mem.h"
#include "SDL3_ttf/SDL_ttf.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static TTF_Font *font = NULL;
static SDL_Texture *texture = NULL;
static int dpr = 1;

EM_JS(void, env_settings, (int *width, int *height, int *dpr), {
    HEAP32[dpr / 4] = window.devicePixelRatio || 1;
    HEAP32[width / 4] = window.innerWidth;
    HEAP32[height / 4] = window.innerHeight;
});

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    int window_width, window_height;
    env_settings(&window_width, &window_height, &dpr);

    if (!SDL_CreateWindowAndRenderer("Echoes of the Deep", window_width * dpr, window_height * dpr, SDL_WINDOW_FULLSCREEN, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!TTF_Init()) {
        SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    font = TTF_OpenFont("ShareTechMono-Regular.ttf", 32.0f * (float) dpr);

    if (!font) {
        SDL_Log("Couldn't open font: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Color color = { 255, 255, 255, SDL_ALPHA_OPAQUE };
    SDL_Surface *text = TTF_RenderText_Blended(font, "0123456789", 0, color);
    if (text) {
        texture = SDL_CreateTextureFromSurface(renderer, text);
        SDL_DestroySurface(text);
    }

    if (!texture) {
        SDL_Log("Couldn't create text: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderScale(renderer, (float) dpr, (float) dpr);

    if (pool_init(32 * 1024 * 1024) < 0) {
        SDL_Log("Cannot allocate memory");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    // if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_QUIT) {
    //     return SDL_APP_SUCCESS;
    // }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    int w = 0, h = 0;
    SDL_FRect dst;
    SDL_GetTextureSize(texture, &dst.w, &dst.h);
    SDL_GetRenderOutputSize(renderer, &w, &h);
    const float scale = 4.0f;
    dst.x = 100;
    dst.y = 100;
    dst.w /= ((float) dpr - 0.2f);
    dst.h /= (float) dpr;

    SDL_SetRenderDrawColor(renderer, 21, 24, 27, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, &dst);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {

}