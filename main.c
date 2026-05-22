#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

unsigned short instructionMemory[1024];
unsigned char  R[64];
uint8_t        data_mem[2048]; //array of unsigned bis
uint16_t       PC   = 0;
uint8_t        SREG = 0;

int clock_cycle = 0;

void run_pipeline(char *filename);

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <program.txt>\n", argv[0]);
        return EXIT_FAILURE;
    }

    memset(instructionMemory, 0, sizeof(instructionMemory));
    memset(data_mem,          0, sizeof(data_mem));
    memset(R,                 0, sizeof(R));
    PC          = 0;
    SREG        = 0;
    clock_cycle = 0;

    run_pipeline(argv[1]);

    return EXIT_SUCCESS;
}