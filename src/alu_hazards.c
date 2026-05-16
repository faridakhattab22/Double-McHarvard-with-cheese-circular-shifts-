#include <stdint.h>
#include <stdio.h>
#include "structures.h"
#include "alu_hazards.h"
#include "registers.h"
#include "sreg.h"
#include "flush.h"
#include "pipeline_if_id.h" 

// ALU operations 
int8_t alu_add(int8_t a, int8_t b){return (int8_t)(a+b);}
int8_t alu_sub(int8_t a, int8_t b){return (int8_t)(a-b);}
int8_t alu_mul(int8_t a, int8_t b){return (int8_t)(a*b);}
int8_t alu_and(int8_t a, int8_t b){return (int8_t)(a&b);}
int8_t alu_or (int8_t a, int8_t b){return (int8_t)(a|b);} 
int8_t alu_sal(int8_t a, int8_t imm){return (int8_t)(a<<imm);}  
int8_t alu_sar(int8_t a, int8_t imm){return (int8_t)(a>>imm);}  

// detect data hazard against previous EX result: if dest_reg matches r1 or r2 in ID/EX, and EX result is valid, then we have a hazard.

HazardType detect_hazard(ProcessorState *state) {
    if (!state->ex_out.valid || !state->id_ex.valid)
        return HAZARD_NONE;

    int dest = state->ex_out.dest_reg;

    if (dest <= 0)
        return HAZARD_NONE;

    int r1  = state->id_ex.r1;
    int r2  = state->id_ex.r2;
    int fmt = state->id_ex.format;

    int r1_hazard = (r1 == dest);
    int r2_hazard = (fmt == FORMAT_R) && (r2 == dest);

    if (!r1_hazard && !r2_hazard)
        return HAZARD_NONE;

    // Only stall for load-use hazard
    if (state->ex_out.opcode == LB)
        return HAZARD_STALL;

    return HAZARD_NONE;
}

void insert_stall(ProcessorState *state) {
    printf("  [HZRD, STALL] bubbling ID_EX, freezing IF/ID\n");

    // Insert bubble into ID/EX: marks it invalid so EX does nothing
    insert_bubble_id_ex(state); // 3nd hana fel pipeline_IF_ID

    state->ex_out.dest_reg = -1;
    state->ex_out.result   = 0;
    state->ex_out.valid    = 0;

    // "freezing the pipeline" 
    // Undo the PC increment from the last fetch so that IF will
    // re-fetch the same instruction next cycle ()
    if (state->pc > 0)
        state->pc--;
}

// 1. DETECT DATA HAZARDS
//    If HAZARD_STALL,  insert_stall and skip EX
//  (set valid=0, no writeback, no mem access)
// 3. SWITCH CASE
// A. ALU operation [AND , OR , ADD, SUB, MUL, SAL, SAR]
//   - compute ALU result based on opcode in ID/EX and inputs
//   - SREG update: call updateSREG with appropriate operands and result to set flags for next cycle
//   - write back to registers needed
// B. Memory access (LB / SB)
//   - For LB: compute effective address, read from data memory, write back to register
//   - For SB: compute effective address, write to data memory
// C. Branch/jump resolution
//   - For BEQZ: evaluate branch condition (val_r1 == 0), compute_beqz_target
//   - For JR: compute_jr_target
//   - use evaluate_branch to determine if branch/jump is taken and get flush type,
//   - if taken, call flush_pipeline to invalidate IF/ID and ID/EX, set PC to target (NEXT CYCLE PREPARATION)
// 4. Store EX result in pipeline register for forwarding to next cycle (EX_Result struct) - this will be used by the next cycle's hazard detection and forwarding logic. (NEXT CYCLE PREPARATION)

void stage_EX(ProcessorState *state){
    state->ex_out.flush = FLUSH_NONE; 
    HazardType hazard = detect_hazard(state);
    
    if(hazard == HAZARD_STALL){
        insert_stall(state);
        return;
    }
    
    if (!state->id_ex.valid){
        state->ex_out.valid    = 0;
        state->ex_out.dest_reg = -1;
        return;
    }

    state->id_ex.val_r1 = readReg(state, state->id_ex.r1);
    state->id_ex.val_r2 = readReg(state, state->id_ex.r2);
    state->ex_out.opcode = state->id_ex.opcode;

    switch(state->id_ex.opcode){
        case ADD:{         
            int result = alu_add(state->id_ex.val_r1,state->id_ex.val_r2);
            updateSREG(state, state->id_ex.opcode, state->id_ex.val_r1, state->id_ex.val_r2, result);
            writeReg(state, state->id_ex.r1, result);
            state->ex_out.dest_reg = state->id_ex.r1;
            state->ex_out.result   = (int8_t)result;
            state->ex_out.valid    = 1;
            break;
        }
        case SUB: {         
            int result = alu_sub(state->id_ex.val_r1,state->id_ex.val_r2);
            updateSREG(state, state->id_ex.opcode, state->id_ex.val_r1, state->id_ex.val_r2, result);
            writeReg(state, state->id_ex.r1, result);
            state->ex_out.dest_reg = state->id_ex.r1;
            state->ex_out.result   = (int8_t)result;
            state->ex_out.valid    = 1;
            break;
        }
        case MUL: {         
            int result = alu_mul(state->id_ex.val_r1,state->id_ex.val_r2);
            updateSREG(state, state->id_ex.opcode, state->id_ex.val_r1, state->id_ex.val_r2, result);
            writeReg(state, state->id_ex.r1, result);
            state->ex_out.dest_reg = state->id_ex.r1;
            state->ex_out.result   = (int8_t)result;
            state->ex_out.valid    = 1;
            break;
        }
        
        case AND: {         
            int result = alu_and(state->id_ex.val_r1,state->id_ex.val_r2);
            updateSREG(state, state->id_ex.opcode, state->id_ex.val_r1, state->id_ex.val_r2, result);
            writeReg(state, state->id_ex.r1, result);
            state->ex_out.dest_reg = state->id_ex.r1;
            state->ex_out.result   = (int8_t)result;
            state->ex_out.valid    = 1;
            break;
        }
        case OR:  {         
            int result = alu_or(state->id_ex.val_r1,state->id_ex.val_r2);
            updateSREG(state, state->id_ex.opcode, state->id_ex.val_r1, state->id_ex.val_r2, result);
            writeReg(state, state->id_ex.r1, result);
            state->ex_out.dest_reg = state->id_ex.r1;
            state->ex_out.result   = (int8_t)result;
            state->ex_out.valid    = 1;
            break;
        }

        case SAL:  {         
            int result = alu_sal(state->id_ex.val_r1,state->id_ex.imm);
            updateSREG(state, state->id_ex.opcode, state->id_ex.val_r1, state->id_ex.val_r2, result);
            writeReg(state, state->id_ex.r1, result);
            state->ex_out.dest_reg = state->id_ex.r1;
            state->ex_out.result   = (int8_t)result;
            state->ex_out.valid    = 1;
            break;
        }
        case SAR:  {         
            int result = alu_sar(state->id_ex.val_r1,state->id_ex.imm);
            updateSREG(state, state->id_ex.opcode, state->id_ex.val_r1, state->id_ex.val_r2, result);
            writeReg(state, state->id_ex.r1, result);
            state->ex_out.dest_reg = state->id_ex.r1;
            state->ex_out.result   = (int8_t)result;
            state->ex_out.valid    = 1;
            break;
        }

        case LDI:{
            writeReg(state, state->id_ex.r1, state->id_ex.imm);
            
            state->ex_out.dest_reg = state->id_ex.r1;
            state->ex_out.result   = state->id_ex.imm;
            state->ex_out.valid    = 1;
            break;
        }

        case LB: {
            short int addr = (short int) (state->id_ex.imm);
            int8_t loaded  = (int8_t)read_data_Mem(state, addr);

            writeReg(state, state->id_ex.r1, loaded);

            state->ex_out.dest_reg = state->id_ex.r1;
            state->ex_out.result   = loaded;
            state->ex_out.valid    = 1;
            break;
        }

        case SB:{
            short int addr  = (short int)(state->id_ex.imm);
            uint8_t   value = (uint8_t)state->id_ex.val_r1;

            write_data_Mem(state, addr, value);

            state->ex_out.dest_reg = -1;
            state->ex_out.valid    = 1;
            break;
        }

        case BEQZ: {
            short int target = compute_beqz_target(state->id_ex.pc, state->id_ex.imm);

            // printf("DEBUG BRANCH (BEQZ): pc=%d imm=%d target=%d r1_val=%d\n",
            //        state->id_ex.pc,
            //        state->id_ex.imm,
            //        target,
            //        state->id_ex.val_r1);

            if (state->id_ex.val_r1 == 0) {
                flush_pipeline(state, target);
                state->ex_out.flush    = FLUSH_TAKEN;
                state->ex_out.new_pc   = target;
            } else {
                state->ex_out.flush  = FLUSH_NONE;
                state->ex_out.new_pc = 0;
            }

            state->ex_out.dest_reg = -1;
            state->ex_out.valid    = 1;
            break;
        }

        case JR: {
            short int target = compute_jr_target(state->id_ex.val_r1, state->id_ex.val_r2);

           
            // printf("DEBUG BRANCH (JR): r1=%d r2=%d target=%d\n",
            //        state->id_ex.val_r1,
            //        state->id_ex.val_r2,
            //        target);

            flush_pipeline(state, target);

            state->ex_out.flush    = FLUSH_TAKEN;
            state->ex_out.new_pc   = target;
            state->ex_out.dest_reg = -1;
            state->ex_out.valid    = 1;
            break;
        }
    }
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
