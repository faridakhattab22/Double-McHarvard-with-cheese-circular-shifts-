
// sreg.h
#ifndef SREG_H
#define SREG_H

#include <stdint.h>
#include "processor.h"




// Master update — call after any ALU op that affects flags.
// Reads result and operands, sets all relevant flags.
void updateSREG(ProcessorState *state, Opcode op,
                int8_t val1, int8_t val2, int8_t result);

// Individual flag checks (used internally by updateSREG)
int  compute_carry(int8_t val1, int8_t val2, Opcode op);
int  compute_overflow(int8_t val1, int8_t val2, int8_t result, Opcode op);
int  compute_negative(int8_t result);
int  compute_zero(int8_t result);
// Sign flag S = N XOR V (computed inside updateSREG)

// Read individual flags from SREG byte
int  flag_C(uint8_t sreg);
int  flag_V(uint8_t sreg);
int  flag_N(uint8_t sreg);
int  flag_S(uint8_t sreg);
int  flag_Z(uint8_t sreg);

#endif