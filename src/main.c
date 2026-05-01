#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "processor.h"
#include "parser.h"
#include "registers.h"
#include "sreg.h"
#include "pipeline_if_id.h"
#include "alu_hazards.h"
#include "flush.h"
#include "output.h"

// Main Logic 


 


// send assembly file to parser 


// while theres no instructions left 

    // execute 
    // print EX stage

    // decode
    // print ID stage

    // fetch
    // print IF stage

    // print all registers, SREG
    // instructions memory, data memory

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
    while (state->pc <= state->instr_count) {
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