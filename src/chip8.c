#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "chip8.h"

#define FRAME_RATE              60
#define FRAME_TIME_MS           (1000 / FRAME_RATE)
#define CPU_FREQUENCY           600
#define CYCLES_PER_FRAME_TIME   (CPU_FREQUENCY / FRAME_RATE)

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static const uint8_t* keyStates = NULL;

const uint8_t KeyBindings[16] = {
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_4,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_E,
    SDL_SCANCODE_R,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S,
    SDL_SCANCODE_D,
    SDL_SCANCODE_F,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_X,
    SDL_SCANCODE_C,
    SDL_SCANCODE_V
};

void chip8Init(Chip8CPU* cpu)
{
    cpuInit(cpu);
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("Chip8 Emulator",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               WINDOW_WIDTH,
                               WINDOW_HEIGHT,
                               SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
    }

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                SCREEN_WIDTH,
                                SCREEN_HEIGHT);
    if (texture == NULL) {
        fprintf(stderr, "Could not create texture: %s\n", SDL_GetError());
    }

    keyStates = SDL_GetKeyboardState(NULL);
}

void chip8Exit(Chip8CPU* cpu)
{
    SDL_DestroyTexture(texture);
    texture = NULL;
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
    SDL_DestroyWindow(window);
    window = NULL;

    SDL_Quit();
    free(cpu);
}

int32_t chip8LoadROM(Chip8CPU* cpu, const char* file)
{
    int32_t filesize = 0;

    FILE* fp = fopen(file, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error opening ROM file: %s\n", file);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (filesize > ROM_MAXSIZE) {
        fprintf(stderr, "ROM file exceeds maximum allowable size.\n");
        return -1;
    }

    fread(&cpu->ram[ROM_START], filesize, 1, fp);
    fclose(fp);
    return filesize;
}

void chip8Execute(Chip8CPU* cpu)
{
    int32_t i, t0, elapsed;
    bool exit = false;
    SDL_Event event;

    while (!exit) {

        t0 = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || keyStates[SDL_SCANCODE_ESCAPE]) {
                exit = true;
            }
        }
        for (i = 0; i < KEY_SIZE; i++) {
            cpu->key[i] = keyStates[ KeyBindings[i] ];
        }

        for (i = 0; i < CYCLES_PER_FRAME_TIME; i++) {
            cpuExecute(cpu);
        }

        chip8DrawScreen(cpu);
        cpuUpdateTimers(cpu);

        elapsed = SDL_GetTicks() - t0;
        if (elapsed < FRAME_TIME_MS) {
            SDL_Delay(FRAME_TIME_MS - elapsed);
        }
    }
}

void chip8DrawScreen(Chip8CPU* cpu) {
    int32_t i;
    uint32_t pixels[FRAMEBUFF_SIZE];
    for (i = 0; i < FRAMEBUFF_SIZE; i++) {
        pixels[i] = (cpu->framebuff[i] * 0x00FFFFFF);
    }

    SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint32_t));

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void chip8PrintROMDisassembly(Chip8CPU* cpu, const char* file)
{
    int32_t opcodeSize, romSize;

    romSize = chip8LoadROM(cpu, file);
    while (cpu->PC < ROM_START + romSize) {
        opcodeSize = cpuDisassemble(cpu);
        cpu->PC += opcodeSize;
    }
}
