#include <stdint.h>
#include <stdio.h>
#include "structures.h"
#include "alu_hazards.h"
#include "registers.h"
#include "sreg.h"
#include "flush.h"
#include "pipeline_if_id.h" 
// ALU operations 

// arithmetic operations
int8_t alu_add(int8_t a, int8_t b){return (int8_t)(a+b);}
int8_t alu_sub(int8_t a, int8_t b){return (int8_t)(a-b);}
int8_t alu_mul(int8_t a, int8_t b){return (int8_t)(a*b);}

// logical operations
int8_t alu_and(int8_t a, int8_t b){return (int8_t)(a&b);}
int8_t alu_or (int8_t a, int8_t b){return (int8_t)(a|b);}

// arithmetic shifts 
int8_t alu_sal(int8_t a, int8_t imm){return (int8_t)(a<<imm);}  
int8_t alu_sar(int8_t a, int8_t imm){return (int8_t)(a>>imm);}  

// hazard detection and forwarding should be done in decoding stage,
// wala detect_hazard at the beginning of EX stage??
// keda keda before we can execute the ALU op,

// DATA HAZARD DETECTION : 
// If returns HAZARD_FORWARD_EX, then call apply_forwarding to patch the inputs before executing ALU op.
// If returns HAZARD_STALL, then we will call insert_stall and skip the rest of EX for this cycle (keep EX result as bubble). 
// If no hazard or forwarding applied, proceed with normal ALU execution and SREG update.

// 1. DETECT DATA HAZARDS FUNCTION
    // detect data hazard (RAW) against previous EX result: if dest_reg matches r1 or r2 in ID/EX, and EX result is valid, then we have a hazard.
    // If hazard detected, check if forwarding can resolve it: if EX dest_reg matches ID/
    // EX r1 or r2, we can forward the EX result to the ALU inputs. If forwarding applied, no stall needed. 
    // If hazard detected but cannot forward (e.g. load-use), then we must insert a stall
// 2. apply solution: Forwarding OR stalling (apply_forwarding and insert_stall functions)
//    - If forwarding: patch val_r1/val_r2 in ID/EX with EX result before ALU executes.
//    - If stalling: freeze IF/ID (keep same instruction in decode), insert bubble into ID/EX (set valid=0, no writeback, no mem access)

// Hazard detection: inspect ID/EX vs previous EX result.
// should be in the beginning of ex or end of decoding
HazardType detect_hazard(ProcessorState *state) {
    // Need a valid previous EX result AND a valid incoming instruction
    if (!state->ex_out.valid || !state->id_ex.valid)
        return HAZARD_NONE;

    // old value of previoys ex reg 
    int dest = state->ex_out.dest_reg;

    // NONE CASES
    // dest == -1 means previous instruction had no writeback (SB/BEQZ/JR).
    // dest == 0 is R0
    if (dest <= 0)
        return HAZARD_NONE;

    int r1  = state->id_ex.r1;
    int r2  = state->id_ex.r2;
    int fmt = state->id_ex.format; // format I or R

    // Check if any source register of the current instruction
    // depends on the destination of the previous instruction. */
    
    int r1_hazard = (r1 == dest);
    int r2_hazard = (fmt == FORMAT_R) && (r2 == dest);

    if (!r1_hazard && !r2_hazard)
        return HAZARD_NONE;

// Decide forward or stall
// mi4 3arfa ana b3mel eh bs stall for LD
// If the previous instruction wrote to a register that the current instruction reads, we have a RAW hazard.
    if(state->id_ex.opcode == LB) {
        // load-use hazard: if current instruction is a load, we cannot forward from EX because the data won't be ready until the end of EX stage. We must stall.
        return HAZARD_STALL;
    }else if (state->ex_out.dest_reg != -1 && // previous wrote a reg
        state->id_ex.opcode != LB)            // current is not a load itself
    {
        return HAZARD_FORWARD_EX;
    }

    // forward for all other cases
    return HAZARD_FORWARD_EX;
}


// Apply forwarding: patch val_r1/val_r2 in ID/EX before ALU executes.
void apply_forwarding(ProcessorState *state) {
    if (!state->ex_out.valid)
        return;

    // get the previous EX destination and result for forwarding 
    int    dest = state->ex_out.dest_reg;
    int8_t val  = state->ex_out.result;

    if (dest <= 0) // r0 is unchangable in pack 4 soo  nothing meaningful to forward
        return; 

    // does R1 need patching? current value in ID_EX is the same as the destination of the previous EX, 
    // so we can forward the result to val_r1
    if (state->id_ex.r1 == dest) {
        printf("  [HZRD, FWD] EX->EX: R%d=%d forwarded to val_r1\n", dest, (int)val);
        state->id_ex.val_r1 = val;
    }

    // does R2 need patching? current value in ID_EX is the same as the destination of the previous EX, 
    // so we can forward the result to val_r2
    // only for R-format anyway
    if (state->id_ex.format == FORMAT_R && state->id_ex.r2 == dest) {
        printf("  [HZRD, FWD] EX->EX: R%d=%d forwarded to val_r2\n", dest, (int)val);
        state->id_ex.val_r2 = val;
    }
}

// Insert stall cycle: freeze IF/ID, insert bubble into ID/EX.
void insert_stall(ProcessorState *state) {
    printf("  [HZRD, STALL] bubbling ID_EX, freezing IF/ID\n");

    // Insert bubble into ID/EX: marks it invalid so EX does nothing
    insert_bubble_id_ex(state); // 3nd hana fel pipeline_IF_ID

    // "freezing the pipeline" 
    // Undo the PC increment from the last fetch so that IF will
    // re-fetch the same instruction next cycle ()
    if (state->pc > 0)
        state->pc--;
}

// STAGE EX ITSELF stage_EX FUNCTION
// 3. IF CONDITION 
// A. ALU operation [AND , OR , ADD, SUB, MUL, SAL, SAR]
//   - compute ALU result based on opcode in ID/EX and inputs (after forwarding if applied)
//   - write back to registers needed, variable regwrite = 1 
//   - SREG update: call updateSREG with appropriate operands and result to set flags for next cycle
//   - ACTUALLY Writeback to register if needed (regwrite = 1)
// B. Memory access (LB / SB)
//   - For LB: compute effective address, read from data memory, write back to register, set regwrite=1, memwrite=0
//   - For SB: compute effective address, write to data memory, set regwrite=0, memwrite=1
// C. Branch/jump resolution
//   - For BEQZ: evaluate branch condition (val_r1 == 0), compute_beqz_target
//   - For JR: compute_jr_target
//   - use evaluate_branch to determine if branch/jump is taken and get flush type,
//   - if taken, call flush_pipeline to invalidate IF/ID and ID/EX, set PC to target (NEXT CYCLE PREPARATION)
// 4. Store EX result in pipeline register for forwarding to next cycle (EX_Result struct) - this will be used by the next cycle's hazard detection and forwarding logic. (NEXT CYCLE PREPARATION)

void stage_EX(ProcessorState *state){
    
}


////////////////////////////////////////////////////////
///////////////////////////////////////////////////////
// testing
// #include <assert.h>

// void main(){
//     // -------- ADD --------
//     assert(alu_add(10, 20) == 30);
//     assert(alu_add(100, 30) == -126);   // overflow
//     assert(alu_add(-50, -50) == -100);
//     assert(alu_add(-128, -1) == 127);   // wrap

//     // -------- SUB --------
//     assert(alu_sub(50, 20) == 30);
//     assert(alu_sub(-50, 50) == -100);
//     assert(alu_sub(-128, 1) == 127);    // overflow
//     assert(alu_sub(0, 1) == -1);

//     // -------- MUL --------
//     assert(alu_mul(5, 5) == 25);
//     assert(alu_mul(20, 10) == -56);     // overflow
//     assert(alu_mul(-10, 10) == -100);
//     assert(alu_mul(-128, 2) == 0);      // overflow wrap

//     // -------- AND --------
//     assert(alu_and(0b10101010, 0b11001100) == (int8_t)0b10001000);
//     assert(alu_and(-1, 0b01010101) == 0b01010101);

//     // -------- OR --------
//     assert(alu_or(0b10100000, 0b00001111) == (int8_t)0b10101111);
//     assert(alu_or(0, -1) == -1);

//     // -------- SAL (<<) --------
//     assert(alu_sal(1, 3) == 8);
//     assert(alu_sal(16, 2) == 64);
//     assert(alu_sal(64, 2) == 0);        // overflow
//     assert(alu_sal(-2, 1) == -4);

//     // -------- SAR (>>) --------
//     assert(alu_sar(8, 2) == 2);
//     assert(alu_sar(-8, 2) == -2);       // arithmetic shift (sign preserved)
//     assert(alu_sar(-1, 1) == -1);       // stays -1


//     for (int i = 0; i < 100000; i++) {
//         int8_t a = rand();
//         int8_t b = rand();
//         assert(alu_add(a,b) == (int8_t)(a+b));
//     }
//     for (int i = 0; i < 100000; i++) {
//         int8_t a = rand();
//         int8_t b = rand();
//         assert(alu_and(a, b) == (int8_t)(a & b));
//     }

//     // SHIFT LEFT (SAL)
//     for (int i = 0; i < 100000; i++) {
//         int8_t a = rand();
//         int8_t imm = rand() % 8;  // IMPORTANT: limit shift to 0–7
//         assert(alu_sal(a, imm) == (int8_t)(a << imm));
//     }

//     // SHIFT RIGHT (SAR)
//     for (int i = 0; i < 100000; i++) {
//         int8_t a = rand();
//         int8_t imm = rand() % 8;  // avoid UB
//         assert(alu_sar(a, imm) == (int8_t)(a >> imm));
//     }

//     printf("Random tests passed!\n");
// }
