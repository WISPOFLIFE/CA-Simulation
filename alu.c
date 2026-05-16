
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define SREG_C_MASK 0x10 /* bit 4 — Carry       */
#define SREG_V_MASK 0x08 /* bit 3 — Overflow     */
#define SREG_N_MASK 0x04 /* bit 2 — Negative     */
#define SREG_S_MASK 0x02 /* bit 1 — Sign         */
#define SREG_Z_MASK 0x01 /* bit 0 — Zero         */

#define NUM_REGS 64
#define ALU_MEM_SIZE 64
#define DATA_MEM_SIZE 2048

extern unsigned char R[NUM_REGS];
extern uint8_t data_mem[DATA_MEM_SIZE];
extern uint8_t SREG;

typedef struct
{
    uint8_t regs[NUM_REGS];
    uint8_t memory[ALU_MEM_SIZE];
    uint8_t sreg;
} ALU;

typedef enum
{
    OP_ADD = 0,
    OP_SUB = 1,
    OP_MUL = 2,
    OP_MOVI = 3,
    OP_BEQZ = 4,
    OP_ANDI = 5,
    OP_EOR = 6,
    OP_BR = 7,
    OP_SLC = 8,
    OP_SRC = 9,
    OP_LDR = 10,
    OP_STR = 11
} Opcode;

/* ── helpers ── */
static inline bool get_C(uint8_t s) { return (s & SREG_C_MASK) != 0; }
static inline bool get_V(uint8_t s) { return (s & SREG_V_MASK) != 0; }
static inline bool get_N(uint8_t s) { return (s & SREG_N_MASK) != 0; }
static inline bool get_S(uint8_t s) { return (s & SREG_S_MASK) != 0; }
static inline bool get_Z(uint8_t s) { return (s & SREG_Z_MASK) != 0; }

/*
 * set_flags — rebuild SREG, merging new flag bits into old.
 * Only the bits whose 'update_X' flag is true are changed;
 * the rest keep their previous value.
 */
static uint8_t merge_flags(uint8_t old_sreg,
                           bool C, bool update_C,
                           bool V, bool update_V,
                           bool N, bool update_N,
                           bool S, bool update_S,
                           bool Z, bool update_Z)
{
    uint8_t s = old_sreg & 0xE0; /* keep bits 7:5 cleared */
    s |= (update_C ? (C ? SREG_C_MASK : 0) : (old_sreg & SREG_C_MASK));
    s |= (update_V ? (V ? SREG_V_MASK : 0) : (old_sreg & SREG_V_MASK));
    s |= (update_N ? (N ? SREG_N_MASK : 0) : (old_sreg & SREG_N_MASK));
    s |= (update_S ? (S ? SREG_S_MASK : 0) : (old_sreg & SREG_S_MASK));
    s |= (update_Z ? (Z ? SREG_Z_MASK : 0) : (old_sreg & SREG_Z_MASK));
    return s;
}

/*
 * alu_execute — main dispatch.
 *
 * Reads and writes alu->regs[] directly.
 * Also mirrors writes to the global R[] and SREG so the pipeline
 * sees results immediately (no separate WB stage in Package 3).
 *
 * Returns: nothing written to *new_pc unless a branch/jump is taken.
 */
void alu_execute(ALU *alu, Opcode op,
                 uint8_t r1, uint8_t r2, uint8_t imm,
                 uint16_t pc, uint16_t *new_pc, bool *branch_taken)
{
    *new_pc = pc;
    *branch_taken = false;

    uint8_t r1_val = alu->regs[r1];
    uint8_t r2_val = alu->regs[r2];
    uint8_t result = r1_val;
    uint8_t old_sreg = alu->sreg;

    bool new_C = false, new_V = false, new_N = false,
         new_S = false, new_Z = false;
    bool upd_C = false, upd_V = false, upd_N = false,
         upd_S = false, upd_Z = false;

    switch (op)
    {

    case OP_ADD:
    {
        uint16_t sum = (uint16_t)r1_val + (uint16_t)r2_val;
        result = (uint8_t)(sum & 0xFF);
        /* Carry: bit 8 of unsigned sum */
        new_C = (sum >> 8) & 1;
        /* Overflow: same-sign inputs, different-sign result */
        int sa = (r1_val >> 7) & 1;
        int sb = (r2_val >> 7) & 1;
        int sr = (result >> 7) & 1;
        new_V = (sa == sb) && (sr != sa);
        new_N = (result >> 7) & 1;
        new_Z = (result == 0);
        new_S = new_N ^ new_V;
        upd_C = upd_V = upd_N = upd_S = upd_Z = true;
        break;
    }
    case OP_SUB:
    {
        uint16_t diff = (uint16_t)r1_val - (uint16_t)r2_val;
        result = (uint8_t)(diff & 0xFF);
        /* Overflow: different-sign inputs, result same sign as subtrahend */
        int sa = (r1_val >> 7) & 1;
        int sb = (r2_val >> 7) & 1;
        int sr = (result >> 7) & 1;
        new_V = (sa != sb) && (sr == sb);
        new_N = (result >> 7) & 1;
        new_Z = (result == 0);
        new_S = new_N ^ new_V;
        /* C, S updated for SUB too */
        upd_V = upd_N = upd_S = upd_Z = true;
        /* C is NOT updated by SUB per Package-3 spec */
        upd_C = false;
        break;
    }
    case OP_MUL:
    {
        uint16_t prod = (uint16_t)r1_val * (uint16_t)r2_val;
        result = (uint8_t)(prod & 0xFF);
        new_N = (result >> 7) & 1;
        new_Z = (result == 0);
        upd_N = upd_Z = true;
        break;
    }

    /* ── I-format ── */
    case OP_MOVI:
        /* R1 = sign-extended 6-bit IMM (sign-extend to 8 bits) */
        if ((imm >> 5) & 1)
            result = imm | 0xC0; /* sign extend */
        else
            result = imm & 0x3F;
        alu->regs[r1] = result;
        R[r1] = result;
        return; /* no flag updates */

    case OP_BEQZ:
        if (r1_val == 0)
        {
            /* sign-extend the 6-bit immediate */
            int8_t offset;
            if ((imm >> 5) & 1)
                offset = (int8_t)(imm | 0xC0);
            else
                offset = (int8_t)(imm & 0x3F);
            /* PC was already incremented during fetch, so "PC" here
               is (fetch_pc + 1).  Branch target = fetch_pc + 1 + offset */
            *new_pc = pc + offset; /* pc here is already pc+1 */
            *branch_taken = true;
        }
        return; /* no flag updates, no register write */

    case OP_ANDI:
        result = r1_val & (imm & 0x3F);
        new_N = (result >> 7) & 1;
        new_Z = (result == 0);
        upd_N = upd_Z = true;
        break;

    /* ── R-format logic ── */
    case OP_EOR:
        result = r1_val ^ r2_val;
        new_N = (result >> 7) & 1;
        new_Z = (result == 0);
        upd_N = upd_Z = true;
        break;

    case OP_BR:
        /* PC = R1[5:0] || R2[5:0]  (12-bit address) */
        *new_pc = ((uint16_t)(r1_val & 0x3F) << 6) | (r2_val & 0x3F);
        *branch_taken = true;
        return; /* no register write, no flag update */

    /* ── I-format shifts (circular) ── */
    case OP_SLC:
    {
        uint8_t shift = imm & 0x07;
        if (shift == 0)
        {
            result = r1_val;
            new_C = false;
        }
        else
        {
            result = (uint8_t)((r1_val << shift) | (r1_val >> (8 - shift)));
            new_C = (r1_val >> (8 - shift)) & 1; /* last bit rotated in */
        }
        new_N = (result >> 7) & 1;
        new_Z = (result == 0);
        upd_C = upd_N = upd_Z = true;
        break;
    }
    case OP_SRC:
    {
        uint8_t shift = imm & 0x07;
        if (shift == 0)
        {
            result = r1_val;
            new_C = false;
        }
        else
        {
            result = (uint8_t)((r1_val >> shift) | (r1_val << (8 - shift)));
            new_C = (r1_val >> (shift - 1)) & 1; /* last bit shifted out */
        }
        new_N = (result >> 7) & 1;
        new_Z = (result == 0);
        upd_C = upd_N = upd_Z = true;
        break;
    }

    /* ── Memory ── */
    case OP_LDR:
    {
        /* address is the 6-bit unsigned immediate */
        uint16_t addr = imm & 0x3F;
        result = data_mem[addr];
        alu->regs[r1] = result;
        R[r1] = result;
        printf("  [EX] DataMem[%u] --> R%d = %d (0x%02X)\n",
               addr, r1, result, result);
        return;
    }
    case OP_STR:
    {
        uint16_t addr = imm & 0x3F;
        data_mem[addr] = r1_val;
        alu->memory[addr < ALU_MEM_SIZE ? addr : 0] = r1_val;
        printf("  [EX] R%d = %d --> DataMem[%u] = 0x%02X\n",
               r1, r1_val, addr, r1_val);
        return;
    }

    default:
        return;
    }

    alu->regs[r1] = result;
    R[r1] = result;

    alu->sreg = merge_flags(old_sreg,
                            new_C, upd_C,
                            new_V, upd_V,
                            new_N, upd_N,
                            new_S, upd_S,
                            new_Z, upd_Z);
    SREG = alu->sreg;
}

void print_sreg(uint8_t sreg)
{
    printf("C=%d V=%d N=%d S=%d Z=%d",
           get_C(sreg), get_V(sreg), get_N(sreg), get_S(sreg), get_Z(sreg));
}