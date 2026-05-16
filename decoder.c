/*
 * decoder.c — Package 3 instruction decoder
 *
 * Task 1 (loadProgram): reads the assembly text file, assembles each
 *   mnemonic line into a 16-bit word, and stores it in instructionMemory[].
 *
 * Task 2 (decode): extracts fields from a 16-bit word into DecodedInstruction.
 *   Also reads current register values for val1/val2.
 *
 * Instruction format (16 bits):
 *   R-format:  [15:12] opcode | [11:6] R1 | [5:0] R2
 *   I-format:  [15:12] opcode | [11:6] R1 | [5:0] IMMEDIATE (signed 6-bit)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Shared globals */
extern unsigned short instructionMemory[1024];
extern unsigned char  R[64];

typedef struct {
    int opcode;
    int r1;
    int r2;
    int immediate;  /* sign-extended 6-bit value */
    int val1;       /* R[r1] at decode time */
    int val2;       /* R[r2] at decode time (R-format only) */
} DecodedInstruction;

/* ── TASK 1: load & assemble program from text file ── */
void loadProgram(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("[Error] Decoder: Could not open '%s'\n", filename);
        return;
    }

    char line[128];
    int  address = 0;

    while (fgets(line, sizeof(line), file) && address < 1024) {
        /* strip newline / carriage-return */
        line[strcspn(line, "\r\n")] = '\0';

        /* skip blank lines and comments */
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
            continue;

        char *tok1 = strtok(line,  " \t");
        char *tok2 = strtok(NULL, " \t");
        char *tok3 = strtok(NULL, " \t");

        if (tok1 == NULL) continue;

        /* ── opcode → 4-bit number ── */
        unsigned short opcodeNum = 0;
        if      (strcmp(tok1, "ADD")  == 0) opcodeNum = 0;
        else if (strcmp(tok1, "SUB")  == 0) opcodeNum = 1;
        else if (strcmp(tok1, "MUL")  == 0) opcodeNum = 2;
        else if (strcmp(tok1, "MOVI") == 0) opcodeNum = 3;
        else if (strcmp(tok1, "BEQZ") == 0) opcodeNum = 4;
        else if (strcmp(tok1, "ANDI") == 0) opcodeNum = 5;
        else if (strcmp(tok1, "EOR")  == 0) opcodeNum = 6;
        else if (strcmp(tok1, "BR")   == 0) opcodeNum = 7;
        else if (strcmp(tok1, "SLC")  == 0) opcodeNum = 8;
        else if (strcmp(tok1, "SRC")  == 0) opcodeNum = 9;
        else if (strcmp(tok1, "LDR")  == 0) opcodeNum = 10;
        else if (strcmp(tok1, "STR")  == 0) opcodeNum = 11;
        else {
            printf("[Decoder] Unknown mnemonic '%s' at line %d — skipped.\n",
                   tok1, address + 1);
            continue;
        }

        /* ── parse R1 field ── */
        int r1_idx = 0;
        if (tok2 != NULL) {
            if (tok2[0] == 'R' || tok2[0] == 'r')
                r1_idx = atoi(tok2 + 1);
            else
                r1_idx = atoi(tok2);
        }

        /* ── parse R2 / IMM field ── */
        int r2_or_imm = 0;
        if (tok3 != NULL) {
            if (tok3[0] == 'R' || tok3[0] == 'r')
                r2_or_imm = atoi(tok3 + 1);
            else
                r2_or_imm = atoi(tok3);  /* may be negative */
        }

        /*
         * Pack into 16-bit word.
         * Immediate/R2 field is 6 bits — mask to 6 bits so negative
         * values are stored correctly in 2's complement 6-bit form.
         */
        unsigned short word = 0;
        word |= (opcodeNum      & 0x0F) << 12;
        word |= (r1_idx         & 0x3F) << 6;
        word |= (r2_or_imm      & 0x3F);        /* 6-bit mask handles sign */

        instructionMemory[address++] = word;
    }

    fclose(file);
    printf("[Decoder] Loaded %d instruction(s) into instruction memory.\n",
           address);
}

/* ── TASK 2: decode a 16-bit instruction word ── */
DecodedInstruction decode(unsigned short instruction)
{
    DecodedInstruction d;

    d.opcode    = (instruction >> 12) & 0x0F;
    d.r1        = (instruction >>  6) & 0x3F;
    d.r2        =  instruction        & 0x3F;
    d.immediate =  instruction        & 0x3F;

    /* Sign-extend the 6-bit immediate to a full int */
    if ((d.immediate >> 5) & 1)
        d.immediate |= 0xFFFFFFC0;   /* fill upper bits with 1s */

    /* Read current register values */
    d.val1 = (int)(signed char)R[d.r1];
    d.val2 = (int)(signed char)R[d.r2];

    return d;
}