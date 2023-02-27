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
#include <time.h>
#include <sys/time.h>
#include "emu.h"

#ifdef DEBUG
    #include <ncurses.h>
#endif
#ifdef HARDWARE
    #include "hardware/ssd1306_i2c.h"
#endif

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
    setup_ncurses();

    #ifdef HARDWARE
        hardware_init();
        hardware_rom_message(argv[1]);
    #endif
    #ifdef DEBUG
        // ensure memcpy properly loaded fontset into mem
        // debug_mem(state, FONTSET_OFFSET, FONTSET_OFFSET + FONTSET_SIZE);
        int mem_scroll = ROM_START;
        int c;
    #endif

    file_to_mem(state, argv[1], ROM_START);

    struct timeval current_time, last_cycle_time;
    gettimeofday(&last_cycle_time, NULL);

    bool done = false;
    while (!done) {
        gettimeofday(&current_time, NULL);

       if (((double)current_time.tv_sec - (double)last_cycle_time.tv_sec) >= 1) {
            gettimeofday(&last_cycle_time, NULL);
            done = state_cycle(state);
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
            // update hardware
            #ifdef HARDWARE
                #ifdef DEBUG
                    hardware_refresh_debug(state); // displays state info on oled
                #else
                    hardware_refresh_fullscreen(state);
                #endif
            #endif
       }
    }
    state_delete(state);
    return 0;
}

#ifdef HARDWARE
/*
============================
| Hardware-related funcs   |
============================
*/


/*
    Initializes necessary hardware;
    currently, this only handles the OLED, but later
    it will also do anything necessary for the key matrix.
*/
void hardware_init()
{
    ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
    ssd1306_clearDisplay();
    ssd1306_display();
}

/*
    Displays the name of the rom being loaded on the OLED,
    then waits for MESSAGE_DELAY milliseconds.
*/
void hardware_rom_message(char* rom_name)
{
    // display has been cleared by hardware_init at this point
    char buffer[0x40];
    snprintf(buffer, sizeof(buffer), "loading rom: %s", rom_name);
    ssd1306_drawString(buffer);
    ssd1306_display();
    delay(MESSAGE_DELAY);
    ssd1306_clearDisplay();
}

/*
    Refreshes the current screen after latest instruction has been
    executed. Since CHIP-8 graphics are 64x28, the following scaling
    is done on each pixel to display it onto the 128x64 OLED:

    Scaling reference:
        generally, any pixel (x,y) in an L x W grid will occupy
        the following pixels in a 2L x 2W grid:
    
        (x, y)   -->    (2x,   2y)   (2x+1,   2y)
                        (2x, 2y+1)   (2x+1, 2y+1)	

*/
void hardware_refresh_fullscreen(emu_state_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    ssd1306_clearDisplay();
    for (int row = 0; row < DISPLAY_HEIGHT; row++) {
        for (int col = 0; col < DISPLAY_WIDTH; col++) {
            bool color = state->display[row * DISPLAY_WIDTH + col];
            for (int c_offset = 0; c_offset <= 1; c_offset++) {
                for (int r_offset = 0; r_offset <= 1; r_offset++) {
                    ssd1306_drawPixel(2 * col + c_offset, 2 * row + r_offset, color);
                }	
            }
        }
    }
    ssd1306_display();
}

/*
    Similar to hardware_refresh_fullscreen, except it displays the CHIP-8
    64x32 graphics in a 1:1 manner in a 64x32 segment of the OLED. The
    rest of the OLED is used to display debug info (currently only the)
    opcode being executed.
*/
void hardware_refresh_debug(emu_state_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    ssd1306_clearDisplay();
    int buflen = 0x40;
    char buffer[buflen];
    // later, write disassembler to translate opcode to text form for easier debugging
    snprintf(buffer, buflen, "op:%02x%02x", state->memory[state->pc], state->memory[state->pc+1]);
    ssd1306_drawString(buffer);
    for (int row = 0; row < DISPLAY_HEIGHT; row++) {
        for (int col = 0; col < DISPLAY_WIDTH; col++) {
            bool color = state->display[row * DISPLAY_WIDTH + col];
            ssd1306_drawPixel(col, row + DISPLAY_HEIGHT, color);
        }
    }
    ssd1306_display();
}

#endif

/*
============================
| State-handling functions |
============================
*/

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
                    keypress = 0; 
                    // TODO - implement this feature - stop all execution until keypress
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


/*
    Deletes current emulator state.
*/
void state_delete(emu_state_t* state)
{
    /* ... */
    free(state);
}

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
    if (set_carry) {
        state->registers[0xF] = result >= 0x100;
    }
    *destination = (0xff & result);
}


void SUB(emu_state_t* state, uint8_t reg_index1, uint8_t reg_index2)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    uint16_t result = state->registers[reg_index1] - state->registers[reg_index2];
    state->registers[0xF] = state->registers[reg_index1] > state->registers[reg_index2];
    state->registers[reg_index1] = (0xff & result);
}

void SUBN(emu_state_t* state, uint8_t reg_index1, uint8_t reg_index2)
{
    if (state == NULL) {
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    uint16_t result = state->registers[reg_index2] - state->registers[reg_index1];
    state->registers[0xF] = state->registers[reg_index2] > state->registers[reg_index1];
    state->registers[reg_index1] = (0xff & result);
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
    state->registers[0xF] = state->registers[reg_index1] & 1;
    state->registers[reg_index1] >>= 1;
}

void SHL(emu_state_t* state, uint8_t reg_index1)
{
    if (state == NULL) {    
        fprintf(stderr, "error: null state\n");
        exit(1);
    }
    state->registers[0xF] = state->registers[reg_index1] & 0x80;
    state->registers[reg_index1] <<= 1;
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

