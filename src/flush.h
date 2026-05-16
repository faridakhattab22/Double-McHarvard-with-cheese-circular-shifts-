#ifndef FLUSH_H
#define FLUSH_H

#include <stdint.h>
#include <stdio.h>
#include "processor.h"
#include "registers.h"
#include "pipeline_if_id.h"   



FlushType evaluate_branch(ProcessorState *state);

short int compute_beqz_target(short int stored_pc, int8_t imm);

short int compute_jr_target(int8_t r1_val, int8_t r2_val);


void flush_pipeline(ProcessorState *state, short int new_pc);
#endif