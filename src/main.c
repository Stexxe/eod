#define SDL_MAIN_USE_CALLBACKS
#include <assert.h>
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
static TTF_Font *num_font = NULL;
// static int dpr = 1;

static SDL_Texture *game_title_tex = NULL;
static SDL_Color primary_color = { 255, 255, 255, 255 };
static SDL_Color del_color = { 34, 51, 55, 255 };

static float cell_size;

static const int field_width = 8;
static const int field_height = 8;
static SDL_FRect *field_rects;
static size_t field_rects_count;

static char *numbers[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

struct cell { int row; int col; };

struct gameState {
    int screen_width, screen_height;
    SDL_Rect game_viewport;
    SDL_Rect field_viewport;
    float dpr;
    SDL_Texture **num_textures;
    struct cell clicked_cell;
    // int *field;
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

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    if (pool_init(32 * 1024 * 1024) < 0) {
        SDL_Log("Cannot allocate memory");
        return SDL_APP_FAILURE;
    }

    struct gameState *state = pool_alloc_struct(struct gameState);
    *appstate = state;

    int d;
    env_settings(&state->screen_width, &state->screen_height, &d);
    state->dpr = (float) d;

    state->clicked_cell.col = -1;
    state->clicked_cell.row = -1;

    if (!SDL_CreateWindowAndRenderer("Echoes of the Deep", state->screen_width * state->dpr, state->screen_height * state->dpr, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!TTF_Init()) {
        SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    title_font = TTF_OpenFont("ShareTechMono-Regular.ttf", 20 * state->dpr);
    game_title_tex = create_text_texture(title_font, "ECHOES OF THE DEEP", primary_color);

    num_font = TTF_OpenFont("ShareTechMono-Regular.ttf", 32 * state->dpr);
    TTF_SetFontStyle(num_font, TTF_STYLE_BOLD);

    state->num_textures = pool_alloc(sizeof(SDL_Texture *) * 10, SDL_Texture *);
    for (int i = 0; i < 10; i++) {
        state->num_textures[i] = create_text_texture(num_font, numbers[i], primary_color);
    }

    SDL_SetRenderScale(renderer, state->dpr, state->dpr);

    int pad = 32;
    state->game_viewport = (SDL_Rect) { pad, pad,  state->screen_width- pad*2, state->screen_height - pad*2 };

    float title_width, title_height;
    SDL_GetTextureSize(game_title_tex, &title_width, &title_height);

    cell_size = (float) (state->game_viewport.w / field_width);

    state->field_viewport = (SDL_Rect) { state->game_viewport.x, state->game_viewport.y + (int) (title_height / state->dpr) + 20,  cell_size * field_width, cell_size * field_height };

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
    struct gameState *state = (struct gameState *) appstate;

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_FINGER_DOWN) {
        float mx = event->button.x / state->dpr;
        float my = event->button.y / state->dpr;

        if (mx >= state->field_viewport.x && mx < state->field_viewport.x + state->game_viewport.w
            && my >= state->field_viewport.y && my < state->field_viewport.y + state->field_viewport.h) {

            int row = (int) (my - state->field_viewport.y) / (int) cell_size;
            if (row >= field_height) row = field_height - 1;

            int col = (int) (mx - state->field_viewport.x) / (int) cell_size;
            if (col >= field_width) col = field_width - 1;

            state->clicked_cell.row = row;
            state->clicked_cell.col = col;

            printf("%d %d\n", row, col);
        }

        printf("%f %f\n", mx, my);
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
    dst.w /= state->dpr;
    dst.h /= state->dpr;
    SDL_RenderTexture(renderer, game_title_tex, NULL, &dst);

    SDL_SetRenderViewport(renderer, &state->field_viewport);

    SDL_SetRenderDrawColor(renderer, del_color.r, del_color.g, del_color.b, del_color.a);
    SDL_RenderRects(renderer, field_rects, (int) field_rects_count);

    if (state->clicked_cell.row >= 0 && state->clicked_cell.col >= 0) {
        int n = 7;
        SDL_Texture *tex = state->num_textures[n];

        float texw, texh;
        SDL_GetTextureSize(tex, &texw, &texh);

        texw /= state->dpr;
        texh /= state->dpr;

        float cellx = cell_size * state->clicked_cell.col;
        float celly = cell_size * state->clicked_cell.row;
        SDL_FRect dest = { cellx + cell_size/2 - texw/2, celly + cell_size/2 - texh/2, texw, texh };

        SDL_RenderTexture(renderer, tex, NULL, &dest);
    }

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {

}