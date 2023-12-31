#ifndef __EMU_H
#define __EMU_H

#include <stdint.h>
#ifdef DEBUG
    #include <ncurses.h>
#endif

#define FONTSET_SIZE   0x50
#define FONTSET_OFFSET 0x50
#define FONT_SIZE      0x5
#define MEM_START      0x0
#define MEM_SIZE       0x1000
#define SHOW_BYTES     0x280
#define BYTES_PER_LINE 0x10
#define ROM_START      0x200
#define STACK_OFFSET   0xEA0
#define DISPLAY_HEIGHT 0x20
#define DISPLAY_WIDTH  0x40
#define CYCLE_DELAY    0x00
#define CYCLE_SUCCESS  0x00
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

typedef struct emu_state {
    uint8_t registers[0x10];
    uint8_t memory[0x1000];
    uint16_t index; // stores 12-bit mem addr
    uint16_t pc; // adr of next instruction
    uint16_t sp;
    uint8_t delay_timer; // timer - if zero, stays zero; if >0, decrement at 60hz
    uint8_t sound_timer; // if 0, play sound; if >0, decrement at 60hz
    uint8_t keys[0x10];
    bool display[0x800];
} emu_state_t;

const uint8_t fontset[FONTSET_SIZE];

emu_state_t* state_new();
void state_init(emu_state_t* state);
int state_cycle(emu_state_t* state);
void state_delete(emu_state_t* state);


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



void PUSH(emu_state_t* state, uint16_t value);
uint16_t POP(emu_state_t* state);
void CLS(emu_state_t* state);
void RET(emu_state_t* state);
void JP(emu_state_t* state, uint16_t address);
void CALL(emu_state_t* state, uint16_t address);
void SE(emu_state_t* state, uint8_t byte1, uint8_t byte2);
void SNE(emu_state_t* state, uint8_t byte1, uint8_t byte2);
void LD(emu_state_t* state, uint16_t* destination, uint16_t value);
void ADD(emu_state_t* state, uint8_t* destination, uint8_t value, bool set_carry);
void SUB(emu_state_t* state, uint8_t stack_index1, uint8_t stack_index2);
void SHR(emu_state_t* state, uint8_t stack_index1);
void SHL(emu_state_t* state, uint8_t stack_index1);
void OR(emu_state_t* state, uint8_t stack_index1, uint8_t stack_index2);
void AND(emu_state_t* state, uint8_t stack_index1, uint8_t stack_index2);
void XOR(emu_state_t* state, uint8_t stack_index1, uint8_t stack_index2);
void RND(emu_state_t* state, uint8_t stack_index, uint8_t byte);
void SUBN(emu_state_t* state, uint8_t stack_index1, uint8_t stack_index2);
void DRW(emu_state_t* state, uint8_t stack_index1, uint8_t stack_index2, uint8_t nibble);
void SKP(emu_state_t* state, uint8_t stack_index, bool checking_pressed);


#endif // EMU_H
