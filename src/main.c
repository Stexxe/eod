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

static SDL_Texture *game_title_tex = NULL;
static SDL_Color primary_color = { 0, 255, 255, 255 };
static SDL_Color del_color = { 34, 51, 55, 255 };

static float cell_size;

#define FIELD_MAX_ROWS 8
#define FIELD_MAX_COLS 8

// static const int field_width = 8;
// static const int field_height = 8;
// static SDL_FRect *field_rects;
// static size_t field_rects_count;

static char *numbers[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

struct cell { int row; int col; };

struct animation {
    Uint64 start_time;
    Uint64 total_time;
    Uint64 elapsed_time;
};

enum treasureType {
    TREASURE_NONE,
    TREASURE_REGULAR
};

struct gameState {
    int screen_width, screen_height;
    SDL_Rect game_viewport;
    SDL_Rect field_viewport;
    float dpr;
    SDL_Texture **num_textures;
    struct cell clicked_cell;
    SDL_Texture *active_num_tex;

    SDL_Texture *treasure_tex;

    SDL_FRect field_rects[FIELD_MAX_ROWS + FIELD_MAX_COLS];

    struct animation **anims;
    int anims_count;
    enum treasureType field[FIELD_MAX_ROWS][FIELD_MAX_COLS];
};

static bool within_field(enum treasureType field[][FIELD_MAX_COLS], int row, int col) {
    return row >= 0 && col >= 0 && row < FIELD_MAX_ROWS && col < FIELD_MAX_COLS;
}

static struct cell coefs[] ={
    {-1, -1},
    {-1, 0},
    {-1, +1},
    {0, -1},
    {0, +1},
    {+1, -1},
    {+1, 0},
    {+1, +1},
};

#define COEF_COUNT (sizeof(coefs) / sizeof(coefs[0]))

static int calc_distance(enum treasureType field[][FIELD_MAX_COLS], int row, int col) {
    assert(row >= 0 && col >= 0);
    assert(row < FIELD_MAX_ROWS && col < FIELD_MAX_COLS);

    if (field[row][col] == TREASURE_REGULAR) return 0;

    int dist = 1;


    bool has_points;
    do {
        has_points = false;

        for (int i = 0; i < COEF_COUNT; i++) {
            int r = row + coefs[i].row * dist;
            int c = col + coefs[i].col * dist;
            if (within_field(field, r, c)) {
                has_points = true;
                if (field[r][c] > 0) goto found;
            }
        }

        dist++;
    } while (has_points);

    found:

    return dist;
}

static Uint32 fade_num(void *userdata, SDL_TimerID timer_id, Uint32 interval) {
    struct gameState *state = userdata;

    struct animation *anim = pool_alloc_struct(struct animation);
    // anim->start_time = SDL_GetTicks();
    // anim->total_time = 2000;
    //
    // state->anims[state->anims_count++] = anim;

    return 0;
}

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

#define MAX_ANIMS 32

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

    state->anims_count = 0;
    state->anims = pool_alloc(MAX_ANIMS * sizeof(struct animation *), struct animation *);

    state->clicked_cell.col = -1;
    state->clicked_cell.row = -1;

    for (int r = 0; r < FIELD_MAX_ROWS; r++) {
        for (int c = 0; c < FIELD_MAX_COLS; c++) {
            state->field[r][c] = TREASURE_NONE;
        }
    }

    state->field[4][4] = TREASURE_REGULAR;

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

    cell_size = (float) (state->game_viewport.w / FIELD_MAX_COLS);

    state->field_viewport = (SDL_Rect) {
        state->game_viewport.x,
        state->game_viewport.y + (int) (title_height / state->dpr) + 20,
        cell_size * FIELD_MAX_COLS,
        cell_size * FIELD_MAX_ROWS
    };

    SDL_FRect *fp = state->field_rects;
    float width = (float) FIELD_MAX_COLS * cell_size;
    float height = (float) FIELD_MAX_ROWS * cell_size;

    for (int c = 0; c < FIELD_MAX_COLS; c++) {
        *fp++ = (SDL_FRect) { (float) c * cell_size, 0, cell_size, height };
    }

    for (int r = 0; r < FIELD_MAX_ROWS; r++) {
        *fp++ = (SDL_FRect) { 0, (float) r * cell_size, width, cell_size };
    }

    state->treasure_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 24, 24);
    SDL_SetRenderTarget(renderer, state->treasure_tex);
    SDL_SetRenderDrawColor(renderer, 178, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, NULL);

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
            if (row >= FIELD_MAX_ROWS) row = FIELD_MAX_ROWS - 1;

            int col = (int) (mx - state->field_viewport.x) / (int) cell_size;
            if (col >= FIELD_MAX_COLS) col = FIELD_MAX_COLS - 1;

            state->clicked_cell.row = row;
            state->clicked_cell.col = col;
            state->active_num_tex = state->num_textures[calc_distance(state->field, row, col)];

            SDL_AddTimer(1000, fade_num, state);
        }

        printf("%f %f\n", mx, my);
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    struct gameState *state = (struct gameState *) appstate;

    for (int i = 0; i < state->anims_count; i++) {
        // state->anims[i]->elapsed_time += SDL_GetTicks() - state->anims[i]->start_time;
    }

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
    SDL_RenderRects(renderer, state->field_rects, FIELD_MAX_ROWS + FIELD_MAX_COLS);

    if (state->clicked_cell.row >= 0 && state->clicked_cell.col >= 0) {
        float texw, texh;
        SDL_GetTextureSize(state->active_num_tex, &texw, &texh);

        texw /= state->dpr;
        texh /= state->dpr;

        float cellx = cell_size * state->clicked_cell.col;
        float celly = cell_size * state->clicked_cell.row;
        SDL_FRect dest = { cellx + cell_size/2 - texw/2, celly + cell_size/2 - texh/2, texw, texh };

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        // SDL_SetTextureAlphaMod(tex, 100);
        SDL_RenderTexture(renderer, state->active_num_tex, NULL, &dest);
    }

    float texw, texh;
    SDL_GetTextureSize(state->treasure_tex, &texw, &texh);
    for (int r = 0; r < FIELD_MAX_ROWS; r++) {
        for (int c = 0; c < FIELD_MAX_COLS; c++) {
            if (state->field[r][c] == TREASURE_REGULAR) {
                float cellx = cell_size * c;
                float celly = cell_size * r;
                SDL_FRect dest = { cellx + cell_size/2 - texw/2, celly + cell_size/2 - texh/2, texw, texh };
                SDL_RenderTexture(
                    renderer,
                    state->treasure_tex,
                    NULL,
                    &dest
                );
            }
        }
    }

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {

}