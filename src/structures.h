// Pipeline registers between stages
typedef struct {
    uint16_t instruction;   // raw 16-bit binary
    uint16_t pc;            // PC of this instruction (stored for branch calc)
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
    uint16_t         pc;        // saved PC for branch target calc
    int              valid;
} ID_EX_Reg;

typedef struct {
    int      dest_reg;      // register to write back (-1 = no writeback)
    int8_t   result;        // ALU or memory result
    uint16_t new_pc;        // updated PC if branch/jump taken
    FlushType flush;
    int       valid;
} EX_Result;

// Full processor state
typedef struct {
    int8_t   regs[64];          // R0–R63, 8-bit general purpose
    uint16_t pc;                // 16-bit program counter
    uint8_t  sreg;              // bits: 0=Z,1=S,2=N,3=V,4=C (bits7:5 = 0)
    uint8_t  instr_mem[1024];   // 16-bit words stored as byte pairs
    uint8_t  data_mem[2048];    // 8-bit per address
    int      instr_count;       // number of instructions loaded
    int      clock_cycle;
    IF_ID_Reg  if_id;
    ID_EX_Reg  id_ex;
    EX_Result  ex_out;
} ProcessorState;