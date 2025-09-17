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
static SDL_Color primary_color = { 255, 255, 255, 255 };
static SDL_Color del_color = { 34, 51, 55, 255 };

static float cell_size;

static const int field_width = 10;
static const int field_height = 10;
static SDL_FRect *field_rects;
static size_t field_rects_count;

struct gameState {
    int screen_width, screen_height;
    SDL_Rect game_viewport;
    SDL_Rect field_viewport;
};

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

    if (pool_init(32 * 1024 * 1024) < 0) {
        SDL_Log("Cannot allocate memory");
        return SDL_APP_FAILURE;
    }

    struct gameState *state = pool_alloc_struct(struct gameState);
    *appstate = state;

    env_settings(&state->screen_width, &state->screen_height, &dpr);

    if (!SDL_CreateWindowAndRenderer("Echoes of the Deep", state->screen_width * dpr, state->screen_height * dpr, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!TTF_Init()) {
        SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    title_font = load_font("ShareTechMono-Regular.ttf", 20.0f);
    game_title_tex = create_text_texture(title_font, "ECHOES OF THE DEEP", primary_color);

    SDL_SetRenderScale(renderer, (float) dpr, (float) dpr);
    int pad = 32;
    state->game_viewport = (SDL_Rect) { pad, pad,  state->screen_width- pad*2, state->screen_height - pad*2 };
    state->field_viewport = (SDL_Rect) { state->game_viewport.x, state->game_viewport.y + 44,  state->game_viewport.w, state->game_viewport.h };

    cell_size = (float) (state->field_viewport.w / field_width);

    SDL_FRect *fp = field_rects = pool_alloc(2 * field_width + 2 * field_height, SDL_FRect);
    float width = (float) field_width * cell_size;
    float height = (float) field_height * cell_size;

    for (int c = 0; c < field_width; c++) {
        *fp++ = (SDL_FRect) { (float) c * cell_size, 0, cell_size, height };
    }

    for (int r = 0; r < field_height; r++) {
        *fp++ = (SDL_FRect) { 0, (float) r * cell_size, width, cell_size };
    }

    field_rects_count = fp - field_rects;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_FINGER_DOWN) {
        printf("MOUSE_BUTTON_DOWN\n");
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    struct gameState *state = (struct gameState *) appstate;

    SDL_SetRenderDrawColor(renderer, 21, 24, 27, 255); // background
    SDL_RenderClear(renderer);

    SDL_SetRenderViewport(renderer, &state->game_viewport);

    SDL_FRect dst;
    SDL_GetTextureSize(game_title_tex, &dst.w, &dst.h);
    dst.x = 0;
    dst.y = 0;
    dst.w /= ((float) dpr);
    dst.h /= (float) dpr;
    SDL_RenderTexture(renderer, game_title_tex, NULL, &dst);

    SDL_SetRenderViewport(renderer, &state->field_viewport);

    SDL_SetRenderDrawColor(renderer, del_color.r, del_color.g, del_color.b, del_color.a);
    SDL_RenderRects(renderer, field_rects, (int) field_rects_count);

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {

}