
// sreg.h
#ifndef SREG_H
#define SREG_H

#include <stdint.h>
#include "processor.h"

// Farah I used these masks in the output.c dont delete them
// Bit masks for SREG flags
#define FLAG_C_MASK 0b00010000  // Bit 4
#define FLAG_V_MASK 0b00001000  // Bit 3
#define FLAG_N_MASK 0b00000100  // Bit 2
#define FLAG_S_MASK 0b00000010  // Bit 1
#define FLAG_Z_MASK 0b00000001  // Bit 0
#define SREG_CLEAR_MASK 0b00011111 // Keeps bits 7:5 zero



// Master update — call after any ALU op that affects flags.
// Reads result and operands, sets all relevant flags.
void updateSREG(ProcessorState *state, Opcode op,
                int8_t val1, int8_t val2, int8_t result);

// Individual flag checks (used internally by updateSREG)
int  compute_carry(int8_t val1, int8_t val2, Opcode op);
int  compute_overflow(int8_t val1, int8_t val2, int8_t result, Opcode op);
int  compute_negative(int8_t result); // flag N bit 2
int  compute_zero(int8_t result);  //flag zero bit 0 
// Sign flag S = N XOR V (computed inside updateSREG)

// Read individual flags from SREG byte
int  flag_C(uint8_t sreg);
int  flag_V(uint8_t sreg);
int  flag_N(uint8_t sreg);
int  flag_S(uint8_t sreg);
int  flag_Z(uint8_t sreg);

#endif