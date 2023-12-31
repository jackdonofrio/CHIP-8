
/*
State handling funcs
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "includes/opcodes.h"
#include "includes/state.h"


const uint8_t fontset[FONTSET_SIZE] = 
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

/*
    Generates new emulator state.
*/
emu_state_t* state_new()
{
    emu_state_t* state = malloc(sizeof(emu_state_t));
    if (state == NULL) {
        fprintf(stderr, "error: unable to allocate memory for emu state\n");
        return NULL;
    }
    /* ... */
    return state;
}


/*
    Handles misc tasks necessary for starting emulator state
*/
void state_init(emu_state_t* state)
{
    state->pc = ROM_START;
    state->sp = STACK_OFFSET;
    memcpy(&(state->memory[FONTSET_OFFSET]), fontset, FONTSET_SIZE);
}

/*
    Deletes current emulator state.
*/
void state_delete(emu_state_t* state)
{
    /* ... */
    free(state);
}


/*
    Performs fetch -> decode -> execute.
*/
int state_cycle(emu_state_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    uint16_t instruction = (state->memory[state->pc] << 8) | (state->memory[state->pc + 1]) ;
    state->pc += 2;

    uint8_t first_nibble = (instruction & 0xf000) >> 12;
    uint8_t second_nibble = (instruction & 0xf00) >> 8;
    uint8_t third_nibble = (instruction & 0xf0) >> 4;
    uint8_t fourth_nibble = (instruction & 0xf);
    uint8_t keypress;

    // decode instruction
    switch (first_nibble) {
        case 0x0:
            switch (instruction) {
                case 0x00E0:
                    CLS(state);
                    break;
                case 0x0EE:
                    RET(state);
                    break;
                default:
                    break;
            }
            break;
        case 0x1:
            JP(state, instruction & 0x0fff);
            break;
        case 0x2:
            CALL(state, instruction & 0x0fff);
            break;
        case 0x3:
            SE(state, state->registers[second_nibble], (uint8_t)(instruction & 0xff));
            break;
        case 0x4:
            SNE(state, state->registers[second_nibble], (uint8_t)(instruction & 0xff));
            break;
        case 0x5:
            SE(state, state->registers[second_nibble], state->registers[third_nibble]);
            break;
        case 0x6:
            state->registers[second_nibble] = (uint8_t)(instruction & 0xff);
            break;
        case 0x7:
            ADD(state, &(state->registers[second_nibble]), (uint8_t)(instruction & 0xff), false);
            break;
        case 0x8:
            switch (fourth_nibble) {
                case 0x0:
                    state->registers[second_nibble] = state->registers[third_nibble];
                    break;
                case 0x1:
                    OR(state, second_nibble, third_nibble);
                    break;
                case 0x2:
                    AND(state, second_nibble, third_nibble);
                    break;
                case 0x3:
                    XOR(state, second_nibble, third_nibble);
                    break;
                case 0x4:
                    ADD(state, &(state->registers[second_nibble]), state->registers[third_nibble], true);
                    break;
                case 0x5:
                    SUB(state, second_nibble, third_nibble);
                    break;
                case 0x6:
                    SHR(state, second_nibble);
                    break;
                case 0x7:
                    SUBN(state, second_nibble, third_nibble);
                    break;
                case 0xE:
                    SHL(state, second_nibble);
                    break;
            }
            break;
        case 0x9:
            SNE(state, state->registers[second_nibble], state->registers[third_nibble]);
            break;
        case 0xA:
            LD(state, &(state->index), instruction & 0x0fff);
            break;
        case 0xB:
            JP(state, (instruction & 0x0fff) + state->registers[0x0]);
            break;
        case 0xC:
            RND(state, second_nibble, (uint8_t) (instruction & 0xff));
            break;
        case 0xD:
            DRW(state, second_nibble, third_nibble, fourth_nibble);
            break;
        case 0xE:
            switch (third_nibble) {
                case 0x9:
                    SKP(state, second_nibble, true);
                    break;
                case 0xA:
                    SKP(state, second_nibble, false);
                    break;
            }
            break;
        case 0xF:
            switch (instruction & 0xff) {
                case 0x07:
                    state->registers[second_nibble] = state->delay_timer;
                    break;
                case 0x0A:
                    keypress = 0; // TODO - update this
                    for (int k = 0; k < 0x10; k++) {
                        if (state->keys[k]) {
                            keypress = k;
                            break;
                        }
                    } 
                    // stop all execution until keypress
                    // then store value in Vx
                    state->registers[second_nibble] = keypress;
                    break;
                case 0x15:
                    state->delay_timer = state->registers[second_nibble];
                    break;
                case 0x18:
                    state->sound_timer = state->registers[second_nibble];
                    break;
                case 0x1E:
                    state->index += state->registers[second_nibble];
                    break;
                case 0x29:
                    state->index = state->registers[second_nibble] * FONT_SIZE + FONTSET_OFFSET;
                    break;
                case 0x33:
                    state->memory[state->index + 2] = state->registers[second_nibble] % 10;
                    state->memory[state->index + 1] = (state->registers[second_nibble] / 10) % 10;
                    state->memory[state->index] = (state->registers[second_nibble] / 100) % 10;
                    break;
                case 0x55:
                    for (int i = 0; i <= second_nibble; i++) {
                        state->memory[state->index + i] = state->registers[i];
                    }
                    break;
                case 0x65:
                    for (int i = 0; i <= second_nibble; i++) {
                        state->registers[i] = state->memory[state->index + i];
                    }
                    break;
            }
            break;
    } 


    if (state->delay_timer > 0) {
        state->delay_timer--;
    }
    if (state->sound_timer > 0) {
        state->sound_timer--;
    }

    return CYCLE_SUCCESS;
}
