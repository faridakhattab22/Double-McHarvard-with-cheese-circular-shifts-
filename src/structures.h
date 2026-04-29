#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "processor.h"
// added the imports 34an kanet 3amla error

// Pipeline registers between stages
typedef struct {
    short int instruction;   // raw 16-bit binary
    short int pc;            // PC of this instruction (stored for branch calc)
    int      valid;         // 0 = bubble/flushed
} IF_ID_Reg;

typedef struct {
    Opcode           opcode;
    InstructionFormat format;
    int              r1;        // destination / source reg index
    int              r2;        // source reg index (R-format)
    int8_t           imm;       // sign-extended 6-bit immediate
    int8_t           val_r1;    // value read from register file
    int8_t           val_r2;
    short int         pc;        // saved PC for branch target calc
    int              valid;
} ID_EX_Reg;

typedef struct {
    int      dest_reg;      // register to write back (-1 = no writeback) rakamo ya farah focussss
    int8_t   result;        // ALU or memory result
    short int new_pc;        // updated PC if branch/jump taken
    FlushType flush;
    int       valid;
} EX_Result;

// Full processor state
typedef struct {
    int8_t   regs[64];          // R0–R63, 8-bit general purpose
    short int pc;                // 16-bit program counter
    uint8_t  sreg;              // bits: 0=Z,1=S,2=N,3=V,4=C (bits7:5 = 0)
    short int  instr_mem[1024];   // 16-bit words stored as byte pairs (kanet ma7tota 8 8ayrtha l 16)
    uint8_t  data_mem[2048];    // 8-bit per address
    int      instr_count;       // number of instructions loaded (it was stated to be 12 ??? but at the same time depends on the program)
    int      clock_cycle;
    IF_ID_Reg  if_id; // inst. fetch
    ID_EX_Reg  id_ex; // inst. Decode
    EX_Result  ex_out; // exec. result
} ProcessorState;

#endif 