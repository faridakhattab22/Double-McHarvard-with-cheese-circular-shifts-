/*
 * test_sreg.c  —  Assert-based unit tests for every SREG function.
 *
 * Compile & run (from src/ directory):
 *   gcc -Wall -Wextra -o test_sreg test_sreg.c sreg.c -I.
 *   .\test_sreg.exe
 *
 * Key behaviour of the logic-based updateSREG:
 *   C        — updated ONLY by ADD  (persists for all others)
 *   V        — updated ONLY by ADD/SUB  (persists for all others)
 *   N, Z     — updated by ADD/SUB/MUL/AND/OR/SAL/SAR  (persist for LDI/LB/SB/BEQZ/JR)
 *   S        — updated ONLY by ADD/SUB as N XOR V  (persists for all others)
 *   bits 7:5 — always cleared by SREG_CLEAR_MASK on write-back
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "structures.h"
#include "processor.h"
#include "sreg.h"

/* ------------------------------------------------------------------ */
static ProcessorState make_state(uint8_t initial_sreg)
{
    ProcessorState s;
    int i;
    for (i = 0; i < 64;   i++) s.regs[i]     = 0;
    for (i = 0; i < 1024; i++) s.instr_mem[i] = 0;
    for (i = 0; i < 2048; i++) s.data_mem[i]  = 0;
    s.pc = 0; s.sreg = initial_sreg; s.instr_count = 0; s.clock_cycle = 0;
    return s;
}

/* ================================================================== */
/*  1. flag_C/V/N/S/Z  —  bit extraction                              */
/* ================================================================== */
static void test_flag_readers(void)
{
    puts("  [flag readers]");

    assert(flag_C(0x00)==0); assert(flag_V(0x00)==0);
    assert(flag_N(0x00)==0); assert(flag_S(0x00)==0); assert(flag_Z(0x00)==0);

    /* all five bits set: 0b00011111 = 0x1F */
    assert(flag_C(0x1F)==1); assert(flag_V(0x1F)==1);
    assert(flag_N(0x1F)==1); assert(flag_S(0x1F)==1); assert(flag_Z(0x1F)==1);

    /* only C  (bit 4) */
    assert(flag_C(0x10)==1); assert(flag_V(0x10)==0);
    assert(flag_N(0x10)==0); assert(flag_S(0x10)==0); assert(flag_Z(0x10)==0);

    /* only Z  (bit 0) */
    assert(flag_C(0x01)==0); assert(flag_Z(0x01)==1);

    /* only N  (bit 2) */
    assert(flag_N(0x04)==1); assert(flag_C(0x04)==0);
    assert(flag_V(0x04)==0); assert(flag_Z(0x04)==0);

    /* bits 7:5 must never bleed into flag reads */
    assert(flag_C(0xE0)==0); assert(flag_Z(0xE0)==0);

    puts("    PASSED");
}

/* ================================================================== */
/*  2. compute_carry                                                    */
/* ================================================================== */
static void test_compute_carry(void)
{
    puts("  [compute_carry]");

    /* non-ADD ops → -1 */
    assert(compute_carry(100,100,SUB)==-1);
    assert(compute_carry(100,100,MUL)==-1);
    assert(compute_carry(100,100,AND)==-1);
    assert(compute_carry(100,100,OR) ==-1);
    assert(compute_carry(100,100,SAL)==-1);
    assert(compute_carry(100,100,SAR)==-1);
    assert(compute_carry(100,100,LDI)==-1);

    /* ADD: no carry (signed ints sign-extend, so sum never > 255) */
    assert(compute_carry(10,  20,  ADD)==0);
    assert(compute_carry(0,   0,   ADD)==0);
    assert(compute_carry(127, 0,   ADD)==0);
    assert(compute_carry(127, 127, ADD)==0);  /* 254 ≤ 255 */

    puts("    PASSED");
}

/* ================================================================== */
/*  3. compute_overflow                                                 */
/* ================================================================== */
static void test_compute_overflow(void)
{
    puts("  [compute_overflow]");

    /* non-ADD/SUB → -1 */
    assert(compute_overflow(50,50,100,MUL)==-1);
    assert(compute_overflow(50,50,100,AND)==-1);
    assert(compute_overflow(50,50,100,OR) ==-1);
    assert(compute_overflow(50,50,100,SAL)==-1);
    assert(compute_overflow(50,50,100,LDI)==-1);

    /* ADD: no overflow */
    assert(compute_overflow(10,  20,  30,  ADD)==0);
    assert(compute_overflow(-10,-20, -30,  ADD)==0);
    assert(compute_overflow(10, -20, -10,  ADD)==0);

    /* ADD: overflow — pos+pos→neg */
    assert(compute_overflow(100,100,(int8_t)(100+100),ADD)==1);
    /* ADD: overflow — neg+neg→pos */
    assert(compute_overflow(-100,-100,(int8_t)(-200),ADD)==1);

    /* SUB: no overflow (same-sign operands, early return 0) */
    assert(compute_overflow(30, 20,  10, SUB)==0);
    assert(compute_overflow(-30,-20,-10, SUB)==0);
    assert(compute_overflow(10, 10,   0, SUB)==0);

    /* SUB: pos-pos→neg fires the shared pos+pos→neg check → returns 1 */
    assert(compute_overflow(5, 15, -10, SUB)==1);

    /* SUB: mixed-sign operands → 0 (early return) */
    assert(compute_overflow(20, -5, 25, SUB)==0);

    puts("    PASSED");
}

/* ================================================================== */
/*  4. compute_negative                                                 */
/* ================================================================== */
static void test_compute_negative(void)
{
    puts("  [compute_negative]");
    assert(compute_negative(0)   ==0);
    assert(compute_negative(1)   ==0);
    assert(compute_negative(127) ==0);
    assert(compute_negative(-1)  ==1);
    assert(compute_negative(-128)==1);
    assert(compute_negative(-50) ==1);
    puts("    PASSED");
}

/* ================================================================== */
/*  5. compute_zero                                                     */
/* ================================================================== */
static void test_compute_zero(void)
{
    puts("  [compute_zero]");
    assert(compute_zero(0)   ==1);
    assert(compute_zero(1)   ==0);
    assert(compute_zero(-1)  ==0);
    assert(compute_zero(127) ==0);
    assert(compute_zero(-128)==0);
    puts("    PASSED");
}

/* ================================================================== */
/*  6. updateSREG — ADD                                                 */
/*     All five flags (C,V,N,Z,S) are fully recalculated for ADD.      */
/* ================================================================== */
static void test_updateSREG_ADD(void)
{
    puts("  [updateSREG ADD]");
    ProcessorState s;
    int8_t res;

    /* 10+20=30: no flags set; start dirty to prove all are cleared */
    s = make_state(0xFF);
    updateSREG(&s, ADD, 10, 20, 30);
    assert(flag_C(s.sreg)==0); assert(flag_V(s.sreg)==0);
    assert(flag_N(s.sreg)==0); assert(flag_S(s.sreg)==0); assert(flag_Z(s.sreg)==0);
    assert((s.sreg & 0xE0)==0);

    /* 5+(-5)=0: Z=1, rest 0 */
    s = make_state(0x00);
    updateSREG(&s, ADD, 5, -5, 0);
    assert(flag_Z(s.sreg)==1);
    assert(flag_N(s.sreg)==0); assert(flag_V(s.sreg)==0);
    assert(flag_C(s.sreg)==0); assert(flag_S(s.sreg)==0);

    /* (-10)+(-5)=-15: N=1, V=0, S=N^V=1 */
    s = make_state(0x00);
    updateSREG(&s, ADD, -10, -5, -15);
    assert(flag_N(s.sreg)==1);
    assert(flag_V(s.sreg)==0); assert(flag_Z(s.sreg)==0);
    assert(flag_S(s.sreg)==1);   /* 1 XOR 0 = 1 */

    /* 100+100→-56 (int8_t wrap): V=1, N=1, S=N^V=0 */
    s = make_state(0x00);
    res = (int8_t)(100+100);
    updateSREG(&s, ADD, 100, 100, res);
    assert(flag_V(s.sreg)==1); assert(flag_N(s.sreg)==1);
    assert(flag_S(s.sreg)==0);   /* 1 XOR 1 = 0 */
    assert(flag_Z(s.sreg)==0);

    /* (-100)+(-100)→56 (int8_t wrap): V=1, N=0, S=N^V=1 */
    s = make_state(0x00);
    res = (int8_t)(-100 + -100);
    updateSREG(&s, ADD, -100, -100, res);
    assert(flag_V(s.sreg)==1); assert(flag_N(s.sreg)==0);
    assert(flag_S(s.sreg)==1);   /* 0 XOR 1 = 1 */
    assert(flag_Z(s.sreg)==0);

    puts("    PASSED");
}

/* ================================================================== */
/*  7. updateSREG — SUB                                                 */
/*     C is NOT updated (persists). V, N, Z, S are updated.            */
/* ================================================================== */
static void test_updateSREG_SUB(void)
{
    puts("  [updateSREG SUB]");
    ProcessorState s;

    /* 30-20=10: start clean → V=0,N=0,Z=0,S=0; C persists (was 0) */
    s = make_state(0x00);
    updateSREG(&s, SUB, 30, 20, 10);
    assert(flag_C(s.sreg)==0);   /* not updated by SUB, was 0 */
    assert(flag_V(s.sreg)==0); assert(flag_N(s.sreg)==0);
    assert(flag_S(s.sreg)==0); assert(flag_Z(s.sreg)==0);
    assert((s.sreg & 0xE0)==0);

    /* C persists: if C=1 before SUB it stays 1 */
    s = make_state(FLAG_C_MASK);
    updateSREG(&s, SUB, 30, 20, 10);
    assert(flag_C(s.sreg)==1);   /* preserved */
    assert(flag_V(s.sreg)==0); assert(flag_N(s.sreg)==0); assert(flag_Z(s.sreg)==0);

    /* 10-10=0: Z=1 */
    s = make_state(0x00);
    updateSREG(&s, SUB, 10, 10, 0);
    assert(flag_Z(s.sreg)==1); assert(flag_N(s.sreg)==0);

    /* 5-15=-10: N=1, V=1 (pos-pos→neg triggers shared overflow check), S=N^V=0 */
    s = make_state(0x00);
    updateSREG(&s, SUB, 5, 15, -10);
    assert(flag_N(s.sreg)==1);
    assert(flag_V(s.sreg)==1);   /* pos+pos→neg overflow check fires for SUB too */
    assert(flag_Z(s.sreg)==0);
    assert(flag_S(s.sreg)==0);   /* 1 XOR 1 = 0 */

    /* 20-(-5)=25: mixed-sign → V=0, N=0, Z=0 */
    s = make_state(0x00);
    updateSREG(&s, SUB, 20, -5, 25);
    assert(flag_V(s.sreg)==0); assert(flag_N(s.sreg)==0); assert(flag_Z(s.sreg)==0);

    puts("    PASSED");
}

/* ================================================================== */
/*  8. updateSREG — MUL / AND / OR / SAL / SAR                         */
/*     N and Z are updated.  C, V, S PERSIST from state->sreg.         */
/* ================================================================== */
static void test_updateSREG_other_alu(void)
{
    puts("  [updateSREG MUL/AND/OR/SAL/SAR]");
    ProcessorState s;
    int8_t res;

    /* ----- MUL ----- */
    /* 4*5=20: N=0,Z=0; C and V persist (were 0) */
    s = make_state(0x00);
    updateSREG(&s, MUL, 4, 5, 20);
    assert(flag_N(s.sreg)==0); assert(flag_Z(s.sreg)==0);
    assert(flag_C(s.sreg)==0); assert(flag_V(s.sreg)==0);
    assert((s.sreg & 0xE0)==0);

    /* C and V persist through MUL */
    s = make_state(FLAG_C_MASK | FLAG_V_MASK);   /* C=1, V=1 */
    updateSREG(&s, MUL, 4, 5, 20);
    assert(flag_C(s.sreg)==1);   /* preserved */
    assert(flag_V(s.sreg)==1);   /* preserved */
    assert(flag_N(s.sreg)==0); assert(flag_Z(s.sreg)==0);

    /* S also persists through MUL */
    s = make_state(FLAG_S_MASK);   /* S=1 */
    updateSREG(&s, MUL, 4, 5, 20);
    assert(flag_S(s.sreg)==1);   /* preserved — S only recalculated for ADD/SUB */

    /* 0*7=0: Z=1 */
    s = make_state(0x00);
    updateSREG(&s, MUL, 0, 7, 0);
    assert(flag_Z(s.sreg)==1); assert(flag_N(s.sreg)==0);

    /* (-3)*4=-12: N=1 */
    s = make_state(0x00);
    updateSREG(&s, MUL, -3, 4, -12);
    assert(flag_N(s.sreg)==1); assert(flag_Z(s.sreg)==0);

    /* ----- AND ----- */
    /* 0xF0 & 0x0F = 0: Z=1 */
    s = make_state(0x00);
    updateSREG(&s, AND, (int8_t)0xF0, (int8_t)0x0F, 0);
    assert(flag_Z(s.sreg)==1);

    /* 0xFF & 0x80 = -128: N=1 */
    s = make_state(0x00);
    res = (int8_t)(0xFF & 0x80);
    updateSREG(&s, AND, (int8_t)0xFF, (int8_t)0x80, res);
    assert(flag_N(s.sreg)==1); assert(flag_Z(s.sreg)==0);

    /* V persists through AND */
    s = make_state(FLAG_V_MASK);
    updateSREG(&s, AND, (int8_t)0xF0, (int8_t)0x0F, 0);
    assert(flag_V(s.sreg)==1);   /* preserved */

    /* ----- OR ----- */
    /* 0|0=0: Z=1 */
    s = make_state(0x00);
    updateSREG(&s, OR, 0, 0, 0);
    assert(flag_Z(s.sreg)==1);

    /* 0x40|0x40=0x40: N=0, Z=0 */
    s = make_state(0x00);
    updateSREG(&s, OR, 0x40, 0x40, 0x40);
    assert(flag_N(s.sreg)==0); assert(flag_Z(s.sreg)==0);

    /* ----- SAL ----- */
    /* 1<<1=2: N=0, Z=0 */
    s = make_state(0x00);
    updateSREG(&s, SAL, 1, 1, 2);
    assert(flag_N(s.sreg)==0); assert(flag_Z(s.sreg)==0);

    /* 0<<4=0: Z=1 */
    s = make_state(0x00);
    updateSREG(&s, SAL, 0, 4, 0);
    assert(flag_Z(s.sreg)==1);

    /* 64<<1 → -128 in int8_t: N=1 */
    s = make_state(0x00);
    res = (int8_t)(64 << 1);
    updateSREG(&s, SAL, 64, 1, res);
    assert(flag_N(s.sreg)==1);

    /* ----- SAR ----- */
    /* -4>>1=-2: N=1 */
    s = make_state(0x00);
    res = (int8_t)(-4 >> 1);
    updateSREG(&s, SAR, -4, 1, res);
    assert(flag_N(s.sreg)==1); assert(flag_Z(s.sreg)==0);

    /* 4>>4=0: Z=1 */
    s = make_state(0x00);
    updateSREG(&s, SAR, 4, 4, 0);
    assert(flag_Z(s.sreg)==1);

    puts("    PASSED");
}

/* ================================================================== */
/*  9. updateSREG — LDI / LB / SB / BEQZ / JR                         */
/*     No flag is recalculated — ALL flags persist from state->sreg.   */
/*     (bits 7:5 are still cleared by the write-back mask)             */
/* ================================================================== */
static void test_updateSREG_noflags(void)
{
    puts("  [updateSREG non-ALU ops — flags persist]");
    ProcessorState s;
    Opcode no_flag_ops[] = { LDI, LB, SB, BEQZ, JR };
    int n = (int)(sizeof(no_flag_ops)/sizeof(no_flag_ops[0]));
    int i;

    /* Start with all flags 0 — they should stay 0 */
    for (i = 0; i < n; i++) {
        s = make_state(0x00);
        updateSREG(&s, no_flag_ops[i], 100, 100, (int8_t)(100+100));
        assert(flag_C(s.sreg)==0); assert(flag_V(s.sreg)==0);
        assert(flag_N(s.sreg)==0); assert(flag_S(s.sreg)==0); assert(flag_Z(s.sreg)==0);
        assert((s.sreg & 0xE0)==0);
    }

    /* Start with all flags set (0x1F) — they should ALL persist */
    for (i = 0; i < n; i++) {
        s = make_state(0x1F);
        updateSREG(&s, no_flag_ops[i], 0, 0, 0);
        assert(flag_C(s.sreg)==1); assert(flag_V(s.sreg)==1);
        assert(flag_N(s.sreg)==1); assert(flag_S(s.sreg)==1); assert(flag_Z(s.sreg)==1);
        assert((s.sreg & 0xE0)==0);
    }

    puts("    PASSED");
}

/* ================================================================== */
/*  10. SREG bits 7:5 are always zero after any updateSREG call        */
/* ================================================================== */
static void test_upper_bits_zero(void)
{
    puts("  [upper bits 7:5 always zero]");
    ProcessorState s;
    Opcode all_ops[] = { ADD,SUB,MUL,LDI,BEQZ,AND,OR,JR,SAL,SAR,LB,SB };
    int n = (int)(sizeof(all_ops)/sizeof(all_ops[0]));
    int8_t vals[][3] = {
        {100,  100, (int8_t)(100+100)},
        {-100,-100, (int8_t)(-200)},
        {0,    0,   0},
        {127,  1,   (int8_t)(127+1)},
        {-128,-1,   (int8_t)(-128-1)},
    };
    int op, v;
    for (op = 0; op < n; op++) {
        for (v = 0; v < 5; v++) {
            s = make_state(0xFF);
            updateSREG(&s, all_ops[op], vals[v][0], vals[v][1], vals[v][2]);
            assert((s.sreg & 0xE0)==0);
        }
    }
    puts("    PASSED");
}

/* ================================================================== */
int main(void)
{
    int total = 0, passed = 0;
    puts("===== SREG Unit Tests =====\n");

#define RUN(fn) do { puts("-- " #fn " --"); fn(); passed++; total++; } while(0)
    RUN(test_flag_readers);
    RUN(test_compute_carry);
    RUN(test_compute_overflow);
    RUN(test_compute_negative);
    RUN(test_compute_zero);
    RUN(test_updateSREG_ADD);
    RUN(test_updateSREG_SUB);
    RUN(test_updateSREG_other_alu);
    RUN(test_updateSREG_noflags);
    RUN(test_upper_bits_zero);
#undef RUN

    printf("\n===== %d/%d test groups passed =====\n", passed, total);
    puts("All SREG tests passed!");
    return 0;
}
