/*
 * memory.c — Package 3 memory module
 *
 * Harvard architecture:
 *   Instruction memory : 1024 × 16-bit words
 *   Data memory        : 2048 × 8-bit bytes
 *   Register file      : 64 × 8-bit GPRs + 8-bit SREG + 16-bit PC
 *
 * All arrays (instructionMemory, data_mem, R, SREG, PC) are the
 * canonical globals defined in main.c.  This file provides helper
 * structs and functions used internally.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define INST_MEM_SIZE    1024
#define DATA_MEM_SIZE    2048
#define NUM_GPRS         64

/* Status-register bit masks (same order as alu.c) */
#define SREG_C_MASK      (1 << 4)   /* Carry    */
#define SREG_V_MASK      (1 << 3)   /* Overflow */
#define SREG_N_MASK      (1 << 2)   /* Negative */
#define SREG_S_MASK      (1 << 1)   /* Sign     */
#define SREG_Z_MASK      (1 << 0)   /* Zero     */
#define SREG_RESERVED    (0xE0)     /* bits 7:5 always 0 */

/* ── Struct definitions (for local use / pipeline.c) ── */

typedef struct {
    uint16_t memory[INST_MEM_SIZE];
} InstructionMemory;

typedef struct {
    uint8_t memory[DATA_MEM_SIZE];
} DataMemory;

typedef struct {
    uint8_t  gpr[NUM_GPRS];
    uint8_t  sreg;
    uint16_t pc;
} RegisterFile;

/* ── Instruction memory helpers ── */

void init_instruction_memory(InstructionMemory *imem) {
    if (!imem) return;
    memset(imem->memory, 0, sizeof(imem->memory));
}

int load_instruction(InstructionMemory *imem, uint16_t addr, uint16_t instr) {
    if (!imem || addr >= INST_MEM_SIZE) return 0;
    imem->memory[addr] = instr;
    return 1;
}

int fetch_instruction(const InstructionMemory *imem,
                      uint16_t addr, uint16_t *out) {
    if (!imem || !out || addr >= INST_MEM_SIZE) return 0;
    *out = imem->memory[addr];
    return 1;
}

/* ── Data memory helpers ── */

void init_data_memory(DataMemory *dmem) {
    if (!dmem) return;
    memset(dmem->memory, 0, sizeof(dmem->memory));
}

int write_data_byte(DataMemory *dmem, uint16_t addr, uint8_t val) {
    if (!dmem || addr >= DATA_MEM_SIZE) return 0;
    dmem->memory[addr] = val;
    return 1;
}

int read_data_byte(const DataMemory *dmem, uint16_t addr, uint8_t *val) {
    if (!dmem || !val || addr >= DATA_MEM_SIZE) return 0;
    *val = dmem->memory[addr];
    return 1;
}

/* ── Register file helpers ── */

void init_register_file(RegisterFile *rf) {
    if (!rf) return;
    memset(rf->gpr, 0, sizeof(rf->gpr));
    rf->sreg = 0;
    rf->pc   = 0;
}

int read_gpr(const RegisterFile *rf, uint8_t n, uint8_t *val) {
    if (!rf || !val || n >= NUM_GPRS) return 0;
    *val = rf->gpr[n];
    return 1;
}

int write_gpr(RegisterFile *rf, uint8_t n, uint8_t val) {
    if (!rf || n >= NUM_GPRS) return 0;
    rf->gpr[n] = val;
    return 1;
}

uint16_t read_pc(const RegisterFile *rf) {
    return rf ? rf->pc : 0;
}

void write_pc(RegisterFile *rf, uint16_t pc) {
    if (!rf) return;
    rf->pc = pc & 0x03FF; /* wrap at 1024 */
}

void increment_pc(RegisterFile *rf) {
    if (!rf) return;
    write_pc(rf, rf->pc + 1);
}

uint8_t read_sreg(const RegisterFile *rf) {
    return rf ? rf->sreg : 0;
}

void write_sreg(RegisterFile *rf, uint8_t val) {
    if (!rf) return;
    rf->sreg = val & ~SREG_RESERVED;
}

/* ── Individual flag accessors ── */

static int flag_bit(const RegisterFile *rf, uint8_t mask) {
    return rf ? ((rf->sreg & mask) ? 1 : 0) : 0;
}
static void set_flag(RegisterFile *rf, uint8_t mask, int val) {
    if (!rf) return;
    if (val) rf->sreg |= mask; else rf->sreg &= ~mask;
    rf->sreg &= ~SREG_RESERVED;
}

int  get_carry_flag(const RegisterFile *rf)        { return flag_bit(rf, SREG_C_MASK); }
void set_carry_flag(RegisterFile *rf, int v)       { set_flag(rf, SREG_C_MASK, v); }
int  get_overflow_flag(const RegisterFile *rf)     { return flag_bit(rf, SREG_V_MASK); }
void set_overflow_flag(RegisterFile *rf, int v)    { set_flag(rf, SREG_V_MASK, v); }
int  get_negative_flag(const RegisterFile *rf)     { return flag_bit(rf, SREG_N_MASK); }
void set_negative_flag(RegisterFile *rf, int v)    { set_flag(rf, SREG_N_MASK, v); }
int  get_sign_flag(const RegisterFile *rf)         { return flag_bit(rf, SREG_S_MASK); }
void set_sign_flag(RegisterFile *rf, int v)        { set_flag(rf, SREG_S_MASK, v); }
int  get_zero_flag(const RegisterFile *rf)         { return flag_bit(rf, SREG_Z_MASK); }
void set_zero_flag(RegisterFile *rf, int v)        { set_flag(rf, SREG_Z_MASK, v); }

void update_zero_flag(RegisterFile *rf, uint8_t result) {
    set_zero_flag(rf, result == 0);
}
void update_negative_flag(RegisterFile *rf, uint8_t result) {
    set_negative_flag(rf, (result & 0x80) ? 1 : 0);
}
void update_sign_flag(RegisterFile *rf) {
    set_sign_flag(rf, get_negative_flag(rf) ^ get_overflow_flag(rf));
}
void reset_flags(RegisterFile *rf) {
    if (!rf) return;
    rf->sreg &= SREG_RESERVED; /* clear all 5 flag bits */
    rf->sreg &= ~SREG_RESERVED;
}

int is_valid_inst_address(uint16_t addr) { return addr < INST_MEM_SIZE; }
int is_valid_data_address(uint16_t addr) { return addr < DATA_MEM_SIZE; }
int is_valid_gpr_number(uint8_t n)       { return n < NUM_GPRS; }