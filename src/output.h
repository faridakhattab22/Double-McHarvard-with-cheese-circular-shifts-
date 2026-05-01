
// output.h
#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>
#include <stdio.h>
#include "processor.h"
#include "registers.h"
#include "sreg.h"

// Per-cycle printing
void print_cycle_header(int cycle);
void print_stage_IF(ProcessorState *state);
void print_stage_ID(ProcessorState *state);
void print_stage_EX(ProcessorState *state);

// Change logging (called by M2 after any write)
void log_register_change(int reg_index, int8_t new_value, int cycle);
void log_memory_change(short int address, uint8_t new_value, int cycle);

// Final state dump
void print_all_registers(ProcessorState *state);
void print_instruction_memory(ProcessorState *state);
void print_data_memory(ProcessorState *state);
void print_sreg(ProcessorState *state);
#endif