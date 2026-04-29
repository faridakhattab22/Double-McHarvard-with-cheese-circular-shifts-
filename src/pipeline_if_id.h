// pipeline_if_id.h
#ifndef PIPELINE_IF_ID_H
#define PIPELINE_IF_ID_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "processor.h"
#include "registers.h"
#include "structures.h"



// Instruction Fetch: load instruction at PC into IF/ID register, increment PC.
void stage_IF(ProcessorState *state);

// Instruction Decode: decode IF/ID into ID/EX register.
// Decodes into ALL possible formats (R and I) as required.
void stage_ID(ProcessorState *state);

// Decode raw 16-bit instruction into both R-format and I-format fields.
void decode_instruction(short int raw, Opcode *opcode, InstructionFormat *fmt,
                        int *r1, int *r2, int8_t *imm);

// Sign-extend a 6-bit immediate to int8_t (2's complement).
int8_t sign_extend_6(uint8_t imm6);

// Insert a bubble (NOP) into ID/EX — used on stall or fluoksh.
void insert_bubble_id_ex(ProcessorState *state);

void insert_bubble_if_id(ProcessorState *state);

#endif