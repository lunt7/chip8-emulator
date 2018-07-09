#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"

#define LOWER_BYTE(opcode)  (opcode & 0x00FF)
#define UPPER_BYTE(opcode)  ((opcode >> 8) & 0x00FF)

#define OP_NNN(opcode)  (opcode & 0x0FFF)
#define OP_N(opcode)    (opcode & 0x000F)
#define OP_X(opcode)    ((opcode >> 8) & 0x0F)
#define OP_Y(opcode)    ((opcode >> 4) & 0x0F)
#define OP_KK(opcode)   (opcode & 0x00FF)

static const uint8_t Chip8Font[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void cpuInit(Chip8CPU* cpu)
{
    memset(cpu, 0, sizeof(Chip8CPU));
    cpu->PC = ROM_START;
    memcpy(cpu->ram, Chip8Font, sizeof(Chip8Font));
}

void cpuUpdateTimers(Chip8CPU* cpu)
{
    if (cpu->DT) {
        cpu->DT--;
    }
    if (cpu->ST) {
        // TODO: play sound
        cpu->ST--;
    }
}

void cpuExecute(Chip8CPU* cpu)
{
    uint16_t opcode = (cpu->ram[cpu->PC] << 8) | (cpu->ram[cpu->PC + 1]);

    uint16_t addr   = OP_NNN(opcode);
    uint8_t  nibble = OP_N(opcode);
    uint8_t  x      = OP_X(opcode);
    uint8_t  y      = OP_Y(opcode);
    uint8_t  byte   = OP_KK(opcode);

    int32_t xx, yy, idx;
    uint8_t spriteByte, pixel;

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode) {
                /* 0nnn - SYS addr */
                case 0x0000:
                    cpu->PC = addr;
                    break;

                /* 00E0 - CLS */
                case 0x00E0:
                    memset(cpu->framebuff, 0, FRAMEBUFF_SIZE);
                    cpu->PC += 2;
                    break;

                /* 00EE - RET */
                case 0x00EE:
                    cpu->SP--;
                    cpu->PC = cpu->stack[cpu->SP];
                    break;

                default:
                    printf("INSTRUCTION UNKNOWN\n");
                    break;
            }
            break;

        /* 1nnn - JP addr */
        case 0x1000:
            cpu->PC = addr;
            break;

        /* 2nnn - CALL addr */
        case 0x2000:
            cpu->stack[cpu->SP] = cpu->PC + 2;
            cpu->SP++;
            cpu->PC = addr;
            break;

        /* 3xkk - SE Vx, byte */
        case 0x3000:
            if (cpu->V[x] == byte)
                cpu->PC += 4;
            else
                cpu->PC += 2;
            break;

        /* 4xkk - SNE Vx, byte */
        case 0x4000:
            if (cpu->V[x] != byte)
                cpu->PC += 4;
            else
                cpu->PC += 2;
            break;

        /* 5xy0 - SE Vx, Vy */
        case 0x5000:
            if (cpu->V[x] == cpu->V[y])
                cpu->PC += 4;
            else
                cpu->PC += 2;
            break;

        /* 6xkk - LD Vx, byte */
        case 0x6000:
            cpu->V[x] = byte;
            cpu->PC += 2;
            break;

        /* 7xkk - ADD Vx, byte */
        case 0x7000:
            cpu->V[x] += byte;
            cpu->PC += 2;
            break;

        case 0x8000:
            switch (opcode & 0x000F) {
                /* 8xy0 - LD Vx, Vy */
                case 0x0000:
                    cpu->V[x] = cpu->V[y];
                    cpu->PC += 2;
                    break;

                /* 8xy1 - OR Vx, Vy */
                case 0x0001:
                    cpu->V[x] |= cpu->V[y];
                    cpu->PC += 2;
                    break;

                /* 8xy2 - AND Vx, Vy */
                case 0x0002:
                    cpu->V[x] &= cpu->V[y];
                    cpu->PC += 2;
                    break;

                /* 8xy3 - XOR Vx, Vy */
                case 0x0003:
                    cpu->V[x] ^= cpu->V[y];
                    cpu->PC += 2;
                    break;

                /* 8xy4 - ADD Vx, Vy */
                case 0x0004:
                    cpu->V[0xF] = (cpu->V[x] + cpu->V[y] > 255) ? 1 : 0;
                    cpu->V[x] += cpu->V[y];
                    cpu->PC += 2;
                    break;

                /* 8xy5 - SUB Vx, Vy */
                case 0x0005:
                    cpu->V[0xF] = (cpu->V[x] > cpu->V[y]) ? 1 : 0;
                    cpu->V[x] -= cpu->V[y];
                    cpu->PC += 2;
                    break;

                /* 8xy6 - SHR Vx {, Vy} */
                case 0x0006:
                    cpu->V[0xF] = cpu->V[x] & 0x01;
                    cpu->V[x] >>= 1;
                    cpu->PC += 2;
                    break;

                /* 8xy7 - SUBN Vx, Vy */
                case 0x0007:
                    cpu->V[0xF] = (cpu->V[y] > cpu->V[x]) ? 1 : 0;
                    cpu->V[x] = cpu->V[y] - cpu->V[x];
                    cpu->PC += 2;
                    break;

                /* 8xyE - SHL Vx {, Vy} */
                case 0x000E:
                    cpu->V[0xF] = (cpu->V[x] >> 7);
                    cpu->V[x] <<= 1;
                    cpu->PC += 2;
                    break;

                default:
                    printf("INSTRUCTION UNKNOWN\n");
                    break;
            }
            break;

        /* 9xy0 - SNE Vx, Vy */
        case 0x9000:
            if (cpu->V[x] != cpu->V[y])
                cpu->PC += 4;
            else
                cpu->PC += 2;
            break;

        /* Annn - LD I, addr */
        case 0xA000:
            cpu->I = addr;
            cpu->PC += 2;
            break;

        /* Bnnn - JP V0, addr */
        case 0xB000:
            cpu->PC = cpu->V[0] + addr;
            break;

        /* Cxkk - RND Vx, byte */
        case 0xC000:
            cpu->V[x] = (rand() % 256) & byte;
            cpu->PC += 2;
            break;

        /* Dxyn - DRW Vx, Vy, nibble */
        case 0xD000:
            yy = cpu->V[y] % SCREEN_HEIGHT;
            for (int32_t row = 0; row < nibble; row++) {
                xx = cpu->V[x] % SCREEN_WIDTH;
                spriteByte = cpu->ram[ cpu->I + row ];
                for (int32_t col = 0; col < 8; col++) {           
                    pixel = (spriteByte >> (7 - col)) & 0x01;
                    idx = yy * SCREEN_WIDTH + xx; 
                    if (cpu->framebuff[idx] && pixel) {
                        cpu->V[0xF] = 1;
                    }
                    cpu->framebuff[idx] ^= pixel;
                    xx = (xx + 1) % SCREEN_WIDTH;
                }
                yy = (yy + 1) % SCREEN_HEIGHT;
            }
            cpu->PC += 2;
            break;

        case 0xE000:
            switch (opcode & 0x00FF) {
                /* Ex9E - SKP Vx */
                case 0x009E:
                    if (cpu->key[x])
                        cpu->PC += 4;
                    else
                        cpu->PC += 2;
                    break;

                /* ExA1 - SKNP Vx */
                case 0x00A1:
                    if (!cpu->key[x])
                        cpu->PC += 4;
                    else
                        cpu->PC += 2;
                    break;

                default:
                    printf("INSTRUCTION UNKNOWN\n");
                    break;
            }
            break;

        case 0xF000:
            switch (opcode & 0x00FF) {
                /* Fx07 - LD Vx, DT */
                case 0x0007:
                    cpu->V[x] = cpu->DT;
                    cpu->PC += 2;
                    break;

                /* Fx0A - LD Vx, K */
                case 0x000A:
                    for (int32_t i = 0; i < KEY_SIZE; i++) {
                        if (cpu->key[i]) {
                            cpu->V[x] = i;
                            cpu->PC += 2;
                            break;
                        }
                    }
                    break;

                /* Fx15 - LD DT, Vx */
                case 0x0015:
                    cpu->DT = cpu->V[x];
                    cpu->PC += 2;
                    break;

                /* Fx18 - LD ST, Vx */
                case 0x0018:
                    cpu->ST = cpu->V[x];
                    cpu->PC += 2;
                    break;

                /* Fx1E - ADD I, Vx */
                case 0x001E:
                    cpu->I += cpu->V[x];
                    cpu->PC += 2;
                    break;

                /* Fx29 - LD F, Vx */
                case 0x0029:
                    cpu->I = cpu->ram[5 * cpu->V[x] ];
                    cpu->PC += 2;
                    break;

                /* Fx33 - LD B, Vx */
                case 0x0033:
                    cpu->ram[cpu->I]        = (cpu->V[x] / 100);
                    cpu->ram[cpu->I + 1]    = (cpu->V[x] / 10) % 10;
                    cpu->ram[cpu->I + 2]    = (cpu->V[x] % 10);
                    cpu->PC += 2;
                    break;

                /* Fx55 - LD [I], Vx */
                case 0x0055:
                    for (int32_t i = 0; i <= x; i++) {
                        cpu->ram[cpu->I + i] = cpu->V[i];
                    }
                    cpu->PC += 2;
                    break;

                /* Fx65 - LD Vx, [I] */
                case 0x0065:
                    for (int32_t i = 0; i <= x; i++) {
                        cpu->V[i] = cpu->ram[cpu->I + i];
                    }
                    cpu->PC += 2;
                    break;

                default:
                    printf("INSTRUCTION UNKNOWN\n");
                    break;
            }
            break;

        default:
            printf("INSTRUCTION UNKNOWN\n");
            break;

    }
}

int32_t cpuDisassemble(Chip8CPU* cpu)
{
    uint16_t opcode;
    opcode = (cpu->ram[cpu->PC] << 8) | (cpu->ram[cpu->PC+1]);
    printf("0x%04x %02x %02x\t", cpu->PC, LOWER_BYTE(opcode), UPPER_BYTE(opcode));

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode) {
                /* 0nnn - SYS addr */
                case 0x0000:
                    printf("SYS 0x%04x\n", opcode);
                    break;

                /* 00E0 - CLS */
                case 0x00E0:
                    printf("CLS\n");
                    break;

                /* 00EE - RET */
                case 0x00EE:
                    printf("RET\n");
                    break;

                default:
                    printf("INSTRUCTION UNKNOWN\n");
                    break;
            }
            break;

        /* 1nnn - JP addr */
        case 0x1000:
            printf("JP 0x%04x\n", OP_NNN(opcode));
            break;

        /* 2nnn - CALL addr */
        case 0x2000:
            printf("CALL 0x%04x\n", OP_NNN(opcode));
            break;

        /* 3xkk - SE Vx, byte */
        case 0x3000:
            printf("SE V%x, #%02x\n", OP_X(opcode), OP_KK(opcode));
            break;

        /* 4xkk - SNE Vx, byte */
        case 0x4000:
            printf("SNE V%x, #%02x\n", OP_X(opcode), OP_KK(opcode));
            break;

        /* 5xy0 - SE Vx, Vy */
        case 0x5000:
            printf("SE V%x, V%x\n", OP_X(opcode), OP_Y(opcode));
            break;

        /* 6xkk - LD Vx, byte */
        case 0x6000:
            printf("LD V%x, #%02x\n", OP_X(opcode), OP_KK(opcode));
            break;

        /* 7xkk - ADD Vx, byte */
        case 0x7000:
            printf("ADD V%x, #%02x\n", OP_X(opcode), OP_KK(opcode));
            break;

        case 0x8000:
            switch (opcode & 0x000F) {
                /* 8xy0 - LD Vx, Vy */
                case 0x0000:
                    printf("LD V%x, V%x\n", OP_X(opcode), OP_Y(opcode));
                    break;

                /* 8xy1 - OR Vx, Vy */
                case 0x0001:
                    printf("OR V%x, V%x\n", OP_X(opcode), OP_Y(opcode));
                    break;

                /* 8xy2 - AND Vx, Vy */
                case 0x0002:
                    printf("AND V%x, V%x\n", OP_X(opcode), OP_Y(opcode));
                    break;

                /* 8xy3 - XOR Vx, Vy */
                case 0x0003:
                    printf("XOR V%x, V%x\n", OP_X(opcode), OP_Y(opcode));
                    break;

                /* 8xy4 - ADD Vx, Vy */
                case 0x0004:
                    printf("ADD V%x, V%x\n", OP_X(opcode), OP_Y(opcode));
                    break;

                /* 8xy5 - SUB Vx, Vy */
                case 0x0005:
                    printf("SUB V%x, V%x\n", OP_X(opcode), OP_Y(opcode));
                    break;

                /* 8xy6 - SHR Vx {, Vy} */
                case 0x0006:
                    printf("SHR V%x {, V%x}\n", OP_X(opcode), OP_Y(opcode));
                    break;

                /* 8xy7 - SUBN Vx, Vy */
                case 0x0007:
                    printf("SUBN V%x, V%x\n", OP_X(opcode), OP_Y(opcode));
                    break;

                /* 8xyE - SHL Vx {, Vy} */
                case 0x000E:
                    printf("SHL V%x {, V%x}\n", OP_X(opcode), OP_Y(opcode));
                    break;

                default:
                    printf("INSTRUCTION UNKNOWN\n");
                    break;
            }
            break;

        /* 9xy0 - SNE Vx, Vy */
        case 0x9000:
            printf("SNE V%x, V%x\n", OP_X(opcode), OP_Y(opcode));
            break;

        /* Annn - LD I, addr */
        case 0xA000:
            printf("LD I, 0x%04x\n", OP_NNN(opcode));
            break;

        /* Bnnn - JP V0, addr */
        case 0xB000:
            printf("JP V0, 0x%04x\n", OP_NNN(opcode));
            break;

        /* Cxkk - RND Vx, byte */
        case 0xC000:
            printf("RND V%x, #%x\n", OP_X(opcode), OP_KK(opcode));
            break;

        /* Dxyn - DRW Vx, Vy, nibble */
        case 0xD000:
            printf("DRW V%x, V%x, %x\n", OP_X(opcode), OP_Y(opcode), OP_N(opcode));
            break;

        case 0xE000:
            switch (opcode & 0x00FF) {
                /* Ex9E - SKP Vx */
                case 0x009E:
                    printf("SKP V%x\n", OP_X(opcode));
                    break;

                /* ExA1 - SKNP Vx */
                case 0x00A1:
                    printf("SKNP V%x\n", OP_X(opcode));
                    break;

                default:
                    printf("INSTRUCTION UNKNOWN\n");
                    break;
            }
            break;

        case 0xF000:
            switch (opcode & 0x00FF) {
                /* Fx07 - LD Vx, DT */
                case 0x0007:
                    printf("LD V%x, DT\n", OP_X(opcode));
                    break;

                /* Fx0A - LD Vx, K */
                case 0x000A:
                    printf("LD V%x, K\n", OP_X(opcode));
                    break;

                /* Fx15 - LD DT, Vx */
                case 0x0015:
                    printf("LD DT, V%x\n", OP_X(opcode));
                    break;

                /* Fx18 - LD ST, Vx */
                case 0x0018:
                    printf("LD ST, V%x\n", OP_X(opcode));
                    break;

                /* Fx1E - ADD I, Vx */
                case 0x001E:
                    printf("ADD I, V%x\n", OP_X(opcode));
                    break;

                /* Fx29 - LD F, Vx */
                case 0x0029:
                    printf("LD F, V%x\n", OP_X(opcode));
                    break;

                /* Fx33 - LD B, Vx */
                case 0x0033:
                    printf("LD B, V%x\n", OP_X(opcode));
                    break;

                /* Fx55 - LD [I], Vx */
                case 0x0055:
                    printf("LD [I], V%x\n", OP_X(opcode));
                    break;

                /* Fx65 - LD Vx, [I] */
                case 0x0065:
                    printf("LD V%x, [I]\n", OP_X(opcode));
                    break;

                default:
                    printf("INSTRUCTION UNKNOWN\n");
                    break;
            }
            break;

        default:
            printf("INSTRUCTION UNKNOWN\n");
            break;
    }
    return 2;
}
