#include "parser.h"
#include <stdio.h>
#include <string.h>
#include "structures.h"
#include "processor.h"

InstructionFormat get_format(Opcode op);

//binary printing for instructions
void print_binary(short int value) {
    for (int i = 15; i >= 0; i--) {
        printf("%d", (value >> i) & 1);

        //(opcode | R1 | R2/IMM)
        if (i == 12 || i == 6) printf(" ");
    }
    printf("\n");
}

//open text file, read line by line, encode each instruction, store in instr_mem, return count
int parse_file(const char *filename, ProcessorState *state) {
    FILE *file = fopen(filename, "r");
    if (!file) return -1;

    char line[100];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {

        short int instr = encode_instruction(line);

        //binary rep
        printf("Encoded value: ");
        print_binary(instr);

        // 🔥 DEBUG: show raw stored instruction
        printf("PARSER DEBUG: stored instr[%d] = %d\n", count, instr);

        state->instr_mem[count++] = instr;
    }

    fclose(file);
    return count;
}

// Convert mnemonic to opcode 
Opcode mnemonic_to_opcode(const char *m) {
    if (strcmp(m, "ADD") == 0) return ADD;
    if (strcmp(m, "SUB") == 0) return SUB;
    if (strcmp(m, "MUL") == 0) return MUL;
    if (strcmp(m, "LDI") == 0) return LDI;
    if (strcmp(m, "BEQZ") == 0) return BEQZ;
    if (strcmp(m, "AND") == 0) return AND;
    if (strcmp(m, "OR") == 0) return OR;
    if (strcmp(m, "JR") == 0) return JR;
    if (strcmp(m, "SAL") == 0) return SAL;
    if (strcmp(m, "SAR") == 0) return SAR;
    if (strcmp(m, "LB") == 0) return LB;
    if (strcmp(m, "SB") == 0) return SB;

    printf("Error: Invalid instruction '%s'\n", m);
    exit(1);
}

InstructionFormat get_format(Opcode op) {
    if (op == ADD || op == SUB || op == MUL ||
        op == AND || op == OR || op == JR) {
        return FORMAT_R;
    }
    return FORMAT_I;
}

//encode the instruction to 16 bit binary
short int encode_instruction(const char *line) {

    if (!line || line[0] == '\n') return 0;

    char temp[100];
    strcpy(temp, line);

    char *mnemonic = strtok(temp, " \n");
    char *op1 = strtok(NULL, " \n");
    char *op2 = strtok(NULL, " \n");

    //check if more operands were used
    char *extra = strtok(NULL, " \n");
    if (extra != NULL) {
        printf("Error: Too many operands in line: %s\n", line);
        exit(1);
    }

    if (!mnemonic) return 0;
    if (!op1 || !op2) {
        printf("Error: Missing operands in line: %s\n", line);
        exit(1);
    }

    Opcode op = mnemonic_to_opcode(mnemonic);
    int opcode_bits = opcode_to_bits(op);

    short int instruction = 0;
    instruction = (opcode_bits << 12);

    if (get_format(op) == FORMAT_R) {

        int r1 = atoi(op1 + 1);
        int r2 = atoi(op2 + 1);

        // 🔥 DEBUG
        printf("PARSER DEBUG (R): opcode=%d r1=%d r2=%d\n", op, r1, r2);

        if (r1 < 0 || r1 > 63 || r2 < 0 || r2 > 63) {
            printf("Error: Invalid register in line: %s\n", line);
            exit(1);
        }

        instruction |= (r1 << 6);
        instruction |= r2;

    } else {

        int r1 = atoi(op1 + 1);
        int imm = atoi(op2);

        // 🔥 DEBUG
        printf("PARSER DEBUG (I): opcode=%d r1=%d imm=%d\n", op, r1, imm);

        if ((op == SAL || op == SAR) && imm < 0) {
            printf("Error: Shift immediate must be positive\n");
            exit(1);
        }

        instruction |= (r1 << 6);
        instruction |= (imm & 0x3F);
    }

    return instruction;   
}

int opcode_to_bits(Opcode op) {
    return (int) op;
}