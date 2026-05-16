#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdbool.h>


#define NUM_REGS 64
#define INST_MEM_SIZE 1024
#define DATA_MEM_SIZE 2048
#define ALU_MEM_SIZE 64





typedef struct {
    uint16_t memory[INST_MEM_SIZE];
} InstructionMemory;

extern unsigned short instructionMemory[1024]; 

typedef struct {
    uint8_t regs[NUM_REGS];
    uint8_t memory[ALU_MEM_SIZE];
    uint8_t sreg;
} ALU;

typedef struct {
    int opcode;
    int r1;
    int r2;
    int immediate;
    int val1;
    int val2;
} DecodedInstruction;

typedef struct {
    int valid;
    int16_t raw_instr;
    uint16_t pc_at_fetch;
    DecodedInstruction decoded;
} PipelineStage;
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


DecodedInstruction decode(unsigned short instruction) ;
void alu_execute(ALU *alu, Opcode op, uint8_t r1, uint8_t r2, uint8_t imm, uint16_t pc, uint16_t *new_pc, bool *branch_taken);
int fetch_instruction(const InstructionMemory* imem, uint16_t address, uint16_t* instruction) ;

int pipelineStage(ALU* alu, uint16_t* global_pc, unsigned short* inst_mem, PipelineStage* threeStages){
   uint16_t pc=0;
   uint16_t fetched_val;
   bool branch_taken=false;
   uint16_t count=threeStages->pc_at_fetch;
   if(count>1){
    DecodedInstruction prevDec=threeStages->decoded;
    alu_execute(alu, (Opcode)prevDec.opcode,prevDec.r1, prevDec.r2, prevDec.immediate, *global_pc,&pc, &branch_taken);
   }
   if(branch_taken){
    *global_pc=pc;
    threeStages->pc_at_fetch=0;
    threeStages->raw_instr = 0;
    return 0;
   }
    if(count>0){
   threeStages->decoded=decode(threeStages->raw_instr);
    }
   fetch_instruction((InstructionMemory*)inst_mem,*global_pc, &fetched_val);
   threeStages->raw_instr=fetched_val;
   threeStages->pc_at_fetch=count+1;
   (*global_pc)++;
  return 1;
}


