#ifndef __OPCODES_H
#define __OPCODES_H

#include <stdbool.h>
#include <stdint.h>
#include "state.h"



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


#endif // __OPCODES_H