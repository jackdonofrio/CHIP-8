#ifndef __SDL_UTILS_H
#define __SDL_UTILS_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "emu.h"


SDL_Window* sdl_create_window(char* rom_name);
void sdl_clear_screen(SDL_Renderer* renderer);
void sdl_draw_screen(SDL_Renderer* renderer, emu_state_t* state);

void sdl_end(SDL_Window* window, SDL_Renderer* renderer);

bool sdl_event_handler(SDL_Event e, emu_state_t* state);


#endif // __SDL_UTILS_H