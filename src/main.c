#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"

int main(int argc, char **argv)
{
    Chip8CPU* cpu = NULL;
    int romSize, opcodeSize;

    if (argc != 2) {
        fprintf(stderr, "No ROM file specified.\n");
        exit(1);
    }

    cpu = malloc(sizeof(Chip8CPU));
    if (cpu == NULL) {
        fprintf(stderr, "malloc error.\n");
        return -1;
    }

    cpu_init(cpu);
    opcodeSize = 0;
    romSize = load_rom(cpu, argv[1]);

    while (cpu->PC < ROM_START + romSize) {
        opcodeSize = disas(cpu);
        cpu->PC += opcodeSize;
    }

    free(cpu);

    return 0;
}
