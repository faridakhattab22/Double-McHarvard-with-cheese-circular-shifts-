#include "structures.h"
#include "processor.h"
#include "sreg.h"

/* C(carry) updated by add 
V(overflow) by add , sub 
N(negative) by kolo  add,sub,mul,and,or,sal,sar
S(sign) add w sub 
Z(zero) by kolo  add ,sub,mul,and,or,sal,sar
*/



//void updateSREG(ProcessorState *state, Opcode op,
//                int8_t val1, int8_t val2, int8_t result);

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

    // el 8 bits le7ad 255 in decimal so  
    if (result > 255) { //check el condition beta3et el hexa number tani keda
    return 1;     //SHOFI DI TANI  
}
return 0;

}

//result of operation is too small or too large tab3an to fit in the avalible bits
int  compute_overflow(int8_t val1, int8_t val2, int8_t result, Opcode op){
    if (op != ADD && op != SUB) {
        return -1;
    }

    // for cases like 100+100=200  and -100+-100=-200 w el range of 8 bit is -128 to 128 
    if (val1 > 0 && val2 > 0 && result < 0){
        return 1 ;
    }
     if (val1 < 0 && val2 < 0 && result > 0) {
            return 1;
        }
        return 0;

        /* Overflow for subtraction (val1 - val2) is a bit different. The spec says: overflow occurs when the signs of the two operands are different AND the result has the same sign as val2 (the subtrahend — the one being subtracted).
        Case 1: val1 is positive, val2 is negative, result is negative. Subtracting a negative is like adding a positive — so a positive minus a negative should give something even more positive. If instead we got a negative, the result wrapped around — overflow.
        Case 2: val1 is negative, val2 is positive, result is positive. A negative minus a positive should give something even more negative. If instead we got a positive, it wrapped around — overflow.
        If the signs of val1 and val2 are the same, subtraction can never overflow, so we return*/

        if (val1 > 0 && val2 < 0 && result < 0) {
        return 1;
    }
    if (val1 < 0 && val2 > 0 && result > 0) {
        return 1;
    }
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
