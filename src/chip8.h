#ifndef CHIP8_H
#define CHIP8_H

#include "cpu.h"

void chip8Init(Chip8CPU* cpu);
void chip8Exit(Chip8CPU* cpu);
int32_t chip8LoadROM(Chip8CPU* cpu, const char* file);
void chip8Execute(Chip8CPU* cpu);
void chip8DrawScreen(Chip8CPU* cpu);
void chip8PrintROMDisassembly(Chip8CPU* cpu, const char* file);

#endif
