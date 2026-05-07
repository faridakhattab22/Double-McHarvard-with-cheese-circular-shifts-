#include <stdio.h>
#include <stdint.h>
#include "output.h"
#include "processor.h"
#include "structures.h"
#include "sreg.h"

// ─── Cycle Header ────────────────────────────────────────────────────────────

void print_cycle_header(int cycle) {
    printf("\n╔══════════════════════════════════════╗\n");
    printf(  "║           CLOCK CYCLE %-3d             ║\n", cycle);
    printf(  "╚══════════════════════════════════════╝\n");
}

// ─── Stage Printers ──────────────────────────────────────────────────────────

void print_stage_IF(ProcessorState *state) {
    printf("  [IF] ");
    if (!state->if_id.valid) {
        printf("(bubble)\n");
        return;
    }
    printf("PC=%-4d  raw=0x%04X\n",
           state->if_id.pc,
           (unsigned short)state->if_id.instruction);
}

void print_stage_ID(ProcessorState *state) {
    printf("  [ID] ");
    if (!state->id_ex.valid) {
        printf("(bubble)\n");
        return;
    }

    // print opcode name
    const char *opname = "???";
    switch (state->id_ex.opcode) {
        case ADD:  opname = "ADD";  break;
        case SUB:  opname = "SUB";  break;
        case MUL:  opname = "MUL";  break;
        case AND:  opname = "AND";  break;
        case OR:   opname = "OR";   break;
        case SAL:  opname = "SAL";  break;
        case SAR:  opname = "SAR";  break;
        case LDI:  opname = "LDI";  break;
        case LB:   opname = "LB";   break;
        case SB:   opname = "SB";   break;
        case BEQZ: opname = "BEQZ"; break;
        case JR:   opname = "JR";   break;
        default:   opname = "???";  break;
    }

    if (state->id_ex.format == FORMAT_R) {
        printf("%-4s  R%d, R%d\n", opname, state->id_ex.r1, state->id_ex.r2);
    } else {
        printf("%-4s  R%d, #%d\n", opname, state->id_ex.r1, (int)state->id_ex.imm);
    }
}

void print_stage_EX(ProcessorState *state) {
    printf("  [EX] ");
    if (!state->ex_out.valid) {
        printf("(bubble)\n");
        return;
    }

    if (state->ex_out.flush == FLUSH_TAKEN) {
        printf("branch/jump taken → new PC=%d\n", (int)state->ex_out.new_pc);
        return;
    }

    if (state->ex_out.dest_reg == -1) {
        printf("no writeback  result=0x%02X\n", (uint8_t)state->ex_out.result);
    } else {
        printf("R%-2d ← %-4d  (0x%02X)\n",
               state->ex_out.dest_reg,
               (int)state->ex_out.result,
               (uint8_t)state->ex_out.result);
    }
}

// ─── Change Logging ──────────────────────────────────────────────────────────

void log_register_change(int reg_index, int8_t new_value, int cycle) {
    // printf("  [WB,  cycle %d] R%d ← %d (0x%02X)\n",
    //        cycle, reg_index, (int)new_value, (uint8_t)new_value);
}

void log_memory_change(short int address, uint8_t new_value, int cycle) {
    printf("  [MEM, cycle %d] mem[0x%04X] ← %u (0x%02X)\n",
           cycle, (unsigned short)address, new_value, new_value);
}

// ─── Final State Dump ────────────────────────────────────────────────────────

void print_all_registers(ProcessorState *state) {
    printf("\n┌─────────────────────────────────────┐\n");
    printf(  "│           REGISTER FILE             │\n");
    printf(  "├──────┬──────────┬───────────────────┤\n");
    printf(  "│  Reg │  Dec     │  Hex              │\n");
    printf(  "├──────┼──────────┼───────────────────┤\n");
    for (int i = 0; i < 64; i++) {
        int8_t v = state->regs[i];
        // if (v != 0 || i == 0) { // always print R0, skip zero regs
            printf("│  R%-2d │  %-7d │  0x%02X             │\n",
                  i, (int)v, (uint8_t)v);
        // }
    }
    printf("└──────┴──────────┴───────────────────┘\n");
}

void print_sreg(ProcessorState *state) {
    uint8_t s = state->sreg;
    printf("\n[SREG] Z=%d  S=%d  N=%d  V=%d  C=%d\n",
           (s & FLAG_Z_MASK) ? 1 : 0,   // bit 0
           (s & FLAG_S_MASK) ? 1 : 0,   // bit 1
           (s & FLAG_N_MASK) ? 1 : 0,   // bit 2
           (s & FLAG_V_MASK) ? 1 : 0,   // bit 3
           (s & FLAG_C_MASK) ? 1 : 0);  // bit 4
}

void print_instruction_memory(ProcessorState *state) {
    printf("\n┌─────────────────────────────────────┐\n");
    printf(  "│         INSTRUCTION MEMORY          │\n");
    printf(  "├────────┬────────────────────────────┤\n");
    printf(  "│  Addr  │  Instruction               │\n");
    printf(  "├────────┼────────────────────────────┤\n");
    for (int i = 0; i < state->instr_count; i++) {
        printf("│  %-4d  │  0x%04X                    │\n",
               i, (unsigned short)state->instr_mem[i]);
    }
    printf("└────────┴────────────────────────────┘\n");
}

void print_data_memory(ProcessorState *state) {
    printf("\n┌─────────────────────────────────────┐\n");
    printf(  "│            DATA MEMORY              │\n");
    printf(  "├──────────┬──────────────────────────┤\n");
    printf(  "│  Addr    │  Value                   │\n");
    printf(  "├──────────┼──────────────────────────┤\n");
    for (int i = 0; i < 2048; i++) {
        if (state->data_mem[i] != 0) { // only print non-zero
            printf("│  0x%04X  │  %-3u (0x%02X)             │\n",
                   i, state->data_mem[i], state->data_mem[i]);
        }
    }
    printf("└──────────┴──────────────────────────┘\n");
}