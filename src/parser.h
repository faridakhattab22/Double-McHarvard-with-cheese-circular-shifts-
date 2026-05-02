// parser.h
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "processor.h"
#include "structures.h"

// Read assembly file, encode each instruction to 16-bit binary,
// store into state->instr_mem. Returns number of instructions loaded.
int  parse_file(const char *filename, ProcessorState *state);

// Encode a single assembly line to its 16-bit binary representation.
short int encode_instruction(const char *line);

// Map mnemonic string to Opcode enum.
Opcode   mnemonic_to_opcode(const char *mnemonic);

// Convert opcode to its 4-bit binary value (0–11).
int      opcode_to_bits(Opcode op);

#endif
