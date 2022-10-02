#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#define SHEEP_DYNARRAY_IMPLEMENTATION
#include "dynarray.h"

#define SDL_DISABLE_IMMINTRIN_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#define SUI_IMPLEMENTATION
#define SUI_SDL_RENDERER_IMPLEMENTATION
#include "sui.h"
#include "sui_sdl_renderer.h"

#define unused(x) (void)x

static uint64_t tick = 0;
static int *array = dynarray_new;
static int max_elem = 0;
static int current_elem = 0;
static int sort_delay_ms = 10;
static int n = 200;
static pthread_t *running_sorts = dynarray_new;

void rnd_array(int lim) {
    for (int i = 0; i < dynarray_len(array); i++) {
        array[i] = rand() % lim + 1;
        if (max_elem < array[i])
            max_elem = array[i];
    }
}

void swap(int l, int r) {
    /* XOR based swapping */
    int temp = array[l];
    array[l] = array[r];
    array[r] = temp;
}

#define dotick do { \
                tick++; \
                SDL_Delay(sort_delay_ms); \
            } while (0)

void *bubble_sort(void *args) {
    unused(args);
    for (int i = 0; i < dynarray_len(array); i++) {
        for (int j = 0; j < dynarray_len(array) - 1; j++) {
            if (array[j] > array[j+1]) {
                swap(j, j + 1);
            }
            current_elem = j;
            dotick;
        }
    }
    return NULL;
}

void *selection_sort(void *args) {
    unused(args);
    for (int i = 0; i < dynarray_len(array) - 1; i++) {
        int cur_min = i;
        for (int j = i + 1; j < dynarray_len(array); j++) {
            current_elem = j;
            if (array[j] < array[cur_min])
                cur_min = j;
            dotick;
        }
        if (i != cur_min) swap(i, cur_min);
    }
    return NULL;
}

void merge_sort_merge(int l, int m, int r) {
    int ln = m - l + 1;
    int rn = r - m;
    int *la = malloc(sizeof *la * ln);
    int *ra = malloc(sizeof *ra * rn);
    int li = 0, ri = 0;
    memcpy(la, array + l, sizeof *la * ln);
    memcpy(ra, array + m + 1, sizeof *ra * rn);
    int *put = array + l;
    while (li < ln && ri < rn) {
        if (la[li] <= ra[ri])
            *put++ = la[li++];
        else
            *put++ = ra[ri++];
        dotick;
    }
    while (li < ln) {
        *put++ = la[li++];
        dotick;
    }
    while (ri < rn) {
        *put++ = ra[ri++];
        dotick;
    }
    free(la);
    free(ra);
}

void merge_sort_help(int l, int r) {
    if (r - l < 1) return;
    int m = l + (r - l) / 2;
    merge_sort_help(l, m);
    merge_sort_help(m + 1, r);
    merge_sort_merge(l, m, r);
}

void *merge_sort(void *args) {
    unused(args);
    merge_sort_help(0, dynarray_len(array) - 1);
    return NULL;
}


/// TODOOOOOOOOOO: more sorting algos

struct {
    const char *name;
    void* (*callback)(void*);
} sorts[] = {{" Bubble ", bubble_sort}, {"Selection", selection_sort}, {"  Merge  ", merge_sort}};

void *gui(void *args) {
    unused(args);
    const int width = 1200;
    const int height = 800;
    const int frame_width = width;
    const int frame_height = 300;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    TTF_Init();
    SDL_Window *win;
    SDL_Renderer *rend;
    TTF_Font *font = TTF_OpenFont("tahoma.ttf", 20);
    SDL_CreateWindowAndRenderer(width, height, 0, &win, &rend);
    sui_ctx ctx;
    sui_ctx_init(&ctx);
    sui_sdl_init_ctx(&ctx);
    sui_sdl_init(rend, font);
    bool running = true;
    int w1 = frame_width / dynarray_len(array);
    int h1 = frame_height / max_elem;
    char tick_buf[64] = { 0 };
    void *(*sel_function)(void*) = NULL;

    while (running) {
        /* update */

        /* event */
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            sui_sdl_handle_event(&ctx, e);
            if (e.type == SDL_QUIT) {
                running = false;
            }
        }

        /* draw */
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);
        for (int i = 0; i < dynarray_len(array); i++) {
            if (current_elem == i)
                SDL_SetRenderDrawColor(rend, 0, 255, 0, 255);
            else
                SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
            SDL_Rect rect = {i * w1, frame_height - array[i] * h1, w1,
                             array[i] * h1};
            SDL_RenderFillRect(rend, &rect);
        }
        snprintf(tick_buf, sizeof tick_buf - 1, "%" PRIu64, tick);
        sui_text(&ctx, tick_buf, 0, 0);
        if (sui_btn(&ctx, "Random", 0, frame_height, 0)) {
            rnd_array(dynarray_len(array));
            w1 = frame_width / dynarray_len(array);
            h1 = frame_height / max_elem;
        }
        int x = 120, y = frame_height;
        for (size_t i = 0; i < sizeof sorts / sizeof *sorts; i++) {
            if (sui_btn(&ctx, sorts[i].name, x, y, i)) {
                sel_function = sorts[i].callback;
            }
            x += 140;
            if (x > width) {
                x = 0;
                y += 80;
            }
        }

        if (sel_function != NULL) {
            /* note - not a nicest way to exit thread, but make code nicer */
            for (long i = 0; i < dynarray_len(running_sorts); i++)
                pthread_cancel(running_sorts[i]);
            pthread_t t;
            tick = 0;
            pthread_create(&t, NULL, sel_function, NULL);
            dynarray_push(running_sorts, t);
            sel_function = NULL;
        }

        SDL_RenderPresent(rend);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();
    TTF_CloseFont(font);
    TTF_Quit();
    return NULL;
}

int main(int argc, char **argv) {
    if (argc > 1 && argv[1] != NULL) {
        n = strtol(argv[1], NULL, 10);
    }
    srand(time(NULL));
    dynarray_setlen(array, n);
    rnd_array(n);

    pthread_t thread;
    pthread_create(&thread, NULL, gui, NULL);

    pthread_join(thread, NULL);

    return 0;
}
