#include "memory.h"

// ============================================================================
// INSTRUCTION MEMORY FUNCTIONS
// ============================================================================

void init_instruction_memory(InstructionMemory* imem) {
    if(imem==NULL) 
        return; 

    memset(imem->memory, 0, sizeof(uint16_t)*INST_MEM_SIZE);
}

int load_instruction(InstructionMemory* imem, uint16_t address, uint16_t instruction) {
    if(imem==NULL) 
        return 0;
    if(!is_valid_inst_address(address))
        return 0;
    imem->memory[address] = instruction;
    return 1;
}

int fetch_instruction(const InstructionMemory* imem, uint16_t address, uint16_t* instruction) {
    if(imem==NULL || instruction==NULL)
        return 0;
    if(!is_valid_inst_address(address))
        return 0;

    *instruction = imem->memory[address];
    return 1;
}

void print_instruction_memory(const InstructionMemory* imem) {
    if(imem==NULL)
        return;
    
    printf("\n=== Instruction Memory (1024 x 16 bits) ===\n");
    printf("Address\t| Hex\t\t| Binary (16 bits)\n");
    printf("--------|---------------|-----------------\n");

    for(int i=0; i<INST_MEM_SIZE; i++) {
        if(imem->memory[i]!=0) {
            printf("%d\t| 0x%04X\t\t| ", i, imem->memory[i]);

            for(int bit=15; bit>=0; bit--) {
                printf("%d", (imem->memory[i] >> bit) & 1);
                if(bit==8)
                    printf(" "); // Space between upper and lower byte
            }
            printf("\n");
        }
    }
    printf("\nNote: Only non-zero memory locations are shown above.\n");
    printf("All other addresses contain 0.\n");
}

// ============================================================================
// DATA MEMORY FUNCTIONS
// ============================================================================

void init_data_memory(DataMemory* dmem) {
    if(dmem==NULL)
        return;

    memset(dmem->memory, 0, sizeof(uint8_t)*DATA_MEM_SIZE);
}

int write_data_byte(DataMemory* dmem, uint16_t address, uint8_t value) {
        return 0;
    if(!is_valid_data_address(address))
        return 0;

    dmem->memory[address] = value;
    return 1;
}

int read_data_byte(const DataMemory* dmem, uint16_t address, uint8_t* value) {
    if(dmem==NULL || value==NULL)
        return 0;
    if(!is_valid_data_address(address))
        return 0;

    *value = dmem->memory[address];
    return 1;
}

void print_data_memory(const DataMemory* dmem) {
    // Print data memory addresses 1024-2047 (data section)
    // Format: "Address X: 0xYY (decimal: Z, binary: ...)"
    if(dmem==NULL)
        return;
    
    printf("\n=== Data Memory (2048 x 8 bits) ===\n");
    printf("Address\t| Hex\t| Dec\t| Binary (8 bits)\n");
    printf("--------|-------|-------|----------------\n");

    for(int i=1024; i<DATA_MEM_SIZE; i++) {
        if(dmem->memory[i]!=0) {
            printf("%d\t| 0x%02X\t| %d\t| ", i, dmem->memory[i], dmem->memory[i]);

            for(int bit=7; bit>=0; bit--) {
                printf("%d", (dmem->memory[i] >> bit) & 1);
            }
            printf("\n");
        }
    }
    printf("\nNote: Only non-zero memory locations are shown above.\n");
    printf("All other addresses contain 0.\n");
}

// ============================================================================
// REGISTER FILE FUNCTIONS
// ============================================================================

void init_register_file(RegisterFile* regs) {
    if(regs==NULL)
        return;

    memset(regs->gpr, 0, sizeof(uint8_t)*NUM_GPRS);
    regs->sreg = 0;
    regs->pc = 0;
}

int read_gpr(const RegisterFile* regs, uint8_t reg_num, uint8_t* value) {
    if(regs==NULL || value==NULL)
        return 0;
    if(!is_valid_gpr_number(reg_num))
        return 0;

    *value = regs->gpr[reg_num];
    return 1;
}

int write_gpr(RegisterFile* regs, uint8_t reg_num, uint8_t value) {
    if(regs==NULL)
        return 0;
    if(!is_valid_gpr_number(reg_num))
        return 0;

    regs->gpr[reg_num] = value;
    return 1;
}

uint16_t read_pc(const RegisterFile* regs) {
    if(regs==NULL)
        return 0;

    return regs->pc;
}

void write_pc(RegisterFile* regs, uint16_t new_pc) {
    if(regs==NULL)
        return;

    regs->pc = new_pc & 0x03FF; // Ensure PC wraps around at 1024
}

void increment_pc(RegisterFile* regs) {
    if(regs==NULL)
        return;

    write_pc(regs, regs->pc + 1);
}

uint8_t read_sreg(const RegisterFile* regs) {
    if(regs==NULL)
        return 0;

    return regs->sreg;
}

void write_sreg(RegisterFile* regs, uint8_t value) {
    if(regs==NULL)
        return;

    regs->sreg = value & ~SREG_RESERVED_MASK;
}

// ============================================================================
// FLAG OPERATIONS
// ============================================================================

int get_carry_flag(const RegisterFile* regs) {
    if(regs==NULL)
        return 0;
    
    return (regs->sreg & SREG_C_MASK) ? 1 : 0;
}

void set_carry_flag(RegisterFile* regs, int value) {
    // Set Carry flag to 1 if value != 0, else clear it
    // Use bitwise OR to set, AND with complement to clear
    if(regs==NULL)
        return;

    if(value != 0)
        regs->sreg |= SREG_C_MASK;
    else
        regs->sreg &= ~SREG_C_MASK;
    
    regs->sreg &= ~SREG_RESERVED_MASK;
}

int get_overflow_flag(const RegisterFile* regs) {
    if(regs==NULL)
        return 0;

    return (regs->sreg & SREG_V_MASK) ? 1 : 0;
}

void set_overflow_flag(RegisterFile* regs, int value) {
    if(regs==NULL)
        return;

    if(value != 0)
        regs->sreg |= SREG_V_MASK;
    else
        regs->sreg &= ~SREG_V_MASK;

     regs->sreg &= ~SREG_RESERVED_MASK;
}

int get_negative_flag(const RegisterFile* regs) {
    if(regs==NULL)
        return 0;

    return (regs->sreg & SREG_N_MASK) ? 1 : 0;
}

void set_negative_flag(RegisterFile* regs, int value) {
    if(regs==NULL)
        return;

    if(value != 0)
        regs->sreg |= SREG_N_MASK;
    else
        regs->sreg &= ~SREG_N_MASK;

    regs->sreg &= ~SREG_RESERVED_MASK;
}

int get_sign_flag(const RegisterFile* regs) {
    if(regs==NULL)
        return 0;

    return (regs->sreg & SREG_S_MASK) ? 1 : 0;
}

void set_sign_flag(RegisterFile* regs, int value) {
    if(regs==NULL)
        return;

    if(value != 0)
        regs->sreg |= SREG_S_MASK;
    else
        regs->sreg &= ~SREG_S_MASK;

    regs->sreg &= ~SREG_RESERVED_MASK;
}

int get_zero_flag(const RegisterFile* regs) {
    if(regs==NULL)
        return 0;

    return (regs->sreg & SREG_Z_MASK) ? 1 : 0;
}

void set_zero_flag(RegisterFile* regs, int value) {
    if(regs==NULL)
        return;

    if(value != 0)
        regs->sreg |= SREG_Z_MASK;
    else
        regs->sreg &= ~SREG_Z_MASK;

    regs->sreg &= ~SREG_RESERVED_MASK;
}

void update_zero_flag(RegisterFile* regs, uint8_t result) {
    // Set Zero flag if result == 0, clear otherwise
    if(regs==NULL)
        return;
    
    set_zero_flag(regs, (result==0) ? 1 : 0);
}

void update_negative_flag(RegisterFile* regs, uint8_t result) {
    // Set Negative flag if MSB (bit 7) of result is 1
    // Check result & 0x80
    if(regs==NULL)
        return;

    set_negative_flag(regs, (result & 0x80) ? 1 : 0);
}

void update_sign_flag(RegisterFile* regs) {
    // Get N and V flags, XOR them, set S flag accordingly
    if(regs==NULL)
        return;
    
    int n = get_negative_flag(regs);
    int v = get_overflow_flag(regs);
    set_sign_flag(regs, (n ^ v) ? 1 : 0);
}

void reset_flags(RegisterFile* regs) {
    if(regs==NULL)
        return;

    regs->sreg &= ~(SREG_C_MASK | SREG_V_MASK | SREG_N_MASK | SREG_S_MASK | SREG_Z_MASK);
    regs->sreg &= ~SREG_RESERVED_MASK;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void print_all_registers(const RegisterFile* regs) {
    if(regs==NULL)
        return;
    
    // Print GPRs (R0-R63)
    printf("\n=== General Purpose Registers (R0 - R63) ===\n");
    printf("Each register: 8 bits\n");
    
    for(int i = 0; i < NUM_GPRS; i++) {
        if(i % 8 == 0) {
            printf("\nR%02d-R%02d: ", i, (i+7 < NUM_GPRS) ? i+7 : NUM_GPRS-1);
        }
        printf("R%02d=0x%02X (%3d)  ", i, regs->gpr[i], regs->gpr[i]);
    }
    
    // Print Special Registers
    printf("\n\n=== Special Purpose Registers ===\n");
    printf("PC (Program Counter): 0x%04X (%d)\n", regs->pc, regs->pc);
    
    printf("SREG (Status Register): 0x%02X (binary: ", regs->sreg);
    for(int bit = 7; bit >= 0; bit--) {
        printf("%d", (regs->sreg >> bit) & 1);
    }
    printf(")\n");
    
    printf("  Flags:\n");
    printf("    C (Carry):      %d\n", get_carry_flag(regs));
    printf("    V (Overflow):   %d\n", get_overflow_flag(regs));
    printf("    N (Negative):   %d\n", get_negative_flag(regs));
    printf("    S (Sign):       %d\n", get_sign_flag(regs));
    printf("    Z (Zero):       %d\n", get_zero_flag(regs));
}

int is_valid_inst_address(uint16_t address) {
    return (address < INST_MEM_SIZE) ? 1 : 0;
}

int is_valid_data_address(uint16_t address) {
    return (address < DATA_MEM_SIZE) ? 1 : 0;
}

int is_valid_gpr_number(uint8_t reg_num) {
    return (reg_num < NUM_GPRS) ? 1 : 0;
}