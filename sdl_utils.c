
#include "includes/sdl_utils.h"


SDL_Window* sdl_create_window(char* rom_name)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return NULL;
    }
    char window_name[100];
    strcpy(window_name, "CHIP-8 Emulator: ");
    strcat(window_name, rom_name);
    // Create a window
    SDL_Window* window = SDL_CreateWindow(
        window_name,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        DISPLAY_WIDTH * SDL_SCALE,
        DISPLAY_HEIGHT * SDL_SCALE,
        SDL_WINDOW_SHOWN
    );
    return window;
}

void sdl_clear_screen(SDL_Renderer* renderer)
{
    // Clear the screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
    SDL_RenderClear(renderer);
}

void sdl_draw_screen(SDL_Renderer* renderer, emu_state_t* state)
{
    // Draw screen
    for (int row = 0; row < DISPLAY_HEIGHT; ++row) {
        for (int col = 0; col < DISPLAY_WIDTH; ++col) {
            if (state->display[row * DISPLAY_WIDTH + col]) {
                SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff); // blue
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff); // light blue
            }
            SDL_Rect squareRect = { col * SDL_SCALE, row * SDL_SCALE, SDL_SCALE, SDL_SCALE};
            SDL_RenderFillRect(renderer, &squareRect);
        }
    }
}

void sdl_end(SDL_Window* window, SDL_Renderer* renderer)
{
    // Destroy window and renderer
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Quit SDL subsystems
    SDL_Quit();
}

/* Returns whether emulator state is done (aka quit) */
bool sdl_event_handler(SDL_Event e, emu_state_t* state)
{
    // User requests quit
    if (e.type == SDL_QUIT) {
        return true;
    } 
    else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
                return true;
                break;
            case SDLK_1:
                state->keys[0x1] = 1;
                break;
            case SDLK_2:
                state->keys[0x2] = 1;
                break;
            case SDLK_3:
                state->keys[0x3] = 1;
                break;
            case SDLK_4:
                state->keys[0xC] = 1;
                break;
            case SDLK_q:
                state->keys[0x4] = 1;
                break;
            case SDLK_w:
                state->keys[0x5] = 1;
                break;
            case SDLK_e:
                state->keys[0x6] = 1;
                break;
            case SDLK_r:
                state->keys[0xD] = 1;
                break;
            case SDLK_a:
                state->keys[0x7] = 1;
                break;
            case SDLK_s:
                state->keys[0x8] = 1;
                break;
            case SDLK_d:
                state->keys[0x9] = 1;
                break;
            case SDLK_f:
                state->keys[0xE] = 1;
                break;
            case SDLK_z:
                state->keys[0xA] = 1;
                break;
            case SDLK_x:
                state->keys[0x0] = 1;
                break;
            case SDLK_c:
                state->keys[0xB] = 1;
                break;
            case SDLK_v:
                state->keys[0xF] = 1;
                break;
        }
    }
    else if (e.type == SDL_KEYUP) {
        switch (e.key.keysym.sym) {
            case SDLK_1:
                state->keys[0x1] = 0;
                break;
            case SDLK_2:
                state->keys[0x2] = 0;
                break;
            case SDLK_3:
                state->keys[0x3] = 0;
                break;
            case SDLK_4:
                state->keys[0xC] = 0;
                break;
            case SDLK_q:
                state->keys[0x4] = 0;
                break;
            case SDLK_w:
                state->keys[0x5] = 0;
                break;
            case SDLK_e:
                state->keys[0x6] = 0;
                break;
            case SDLK_r:
                state->keys[0xD] = 0;
                break;
            case SDLK_a:
                state->keys[0x7] = 0;
                break;
            case SDLK_s:
                state->keys[0x8] = 0;
                break;
            case SDLK_d:
                state->keys[0x9] = 0;
                break;
            case SDLK_f:
                state->keys[0xE] = 0;
                break;
            case SDLK_z:
                state->keys[0xA] = 0;
                break;
            case SDLK_x:
                state->keys[0x0] = 0;
                break;
            case SDLK_c:
                state->keys[0xB] = 0;
                break;
            case SDLK_v:
                state->keys[0xF] = 0;
                break;
        }

    }
    return false;
}
