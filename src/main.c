#include <stdio.h>
#include <stdlib.h>
#include "chip8.h"

int main(int argc, char **argv)
{
    Chip8CPU* cpu = NULL;

    if (argc != 2) {
        fprintf(stderr, "No ROM file specified.\n");
        exit(1);
    }

    cpu = malloc(sizeof(Chip8CPU));
    if (cpu == NULL) {
        fprintf(stderr, "malloc error.\n");
        exit(1);
    }

    chip8Init(cpu);
    chip8LoadROM(cpu, argv[1]);
    chip8Execute(cpu);

    chip8Exit(cpu);

    return 0;
}
