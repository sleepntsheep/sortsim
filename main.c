#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static int *array = dynarray_new;
static int max_elem = 0;
static size_t current_elem = 0;
static int sort_delay_ms = 10;
static int n = 200;
static pthread_t *running_sorts = dynarray_new;

typedef struct {
    int compare, access;
} sort_stat;

static sort_stat stat = {0};

void delay() { SDL_Delay(sort_delay_ms); }

int *array_at(size_t i) {
    stat.access++;
    current_elem = i;
    delay();
    return array + i;
}

void *rnd_array(void *args) {
    unused(args);
    for (size_t i = 0; i < dynarray_len(array); i++) {
        array[i] = rand() % n + 1;
        if (max_elem < array[i])
            max_elem = array[i];
    }
    return NULL;
}

void swap(int i, int j) {
    int temp = *array_at(i);
    *array_at(i) = *array_at(j);
    *array_at(j) = temp;
}

void *bubble_sort(void *args) {
    unused(args);
    for (size_t i = 0; i < dynarray_len(array); i++) {
        for (size_t j = 0; j < dynarray_len(array) - 1; j++) {
            if (*array_at(j) > *array_at(j + 1)) {
                swap(j, j + 1);
            }
        }
    }
    return NULL;
}

void *selection_sort(void *args) {
    unused(args);
    for (size_t i = 0; i < dynarray_len(array) - 1; i++) {
        size_t cur_min = i;
        for (size_t j = i + 1; j < dynarray_len(array); j++) {
            if (*array_at(j) < *array_at(cur_min))
                cur_min = j;
        }
        if (i != cur_min)
            swap(i, cur_min);
    }
    return NULL;
}

void merge_sort_merge(int l, int m, int r) {
    size_t ln = m - l + 1;
    size_t rn = r - m;
    int *la = malloc(sizeof *la * ln);
    int *ra = malloc(sizeof *ra * rn);
    size_t li = 0, ri = 0;
    for (size_t i = 0; i < ln; i++)
        la[i] = *array_at(l + i);
    for (size_t i = 0; i < rn; i++)
        ra[i] = *array_at(m + 1 + i);
    size_t put = l;
    while (li < ln && ri < rn)
        if (la[li] <= ra[ri])
            *array_at(put++) = la[li++];
        else
            *array_at(put++) = ra[ri++];
    while (li < ln)
        *array_at(put++) = la[li++];
    while (ri < rn)
        *array_at(put++) = ra[ri++];
    free(la);
    free(ra);
}

void merge_sort_help(int l, int r) {
    if (r - l < 1)
        return;
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

void *stop_sort(void *args) {
    unused(args);
    return NULL;
}

/// TODOOOOOOOOOO: more sorting algos

struct {
    const char *name;
    void *(*callback)(void *);
} sorts[] = {{"Random", rnd_array},
             {"Stop", stop_sort},
             {" Bubble ", bubble_sort},
             {"Selection", selection_sort},
             {"  Merge  ", merge_sort}};

void *gui(void *args) {
    unused(args);
    const int width = 1200;
    const int height = 800;
    const int frame_width = width;
    const int frame_height = 500;
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
    float w1, h1;
    char buf[1024] = {0};
    void *(*sel_function)(void *) = NULL;

    while (running) {
        /* update */
        {
            w1 = (float)frame_width / dynarray_len(array);
            h1 = (float)frame_height / max_elem;

            if (sel_function != NULL) {
                /* note - not a nicest way to exit thread, but make code nicer
                 */
                for (size_t i = 0; i < dynarray_len(running_sorts); i++)
                    pthread_cancel(running_sorts[i]);
                stat = (sort_stat){0};
                pthread_t t;
                pthread_create(&t, NULL, sel_function, NULL);
                dynarray_push(running_sorts, t);
                sel_function = NULL;
            }
        }

        /* event */
        {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                sui_sdl_handle_event(&ctx, e);
                if (e.type == SDL_QUIT) {
                    running = false;
                }
            }
        }

        /* draw */
        {

            SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
            SDL_RenderClear(rend);
            for (size_t i = 0; i < dynarray_len(array); i++) {
                if (current_elem == i)
                    SDL_SetRenderDrawColor(rend, 0, 255, 0, 255);
                else
                    SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
                SDL_Rect rect = {i * w1, frame_height - array[i] * h1, w1,
                                 array[i] * h1};
                SDL_RenderFillRect(rend, &rect);
            }
        }

        /* draw ui */
        {
            snprintf(buf, sizeof buf - 1, "%d", stat.access);
            sui_text(&ctx, buf, 0, 0);
            float temp_sort_delay_ms = sort_delay_ms;
            sui_slider_float(&ctx, 5, &temp_sort_delay_ms, 40, 120,
                             frame_height + 5, 200, 20);
            sort_delay_ms = temp_sort_delay_ms;
            int x = 120, y = frame_height + 50;
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
    rnd_array(NULL);

    pthread_t thread;
    pthread_create(&thread, NULL, gui, NULL);

    pthread_join(thread, NULL);

    return 0;
}
