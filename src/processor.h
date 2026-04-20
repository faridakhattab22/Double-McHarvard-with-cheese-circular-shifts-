#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef enum {
    ADD, SUB, MUL, LDI,
    BEQZ, AND, OR, JR,
    SAL, SAR, LB, SB
} Opcode;

typedef enum {
    FORMAT_R,   // opcode | R1 | R2
    FORMAT_I    // opcode | R1 | IMMEDIATE
} InstructionFormat;

typedef enum {
    HAZARD_NONE,
    HAZARD_STALL,
    HAZARD_FORWARD_EX
} HazardType;

typedef enum {
    FLUSH_NONE,
    FLUSH_TAKEN      // branch or JR taken — drop IF + ID
} FlushType;

#endif