#include "structures.h"
#include "flush.h"
#include <stdint.h>

short int compute_beqz_target(short int stored_pc, int8_t imm) {
    return stored_pc  + imm;
}

short int compute_jr_target(int8_t r1_val, int8_t r2_val) {
    return (short int)((r1_val << 8) | (r2_val & 0xFF));
}

void flush_pipeline(ProcessorState *state, short int new_pc) {
    state->pc = new_pc;

    state->if_id.valid = 0;
    state->id_ex.valid = 0;
}