#ifndef __STATE_H
#define __STATE_H

#include <stdint.h>
#include <stdbool.h>


#define ROM_START      0x200
#define STACK_OFFSET   0xEA0
#define FONTSET_SIZE   0x50
#define FONTSET_OFFSET 0x50
#define FONT_SIZE      0x5
#define CYCLE_SUCCESS  0x00

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

#endif // __STATE_H
