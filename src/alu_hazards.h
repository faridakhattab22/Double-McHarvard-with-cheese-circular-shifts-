
// alu_hazards.h
#ifndef ALU_HAZARDS_H
#define ALU_HAZARDS_H

#include <stdint.h>

// Execute stage: performs ALU op, writeback, calls updateSREG.
void stage_EX(ProcessorState *state);

// ALU operations (pure functions, return result)
int8_t alu_add(int8_t a, int8_t b);
int8_t alu_sub(int8_t a, int8_t b);
int8_t alu_mul(int8_t a, int8_t b);
int8_t alu_and(int8_t a, int8_t b);
int8_t alu_or (int8_t a, int8_t b);
int8_t alu_sal(int8_t a, int8_t imm);   // arithmetic left shift
int8_t alu_sar(int8_t a, int8_t imm);   // arithmetic right shift

// Hazard detection: inspect ID/EX vs previous EX result.
HazardType detect_hazard(ProcessorState *state);

// Insert stall cycle: freeze IF/ID, insert bubble into ID/EX.
void insert_stall(ProcessorState *state);

#endif