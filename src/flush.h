// flush.h
#ifndef FLUSH_H
#define FLUSH_H

#include <stdint.h>
#include <stdio.h>
#include "processor.h"
#include "registers.h"
#include "pipeline_if_id.h"   // needs insert_bubble_if_id and insert_bubble_id_ex



// Evaluate branch/jump outcome after EX resolves.
// Returns FLUSH_TAKEN or FLUSH_NONE.
FlushType evaluate_branch(ProcessorState *state);

// Compute BEQZ target: stored_pc + 1 + sign_extended_imm
uint16_t compute_beqz_target(uint16_t stored_pc, int8_t imm);

// Compute JR target: R1[7:0] || R2[7:0] (concatenation)
uint16_t compute_jr_target(int8_t r1_val, int8_t r2_val);

// Perform flush: invalidate IF/ID and ID/EX pipeline registers,
// set PC to new target.
void flush_pipeline(ProcessorState *state, uint16_t new_pc);
#endif