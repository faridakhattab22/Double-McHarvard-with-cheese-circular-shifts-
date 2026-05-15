#include "structures.h"
#include "processor.h"
#include "sreg.h"

/* C(carry) updated by add 
V(overflow) by add , sub 
N(negative) by add,sub,mul,and,or,sal,sar
S(sign) add w sub 
Z(zero) by add ,sub,mul,and,or,sal,sar
*/
//chcp 65001 | Out-Null ; .\e.exe




// Individual flag checks (used internally by updateSREG)
// carry bit 4 
int  compute_carry(int8_t val1, int8_t val2, Opcode op){
     if (op != ADD){
         return -1; // carry gets updated only by add 
     }
     // for negative numbers el mafrod en C maslan law -5 it stores it in 32 bits ya3ni beykamel el ba2i b ones fa 3shan keda lazem nestakhdem uint8_t
    int temp1 = (int8_t)val1;
    int temp2 = (int8_t)val2;
    int result = temp1 + temp2;

    // 0x100 is 256 in decimal aw 100000000 in binary 
    if ((result & 0x100) != 0) {
        return 1;
    }
    return 0;    
}

//result of operation is too small or too large tab3an to fit in the avalible bits
int compute_overflow(int8_t val1, int8_t val2, int8_t result, Opcode op){
    if (op != ADD && op != SUB) {
        return -1;
    }
    if (op == ADD){
        // same sign operands → overflow if result has opposite sign
        if (val1 > 0 && val2 > 0 && result < 0) return 1;
        if (val1 < 0 && val2 < 0 && result > 0) return 1;
        return 0;
    } 

    // SUB: only different signs can overflow
    // pos - neg → result should be more positive, if negative then overflow
    if (val1 > 0 && val2 < 0 && result < 0) return 1;
    // neg - pos → result should be more negative, if positive then overflow
    if (val1 < 0 && val2 > 0 && result > 0) return 1;
    return 0;
}


// flag N bit 2
int  compute_negative(int8_t result){
    if (result < 0) {
        return 1;
    }
    return 0;
}
//flag zero bit 0 
int  compute_zero(int8_t result){
    if (result == 0) {
        return 1;
    }
    return 0;
} 

// Sign flag S = N XOR V (computed inside updateSREG)

//>> right shift operator 
// & bitwise AND operator
int flag_C(uint8_t sreg) {
    return (sreg >> 4) & 1;  // Bit 4
}

int flag_V(uint8_t sreg) {
    return (sreg >> 3) & 1;  // Bit 3
}

int flag_N(uint8_t sreg) {
    return (sreg >> 2) & 1;  // Bit 2
}

int flag_S(uint8_t sreg) {
    return (sreg >> 1) & 1;  // Bit 1
}

int flag_Z(uint8_t sreg) {
    return (sreg >> 0) & 1;  // Bit 0
}
void updateSREG(ProcessorState *state, Opcode op, int8_t val1, int8_t val2, int8_t result){
    uint8_t new_sreg = state->sreg;   // start from current SREG — only relevant flags get updated

    // carry - bit 4 - gets updated ONLY by ADD
    // ~ flips all bits of FLAG_C_MASK
    // & (AND) with sreg forces only bit 4 to become 0, everything else unchanged
    // return 1 if carry happened so turn bit 4 ON
    int C = compute_carry(val1, val2, op);
    if (C != -1) {
        if (C) new_sreg |=  FLAG_C_MASK;   // set bit 4
        else   new_sreg &= ~FLAG_C_MASK;   // clear bit 4
    }

    // overflow - bit 3 - updated by ADD and SUB
    int V = compute_overflow(val1, val2, result, op);
    if (V != -1) {
        if (V == 1) {
            new_sreg = new_sreg | FLAG_V_MASK;
        } else {
            new_sreg = new_sreg & ~FLAG_V_MASK;
        }
    }

    // negative and zero: ADD, SUB, MUL, AND, OR, SAL, SAR
    if (op == ADD || op == SUB || op == MUL ||
        op == AND || op == OR  || op == SAL || op == SAR) {

        int N = compute_negative(result);
        if (N == 1) {
            new_sreg = new_sreg | FLAG_N_MASK;
        } else {
            new_sreg = new_sreg & ~FLAG_N_MASK;
        }

        int Z = compute_zero(result);
        if (Z == 1) {
            new_sreg = new_sreg | FLAG_Z_MASK;
        } else {
            new_sreg = new_sreg & ~FLAG_Z_MASK;
        }
    }

    // S = N XOR V (bit 1) — computed LAST because it depends on N and V being already updated
    // only for ADD and SUB
    if (op == ADD || op == SUB) {
        int final_n = flag_N(new_sreg);
        int final_v = flag_V(new_sreg);
        int s = final_n ^ final_v;   // XOR them
        if (s == 1) {
            new_sreg = new_sreg | FLAG_S_MASK;
        } else {
            new_sreg = new_sreg & ~FLAG_S_MASK;
        }
    }

    state->sreg = new_sreg & SREG_CLEAR_MASK; // write back fel proccesor — bits 7:5 always zero
}