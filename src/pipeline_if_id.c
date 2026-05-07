#include "pipeline_if_id.h"
#include "registers.h" 
#include "processor.h"


// void stage_IF(ProcessorState *state) {
//     // Look at the processor's current ticket number
//     short int current_pc = state->pc; 
//         printf("DEBUG IF: pc=%d instr_count=%d\n", current_pc, state->instr_count);


//     // Check if we have processed all the instructions M1 loaded
//     if (current_pc >= state->instr_count) {
//         // If we are out of instructions, we insert a "bubble" (a fake, empty instruction)
//         state->if_id.valid = 0; 
//         return; // Stop executing this function
//     }

//     // Our instruction is 16 bits, but M7 made the memory array 8 bits per slot.
//     // We multiply PC by 2 to find our starting slot, then grab two slots.
//     uint8_t byte1 = state->instr_mem[current_pc * 2];       // Top 8 bits
//     uint8_t byte2 = state->instr_mem[(current_pc * 2) + 1]; // Bottom 8 bits

//     // We shift the top 8 bits to the left by 8 spaces, and OR (|) them with the bottom 8 bits.
//     // This glues them together into a single 16-bit number.
//     state->if_id.instruction = (byte1 << 8) | byte2; 

//     // Save the PC so the Execute stage knows where it is if it needs to branch
//     state->if_id.pc = current_pc; 
    
//     // Tell the Decode stage that this is a real instruction, not a bubble
//     state->if_id.valid = 1; 

//     // Increment the PC so the next clock cycle grabs the next instruction
//     state->pc = current_pc + 1; 
// }

void stage_IF(ProcessorState *state) {
    short int current_pc = state->pc;

    // printf("DEBUG IF: pc=%d instr_count=%d\n", current_pc, state->instr_count);

    if (current_pc >= state->instr_count|| state->ex_out.flush==FLUSH_TAKEN) {
        state->if_id.valid = 0 ; 
        return;
    }

    // ✅ FIX: directly read full 16-bit instruction
    state->if_id.instruction = state->instr_mem[current_pc];

    state->if_id.pc = current_pc;
    state->if_id.valid = 1;

    state->pc = current_pc + 1;
}
void stage_ID(ProcessorState *state) {
    // Check if the Fetch stage gave us a bubble (empty instruction)
    // This happens if we ran out of instructions or if M6 flushed the pipeline
    if (state->if_id.valid == 0) {
        state->id_ex.valid = 0; // Pass a bubble down to Execute
        return;                 // Stop executing this function
    }

    // Call our butcher block helper. 
    // We pass addresses (&) of our id_ex struct variables so the helper can update them directly.
    decode_instruction(
        state->if_id.instruction, 
        &state->id_ex.opcode, 
        &state->id_ex.format, 
        &state->id_ex.r1, 
        &state->id_ex.r2, 
        &state->id_ex.imm
    );

    // Now we know which registers the instruction wants (r1 and r2).
    // We call M2's readReg function to get the actual numbers stored inside them.
    state->id_ex.val_r1 = readReg(state, state->id_ex.r1); 
    state->id_ex.val_r2 = readReg(state, state->id_ex.r2); // Read it even if it's I-format, just in case.

    // Pass the PC down the assembly line
    state->id_ex.pc = state->if_id.pc; 

    // Tell the Execute stage that this is a real instruction ready to be calculated
    state->id_ex.valid = 1; 
}
void decode_instruction(short int raw, Opcode *opcode, InstructionFormat *fmt, int *r1, int *r2, int8_t *imm) {
    
    // Shift right 12 spaces to push the top 4 bits to the bottom.
    // Mask with 0x000F (0000 0000 0000 1111) to isolate just those 4 bits.
    *opcode = (raw >> 12) & 0x000F;
    
    // Shift right 6 spaces to push the middle 6 bits to the bottom.
    // Mask with 0x003F (0000 0000 0011 1111) to isolate those 6 bits.
    *r1 = (raw >> 6) & 0x003F;
    
    // No shifting needed for the last 6 bits. Just mask out the top 10 bits.
    *r2 = raw & 0x003F;
    
    // Get those same last 6 bits, but pass them through our sign-extension helper.
    uint8_t raw_imm = raw & 0x003F;
    *imm = sign_extend_6(raw_imm);
    
    // Check the opcode to figure out if this instruction uses R2 (Format R) or IMM (Format I).
    // According to Package 4: LDI (3), BEQZ (4), SAL (8), SAR (9), LB (10), SB (11) are all I-Format.
    if (*opcode == LDI || *opcode == BEQZ || *opcode >= SAL) {
        *fmt = FORMAT_I; 
    } else {
        *fmt = FORMAT_R;
    }
}
int8_t sign_extend_6(uint8_t imm6) {
    // 0x20 is binary 00100000. This isolates the 6th bit (the sign bit).
    // We check: is the 6th bit a 1?
    if ((imm6 & 0x20) == 0x20) {
        
        // If yes, it's negative. 
        // 0xC0 is binary 11000000. 
        // We use bitwise OR (|) to forcefully turn the 7th and 8th bits into 1s.
        return (int8_t)(imm6 | 0xC0); 
        
    } else {
        // If the 6th bit is a 0, it's a positive number. 
        // No padding needed, just pass it through.
        return (int8_t)imm6;
    }
}


void insert_bubble_if_id(ProcessorState *state) {
    state->if_id.instruction = 0;
    state->if_id.pc          = 0;
    state->if_id.valid       = 0;
}

void insert_bubble_id_ex(ProcessorState *state) {
    state->id_ex.opcode  = 0;
    state->id_ex.format  = 0;
    state->id_ex.r1      = 0;
    state->id_ex.r2      = 0;
    state->id_ex.imm     = 0;
    state->id_ex.val_r1  = 0;
    state->id_ex.val_r2  = 0;
    state->id_ex.pc      = 0;
    state->id_ex.valid   = 0;  // EX checks this
}