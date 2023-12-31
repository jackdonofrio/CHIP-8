#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "emu.h"
#include "sdl_utils.h"




int main(const int argc, char** argv)

{
    const int expected_arg_count = 2;
    if (argc != expected_arg_count) {
        fprintf(stderr, "usage: %s <rom file>\n", argv[0]);
        exit(1);
    }
    emu_state_t* state = state_new();
    if (state == NULL) {
        exit(1);
    }
    srand(time(NULL)); // seed rng for RND instruction
    state_init(state);
    #ifdef DEBUG
        setup_ncurses();
    #endif

    #ifdef DEBUG
        // ensure memcpy properly loaded fontset into mem
        // debug_mem(state, FONTSET_OFFSET, FONTSET_OFFSET + FONTSET_SIZE);
        int mem_scroll = ROM_START;
        int c;
    #endif


    #ifdef SDLMODE
        SDL_Window* window = sdl_create_window(argv[1]);
        if (window == NULL) {
            fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
            return 1;
        }
        // Create a renderer
        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
            return -1;
        }
        SDL_Event e;

    #endif

    file_to_mem(state, argv[1], ROM_START);

    struct timeval current_time, last_cycle_time;
    gettimeofday(&last_cycle_time, NULL);

    bool done = false;
    while (!done) {
        gettimeofday(&current_time, NULL);

       if (((double)current_time.tv_sec - (double)last_cycle_time.tv_sec) >= CYCLE_DELAY) {
            gettimeofday(&last_cycle_time, NULL);
            done = state_cycle(state);

            #ifdef SDLMODE

                // Handle events on the queue
                while (SDL_PollEvent(&e) != 0) {
                    done = sdl_event_handler(e, state);
                }
                
                sdl_clear_screen(renderer);
                
                // Draw screen
                sdl_draw_screen(renderer, state);

                // Update the screen
                SDL_RenderPresent(renderer);
            #endif

            #ifdef DEBUG
                curse_graphics(state);
                curse_state(state);
                curse_memory(state, mem_scroll);
                refresh();
                while ((c = getch()) == 'm' || c == 'n') {
                    if (c == 'm') {
                        mem_scroll = min(MEM_SIZE - SHOW_BYTES, mem_scroll + BYTES_PER_LINE);
                    } else {
                        mem_scroll = max(0, mem_scroll - BYTES_PER_LINE);
                    }
                    curse_memory(state, mem_scroll);
                    refresh();
                }
            #endif
       }
    }
    state_delete(state);
    #ifdef SDLMODE
        sdl_end(window, renderer);
    #endif
    return 0;
}
