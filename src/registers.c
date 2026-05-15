#include "structures.h"
#include "registers.h"
#include "output.h"



int8_t  readReg (ProcessorState *state, int idx){
    if (idx < 0 || idx > 63){
     return 0; // invalid index checking 
    }
    else {
        return state->regs[idx]; //simple ya3ny

    }
}

void    writeReg(ProcessorState *state, int idx, int8_t value){ // hns2al tany 
    if (idx < 0 || idx > 63 ||idx == 0 ){
     return ; // Nafs el7aga tany bas hnzawed el 0 34an mayenfa34 a overwrite
    }
    else {
        state->regs[idx] = value;
    }

}


/* --- Data memory (byte-addressable, 2048 × 8-bit) --- */
uint8_t read_data_Mem (ProcessorState *state, short int addr){
     if (addr < 0 ){
     return 0; // invalid address checking 
    }
     if ( addr > 2047){
        fprintf(stderr, "[WARN] Data memory read out of range: %u\n", addr);
     return 0; // invalid address checking 
    }
    else {
        return state->data_mem[addr]; //simple ya3ny

    }

}
/* --- Instruction memory (byte-addressable, 1024 × 16-bit) --- */
short int  read_inst_Mem (ProcessorState *state, short int addr){ // mmkn t7tagoha f fetching el instruction
     if (addr < 0 ){
     return 0; // invalid address checking 
    }
     if ( addr > 1023){
        fprintf(stderr, "[WARN] Instruction memory read out of range: %u\n", addr);
     return 0; // invalid address checking 
    }
    else {
        return state->instr_mem[addr]; //simple ya3ny

    }

}
/* --- Data memory (byte-addressable, 2048 × 8-bit) --- */
// void    write_data_Mem(ProcessorState *state, short int addr, uint8_t value){
//     if (addr < 0 ){
//      return ; // invalid address checking 
//     }
//     if (addr > 2047) {
//         fprintf(stderr, "[WARN] Data memory write out of range: %u\n", addr);
//         return;
//     }
//     else{
//     state->data_mem[addr] = value;

//     }

// }
void write_data_Mem(ProcessorState *state, short int addr, uint8_t value){

    if (addr < 0 ){
        return;
    }

    if (addr > 2047) {
        fprintf(stderr, "[WARN] Data memory write out of range: %u\n", addr);
        return;
    }
    state->data_mem[addr] = value;

    if (state->data_mem[addr] != value) {
        log_memory_change(addr, value, state->clock_cycle);
    }

}
/* --- Instruction memory (byte-addressable, 1024 × 16-bit) --- */
void    write_inst_Mem(ProcessorState *state, short int addr, short int value){ // mmkn t7tagoha f el parsing f elawel
    if (addr < 0 ){
     return ; // invalid address checking 
    }
    if (addr > 1023) {
        fprintf(stderr, "[WARN] Instruction memory write out of range: %u\n", addr);
        return;
    }
    else{
    state->instr_mem[addr] = value;

    }

}

/* --- PC helpers --- */
short int getPC      (ProcessorState *state){
    return state->pc;
}

void     setPC      (ProcessorState *state, short int value){
    state->pc = value;
}

void     incrementPC(ProcessorState *state){
    state->pc++;
}

/* --- Initialisation --- */
void init_registers  (ProcessorState *state){
    memset(state->regs,0,sizeof(state->regs));
    state->pc=0;
    state->sreg=0;
    
}

void init_data_memory(ProcessorState *state){
     memset(state->data_mem, 0, sizeof(state->data_mem));
}

void init_inst_memory(ProcessorState *state){ // 3mltha belmara
     memset(state->instr_mem, 0, sizeof(state->instr_mem));
}