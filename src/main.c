#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "processor.h"
#include "structures.h"
#include "registers.h"
#include "output.h"
#include "pipeline_if_id.h"
#include "parser.h"
#include "flush.h"
#include "alu_hazards.h"



int main(void) {
    // initialise processor,registers (start as bubbles) including Sreg in initialisation
    // instructions memory +  Data memory

    ProcessorState state;
    memset(&state, 0, sizeof(ProcessorState));

    init_registers(&state);
    init_data_memory(&state);
    init_inst_memory(&state);

    state.if_id.valid     = 0;
    state.id_ex.valid     = 0;
    state.ex_out.valid    = 0;
    state.ex_out.dest_reg = -1;
    state.clock_cycle     = 1;

    //  Load program
    const char *filename = "program_1.asm";

    int loaded = parse_file(filename, &state);
    if (loaded <= 0) {
        fprintf(stderr, "Error: failed to load '%s'\n", filename);
        return 1;
    }

    printf("Loaded %d instructions from %s\n", loaded, filename);
    print_instruction_memory(&state);

    // ── Pipeline loop ─────────────────────────────────────────────────────────
    while (state.pc <= state.instr_count) {
        print_cycle_header(state.clock_cycle);

        stage_EX(&state);
        print_stage_EX(&state);

        stage_ID(&state);
        print_stage_ID(&state);

        if (state.pc < state.instr_count)
            stage_IF(&state);
        print_stage_IF(&state);

        print_all_registers(&state);
        print_sreg(&state);
        print_data_memory(&state);

        state.clock_cycle++;
    }

    // ── Final state ───────────────────────────────────────────────────────────
    printf("\n════════════════════════════════════════\n");
    printf("  Simulation complete — %d cycles\n", state.clock_cycle - 1);
    printf("════════════════════════════════════════\n");
    print_all_registers(&state);
    print_sreg(&state);
    print_instruction_memory(&state);
    print_data_memory(&state);

    return 0;
}


/*

LDI R1, 42

LDI R1, 10
LDI R2, 20
ADD R1, R2

LDI R1, 30
LDI R2, 10
SUB R1, R2

LDI R1, 5
LDI R2, 6
MUL R1, R2

LDI R1, 12
LDI R2, 10
AND R1, R2

LDI R1, 12
LDI R2, 10
OR R1, R2

LDI R1, 4
LDI R2, 2
SAL R1, R2

LDI R1, 16
LDI R2, 2
SAR R1, R2

LDI R1, 99
SB  R1, R2, 0

LDI R1, 5
SB  R1, R2, 0
LB  R3, R2, 0

LDI R1, 0
BEQZ R1, 2
LDI R2, 99
LDI R3, 42

LDI R1, 0
LDI R2, 5
JR  R1, R2
LDI R3, 99
LDI R4, 42

hazards

LDI R2, 0
LDI R1, 55
SB  R1, R2, 0
LB  R3, R2, 0
ADD R3, R1 
*/