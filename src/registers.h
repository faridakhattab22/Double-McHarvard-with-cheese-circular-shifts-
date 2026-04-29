// registers.h
#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include <stdio.h>
#include "processor.h"
#include "structures.h"


// --- 
// Register file ---
int8_t  readReg(ProcessorState *state, int reg_index);
void    writeReg(ProcessorState *state, int reg_index, int8_t value);
// (no-op silently if reg_index out of range)

// --- memory ---
uint8_t read_data_Mem (ProcessorState *state, short int addr);
short int read_inst_Mem (ProcessorState *state, short int addr);// mmkn t7tagoha f fetching el instruction
void    write_data_Mem(ProcessorState *state, short int addr, uint8_t value);
void    write_inst_Mem(ProcessorState *state, short int addr, short int value);// mmkn t7tagoha f el parsing f elawel

// --- PC ---
short int getPC(ProcessorState *state);
void     setPC(ProcessorState *state, short int value);
void     incrementPC(ProcessorState *state);   // PC++

// --- Init ---
void    init_registers(ProcessorState *state);  // zero all regs, PC=0
void    init_data_memory(ProcessorState *state); // zero all data mem
void    init_inst_memory(ProcessorState *state); // zero all data mem

#endif