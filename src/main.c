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
static TTF_Font *title_font = NULL;
static int dpr = 1;

static SDL_Texture *game_title_tex = NULL;
static SDL_Color text_color = { 255, 255, 255, 255 };

EM_JS(void, env_settings, (int *width, int *height, int *dpr), {
    HEAP32[dpr / 4] = window.devicePixelRatio || 1;
    HEAP32[width / 4] = window.innerWidth;
    HEAP32[height / 4] = window.innerHeight;
});

static SDL_Texture *create_text_texture(TTF_Font *f, const char *text, SDL_Color color) {
    SDL_Surface *surf = TTF_RenderText_Blended(f, text, 0, color);
    SDL_Texture *texture = NULL;
    if (surf) {
        texture = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);
    }

    return texture;
}

static TTF_Font *load_font(const char *path, float ptsize) {
    return TTF_OpenFont("ShareTechMono-Regular.ttf", ptsize * (float) dpr);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    int window_width, window_height;
    env_settings(&window_width, &window_height, &dpr);

    if (!SDL_CreateWindowAndRenderer("Echoes of the Deep", window_width * dpr, window_height * dpr, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!TTF_Init()) {
        SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    title_font = load_font("ShareTechMono-Regular.ttf", 20.0f);
    game_title_tex = create_text_texture(title_font, "ECHOES OF THE DEEP", text_color);

    SDL_SetRenderScale(renderer, (float) dpr, (float) dpr);

    SDL_Rect rect = { 20, 20, window_width - 20, window_height - 20 };
    SDL_SetRenderViewport(renderer, &rect);

    if (pool_init(32 * 1024 * 1024) < 0) {
        SDL_Log("Cannot allocate memory");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_FINGER_DOWN) {
        printf("MOUSE_BUTTON_DOWN\n");
    }
    // if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_QUIT) {
    //     return SDL_APP_SUCCESS;
    // }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    SDL_FRect dst;
    SDL_GetTextureSize(game_title_tex, &dst.w, &dst.h);
    dst.x = 0;
    dst.y = 0;
    dst.w /= ((float) dpr);
    dst.h /= (float) dpr;

    SDL_SetRenderDrawColor(renderer, 21, 24, 27, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, game_title_tex, NULL, &dst);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {

}