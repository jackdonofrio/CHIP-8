
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "includes/emu.h"
#include "includes/opcodes.h"


/*
============================
| Instruction functions    |
============================
*/

void PUSH(emu_state_t* state, uint16_t value)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    state->memory[state->sp] = (value & 0xff00) >> 8;
    state->memory[state->sp + 1] = value & 0xff;
    state->sp += 2;
}

uint16_t POP(emu_state_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    state->sp -= 2;
    return (state->memory[state->sp] << 8) | state->memory[state->sp + 1]; 
}

/*
clears display
*/
void CLS(emu_state_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    memset(state->display, false, sizeof(state->display));
}

/*
returns from subroutine
*/
void RET(emu_state_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    state->pc = POP(state);
}

void JP(emu_state_t* state, uint16_t address)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    state->pc = address;
}

void CALL(emu_state_t* state, uint16_t address)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    PUSH(state, state->pc);
    JP(state, address);
}

/*
generalized - skip next instr if bytes are equal
*/
void SE(emu_state_t* state, uint8_t byte1, uint8_t byte2)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    if (byte1 == byte2) {
        state->pc += 2;
    }
}

/*
generalized - skip next instr if bytes aren't equal
*/
void SNE(emu_state_t* state, uint8_t byte1, uint8_t byte2)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    if (byte1 != byte2) {
        state->pc += 2;
    }
}

/*
loads one word into another
*/
void LD(emu_state_t* state, uint16_t* destination, uint16_t value)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    *destination = value;
}


/*
adds byte to register
*/
void ADD(emu_state_t* state, uint8_t* destination, uint8_t value, bool set_carry)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    uint16_t result = *destination + value;
    *destination = (0xFF & result);
	if (set_carry) {
        state->registers[0xF] = result > 0xFF;
    }
}


void SUB(emu_state_t* state, uint8_t reg_index1, uint8_t reg_index2)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }

    uint8_t carry = state->registers[reg_index1] >= state->registers[reg_index2];
    state->registers[reg_index1] = state->registers[reg_index1] - state->registers[reg_index2];
    state->registers[0xF] = carry;
}


void SUBN(emu_state_t* state, uint8_t reg_index1, uint8_t reg_index2)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    uint8_t carry = state->registers[reg_index2] >= state->registers[reg_index1];
    state->registers[reg_index1] = state->registers[reg_index2] - state->registers[reg_index1];
    state->registers[0xF] = carry;
}

void OR(emu_state_t* state, uint8_t reg_index1, uint8_t reg_index2)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    state->registers[reg_index1] |= state->registers[reg_index2];
}

void AND(emu_state_t* state, uint8_t reg_index1, uint8_t reg_index2)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    state->registers[reg_index1] &= state->registers[reg_index2];
}

void XOR(emu_state_t* state, uint8_t reg_index1, uint8_t reg_index2)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    state->registers[reg_index1] ^= state->registers[reg_index2];
}

void SHR(emu_state_t* state, uint8_t reg_index1)
{
    if (state == NULL) {    
        fprintf(stderr, "error: null state\n");
        exit(1);
    }

    uint8_t fl = state->registers[reg_index1] & 1;
    state->registers[reg_index1] >>= 1;
    state->registers[0xF] = fl;
}

void SHL(emu_state_t* state, uint8_t reg_index1)
{
    if (state == NULL) {    
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    uint8_t carry = state->registers[reg_index1] >> 7;
    state->registers[reg_index1] <<= 1;
    state->registers[0xF] = carry;
}

// For JP V0, simply call JP(state, addr + v0);

void RND(emu_state_t* state, uint8_t reg_index, uint8_t byte)
{
    if (state == NULL) {    
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    state->registers[reg_index] = byte & (rand() & 0xff);
}

// adapted from tutorial by austinmorlan
void DRW(emu_state_t* state, uint8_t reg_index1, uint8_t reg_index2, uint8_t nibble)
{
    if (state == NULL) {    
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    uint8_t x = state->registers[reg_index1] % DISPLAY_WIDTH; // for wrap-around
    uint8_t y = state->registers[reg_index2] % DISPLAY_HEIGHT;

    state->registers[0xF] = 0;

    for (uint8_t row = 0; row < nibble; row++)
    {
        uint8_t sprite_byte = state->memory[state->index + row];
        for (uint8_t column = 0; column < 8; column++)
        {
            uint8_t sprite_pixel = sprite_byte & (0x80 >> column);
            bool* screen_pixel = &(state->display[(y + row) * DISPLAY_WIDTH + x + column]);
            if (sprite_pixel)
            {
                if (*screen_pixel)
                {
                    state->registers[0xF] = 1;
                }
                *screen_pixel ^= true;
            }
        }
    }
}




void SKP(emu_state_t* state, uint8_t reg_index, bool checking_pressed)
{
    if (state == NULL) {    
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    if ((state->keys[state->registers[reg_index]] & 1) == checking_pressed) {
        state->pc += 2;
    }
}