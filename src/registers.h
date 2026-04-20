// registers.h
#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include <stdio.h>
#include "processor.h"



// --- 
// Register file ---
int8_t  readReg(ProcessorState *state, int reg_index);
void    writeReg(ProcessorState *state, int reg_index, int8_t value);
// (no-op silently if reg_index out of range)

// --- Data memory ---
uint8_t readMem(ProcessorState *state, uint16_t address);
void    writeMem(ProcessorState *state, uint16_t address, uint8_t value);

// --- PC ---
uint16_t getPC(ProcessorState *state);
void     setPC(ProcessorState *state, uint16_t value);
void     incrementPC(ProcessorState *state);   // PC++

// --- Init ---
void    init_registers(ProcessorState *state);  // zero all regs, PC=0
void    init_data_memory(ProcessorState *state); // zero all data mem

#endif