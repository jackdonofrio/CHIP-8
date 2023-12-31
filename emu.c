/*
CHIP-8 emulator by Jack Donofrio

usage: emu <rom file>

or simply `make test` for a quick demo

Memory Notes:
    remember 12-bit address space
        0x000-0x1ff: generally reserved
            0x050-0xa0: storing characters 0-f
        0x200-0xfff:
            rom is loaded at 0x200
            everything after rom is free
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "includes/opcodes.h"
#include "includes/emu.h"


/*
=======================
| Misc mem funcs      |
=======================
*/

/*
    reads bytes from file into memory starting at given address
    (adapted from code in my 8080 emu which was adapted from an Emulator101 tutorial)
*/
void file_to_mem(emu_state_t* state, char* filename, uint16_t address)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    if (filename == NULL) {
        fprintf(stderr, "erorr: null filename ptr\n");
        exit(1);
    }
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "error: unable to open %s\n", filename);
        exit(1);
    }
    fseek(fp, 0L, SEEK_END);
    int fsize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    uint8_t* mem_buffer = &(state->memory[address]);
    fread(mem_buffer, fsize, 1, fp);
    fclose(fp);
}

/*
=======================
| Debugging funcs     |
=======================
*/
#ifdef DEBUG

/*
    prints current memory state to stdout, with PC position in red
*/
void debug_mem(emu_state_t* state, uint16_t start, uint16_t end)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    printf("Memory:");
    for (int i = start; i < end; i++) {
        if (i % BYTES_PER_LINE == 0) {
            if (i < 0x100) {
                printf("\n0x0%02x: ", i);
            } else {
                printf("\n0x%02x: ", i);
            }
        }
        if (i == state->pc) {
            printf(KRED"%02x " RESET, state->memory[i]);
        } else {
            printf("%02x ", state->memory[i]);
        }
    }
    printf("\n");
}

/*
    prints ascii representation of 64x32 screen to stdout
*/
void debug_graphics(emu_state_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    printf("Graphics:\n");
    for (int row = 0; row < DISPLAY_HEIGHT; row++) {
        for (int col = 0; col < DISPLAY_WIDTH; col++) {
            if (state->display[row * DISPLAY_WIDTH + col]) {
                printf("#");
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
}

/*
    Prints misc. state information (registers, stack, current opcode) to stdout.
*/
void debug_state(emu_state_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    printf("Opcode: %02x%02x\n", state->memory[state->pc], state->memory[state->pc+1]);
    printf("Registers:\n");
    for (int reg_index = 0; reg_index < 0x8; reg_index++) {
        printf("0x%02x:%02x ", reg_index, state->registers[reg_index]);
    }
    printf("\n");
    for (int reg_index = 8; reg_index < 0x10; reg_index++) {
        printf("0x%02x:%02x ", reg_index, state->registers[reg_index]);
    }
    printf("\n");
    printf("Index: %02x%02x | PC: %02x%02x | SP: %02x | Delay Timer %02x | Sound Timer %02x\n",
        state->index >> 8, state->index & 0xff, state->pc >> 8, state->pc & 0xff,
        state->sp, state->delay_timer, state->sound_timer);
    printf("Stack:\n");
    for (int stack_index = 0; stack_index < 0x8; stack_index++) {
        printf("0x%02x:%02x ", stack_index, state->memory[STACK_OFFSET + stack_index]);
    }
    printf("\n");
    for (int stack_index = 8; stack_index < 0x10; stack_index++) {
        printf("0x%02x:%02x ", stack_index, state->memory[STACK_OFFSET + stack_index]);
    }
    printf("\n");
}


void setup_ncurses()
{
    initscr();
    cbreak();
    noecho();
    start_color();
    init_pair('#', COLOR_YELLOW, COLOR_BLACK);
    init_pair('?', COLOR_CYAN, COLOR_BLACK); // for mem addresses
    init_pair('M', COLOR_GREEN, COLOR_BLACK);
}

void curse_graphics(emu_state_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    curse_clearlines(0, DISPLAY_HEIGHT, 0);
    attron(COLOR_PAIR('#'));
    for (int row = 0; row < DISPLAY_HEIGHT; row++) {
        for (int col = 0; col < DISPLAY_WIDTH; col++) {
            if (state->display[row * DISPLAY_WIDTH + col]) {
                mvaddch(row, col, '#');
            } else {
                mvaddch(row, col, ' ');
            }
        }
    }
    attroff(COLOR_PAIR('#'));
}

void curse_clearlines(int start_row, int inclusive_end_row, int column)
{
    for (int line = start_row; line <= inclusive_end_row; line++) {
        move(line, column);
        clrtoeol();
    }
}

void curse_state(emu_state_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    int row_offset = DISPLAY_HEIGHT;
    curse_clearlines(row_offset, row_offset + 8, 0);
    mvprintw(row_offset, 0, "Opcode: %02x%02x\n", state->memory[state->pc], state->memory[state->pc+1]);
    mvprintw(row_offset + 1, 0, "Registers");
    for (int reg_index = 0; reg_index < 0x8; reg_index++) {
        mvprintw(row_offset + 2, reg_index * 9, "0x%02x:%02x ", reg_index, state->registers[reg_index]);
    }
    for (int reg_index = 8; reg_index < 0x10; reg_index++) {
        mvprintw(row_offset + 3, (reg_index-8) * 9, "0x%02x:%02x ", reg_index, state->registers[reg_index]);
    }
    mvprintw(row_offset + 4, 0, "Index: %02x%02x | PC: %02x%02x | SP: %02x | Delay Timer %02x | Sound Timer %02x\n",
        state->index >> 8, state->index & 0xff, state->pc >> 8, state->pc & 0xff,
        state->sp, state->delay_timer, state->sound_timer);
    mvprintw(row_offset + 5, 0, "Stack");
    for (int stack_index = 0; stack_index < 0x8; stack_index++) {
        mvprintw(row_offset + 6, stack_index * 9, "0x%02x:%02x ", stack_index, state->memory[STACK_OFFSET + stack_index]);
    }
    for (int stack_index = 8; stack_index < 0x10; stack_index++) {
        mvprintw(row_offset + 7, (stack_index-8) * 9, "0x%02x:%02x ", stack_index, state->memory[STACK_OFFSET + stack_index]);
    }
    attron(COLOR_PAIR('#'));
    mvprintw(row_offset + 8, 0, "Hit m to scroll down memory, n to scroll up, and other key for next instruction.");
    attroff(COLOR_PAIR('#'));
    attron(COLOR_PAIR('M'));
    mvprintw(row_offset + 9, 0, "PC is highlighted as green in memory.");
    attroff(COLOR_PAIR('M'));
    // TODO - also highlight stackp
}

void curse_memory(emu_state_t* state, int start_offset)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    const int width_offset = DISPLAY_WIDTH + 12;
    curse_clearlines(0, DISPLAY_HEIGHT, width_offset);
    for (int i = start_offset; i < start_offset + SHOW_BYTES; i++) {
        int curse_row = (i - start_offset) / BYTES_PER_LINE;
        if (i % BYTES_PER_LINE == 0) {
            attron(COLOR_PAIR('?'));
            if (i < 0x100) {
                mvprintw(curse_row, width_offset, "0x0%02x: ", i);
            } else {
                mvprintw(curse_row, width_offset, "0x%02x: ", i);
            }
            attroff(COLOR_PAIR('?'));
        }
        if (i == state->pc) {
            attron(COLOR_PAIR('M'));
            mvprintw(curse_row, width_offset + 8 + (i % BYTES_PER_LINE) * 4, "%02x ", state->memory[i]);
            attroff(COLOR_PAIR('M'));
        } else {
            mvprintw(curse_row, width_offset + 8 + (i % BYTES_PER_LINE) * 4, "%02x ", state->memory[i]);
        }
    }
    // printf("\n");
}

#endif



