#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define NUM_GPRS        64
#define INST_MEM_SIZE   1024
#define DATA_MEM_SIZE   2048

#define SREG_C_MASK  0x10   // bit 4 
#define SREG_V_MASK  0x08   // bit 3
#define SREG_N_MASK  0x04   // bit 2
#define SREG_S_MASK  0x02   // bit 1
#define SREG_Z_MASK  0x01   // bit 0

static const char *MNEMONICS[] = {
    "ADD", "SUB", "MUL", "MOVI",
    "BEQZ", "ANDI", "EOR", "BR",
    "SLC", "SRC", "LDR", "STR"
};
#define NUM_MNEMONICS 12

const char *opcode_to_mnemonic(int opcode)
{
    if (opcode >= 0 && opcode < NUM_MNEMONICS)
        return MNEMONICS[opcode];
    return "???";
}

static void print_binary16(uint16_t val)
{
    for (int i = 15; i >= 0; i--)
        printf("%d", (val >> i) & 1);
}

static void print_sreg(uint8_t sreg)
{
    printf("C=%d V=%d N=%d S=%d Z=%d",
           (sreg & SREG_C_MASK) ? 1 : 0,
           (sreg & SREG_V_MASK) ? 1 : 0,
           (sreg & SREG_N_MASK) ? 1 : 0,
           (sreg & SREG_S_MASK) ? 1 : 0,
           (sreg & SREG_Z_MASK) ? 1 : 0);
}

void print_cycle_header(int cycle)
{
    printf("\n========================================\n");
    printf("           Clock Cycle %d\n", cycle);
    printf("========================================\n");
}

void print_stage_IF(uint16_t pc_before, int16_t raw_instr)
{
    printf("\n[IF - Instruction Fetch]\n");
    if (raw_instr == -1) {
        printf("  Stage is EMPTY (no instruction to fetch)\n");
        return;
    }
    printf("  Input  : PC = %u\n", pc_before);
    printf("  Fetched: 0x%04X  (binary: ", (uint16_t)raw_instr);
    print_binary16((uint16_t)raw_instr);
    printf(")\n");
    printf("  PC incremented to: %u\n", pc_before + 1);
}

void print_stage_ID(int16_t raw_instr,
                    int opcode, int r1, int r2, int imm,
                    int8_t r1_val, int8_t r2_val)
{
    printf("\n[ID - Instruction Decode]\n");
    if (raw_instr == -1) {
        printf("  Stage is EMPTY\n");
        return;
    }

    const char *mnem = opcode_to_mnemonic(opcode);
    int is_r = (opcode == 0 || opcode == 1 || opcode == 2 ||
                opcode == 6 || opcode == 7);

    printf("  Instruction : 0x%04X  (%s)\n", (uint16_t)raw_instr, mnem);
    printf("  Opcode      : %d (%s)\n", opcode, mnem);
    printf("  R1          : R%d\n", r1);

    if (is_r) {
        printf("  R2          : R%d\n", r2);
        printf("  R1 value    : %d (0x%02X)\n", r1_val, (uint8_t)r1_val);
        printf("  R2 value    : %d (0x%02X)\n", r2_val, (uint8_t)r2_val);
    } else {
        printf("  IMM         : %d\n", imm);
        printf("  R1 value    : %d (0x%02X)\n", r1_val, (uint8_t)r1_val);
    }
}

void print_stage_EX(int16_t raw_instr,
                    const char *mnemonic,
                    int result,
                    int branch_taken)
{
    printf("\n[EX - Execute]\n");
    if (raw_instr == -1) {
        printf("  Stage is EMPTY\n");
        return;
    }

    printf("  Instruction : 0x%04X  (%s)\n", (uint16_t)raw_instr, mnemonic);
    if (branch_taken)
        printf("  Branch/Jump : TAKEN  --> New PC = %d\n", result);
    else
        printf("  Result      : %d (0x%02X)\n", (int8_t)result, (uint8_t)result);
}

void print_register_change(const char *stage, int reg_num, int new_val)
{
    if (reg_num >= 0 && reg_num < 64) {
        printf("  [%s] R%d changed --> %d (0x%02X)\n",
               stage, reg_num, (int8_t)new_val, (uint8_t)new_val);
    } else if (reg_num == 64) {
        printf("  [%s] SREG changed --> 0x%02X  ", stage, (uint8_t)new_val);
        print_sreg((uint8_t)new_val);
        printf("\n");
    } else if (reg_num == 65) {
        printf("  [%s] PC changed --> %d\n", stage, (uint16_t)new_val);
    }
}

void print_memory_change(const char *stage, uint16_t address, uint8_t new_val)
{
    printf("  [%s] DataMem[%u] changed --> %d (0x%02X)\n",
           stage, address, new_val, new_val);
}

void print_flush_notice(uint16_t flushed_pc)
{
    printf("\n  *** FLUSH: Instruction at PC=%u dropped from pipeline ***\n",
           flushed_pc);
}

void print_final_report(int8_t  *gpr,
                        uint8_t  sreg,
                        uint16_t pc,
                        int16_t *inst_mem,
                        uint8_t *dmem)
{
    printf("\n");
    printf("########################################\n");
    printf("#        FINAL STATE REPORT            #\n");
    printf("########################################\n");

    printf("\n--- General-Purpose Registers (R0–R63) ---\n");
    for (int i = 0; i < NUM_GPRS; i++) {
        printf("  R%-2d = %4d  (0x%02X)", i, gpr[i], (uint8_t)gpr[i]);
        if ((i + 1) % 4 == 0) printf("\n");
        else                  printf("  |  ");
    }
    printf("\n");

    printf("\n--- Status Register (SREG) ---\n");
    printf("  Raw value : 0x%02X\n", sreg);
    printf("  Flags     : ");
    print_sreg(sreg);
    printf("\n");

    printf("\n--- Program Counter (PC) ---\n");
    printf("  PC = %u\n", pc);

    printf("\n--- Instruction Memory (non-zero entries) ---\n");
    int any = 0;
    for (int i = 0; i < INST_MEM_SIZE; i++) {
        if (inst_mem[i] != 0) {
            printf("  InstMem[%4d] = 0x%04X  (", i, (uint16_t)inst_mem[i]);
            print_binary16((uint16_t)inst_mem[i]);
            printf(")\n");
            any = 1;
        }
    }
    if (!any) printf("  (all zero)\n");

    printf("\n--- Data Memory (non-zero entries) ---\n");
    any = 0;
    for (int i = 0; i < DATA_MEM_SIZE; i++) {
        if (dmem[i] != 0) {
            printf("  DataMem[%4d] = %4d  (0x%02X)\n",
                   i, dmem[i], dmem[i]);
            any = 1;
        }
    }
    if (!any) printf("  (all zero)\n");

    printf("\n########################################\n");
    printf("#           END OF SIMULATION          #\n");
    printf("########################################\n");
}