#ifndef __EMU_H
#define __EMU_H

#include <stdint.h>
#include "state.h"
#ifdef DEBUG
    #include <ncurses.h>
#endif


#define MEM_START      0x0
#define MEM_SIZE       0x1000
#define SHOW_BYTES     0x280
#define BYTES_PER_LINE 0x10
#define DISPLAY_HEIGHT 0x20
#define DISPLAY_WIDTH  0x40
#define CYCLE_DELAY    0x00
#define KRED  "\x1B[31m"
#define RESET "\033[0m"
#define MESSAGE_DELAY 5000 // milliseconds
#define SDL_SCALE 10


#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })



void file_to_mem(emu_state_t* state, char* filename, uint16_t address);

#ifdef DEBUG
	void debug_mem(emu_state_t* state, uint16_t start, uint16_t end);
	void debug_state(emu_state_t* state);
	void debug_graphics(emu_state_t* state);
	void setup_ncurses();
	void curse_graphics(emu_state_t* state);
	void curse_state(emu_state_t* state);
	void curse_memory(emu_state_t* state, int start_offset);
	void curse_clearlines(int start_row, int inclusive_end_row, int column);
#endif




#endif // EMU_H
