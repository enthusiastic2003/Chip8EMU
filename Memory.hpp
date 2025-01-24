#ifndef MEMORY_H
#define MEMORY_H

#include <cstdio>
#include <cstdlib>
#include "CHIP8Def.hpp"


class Memory {
public:
    static constexpr u32 MAX_MEM = 4096;

private:
    Byte Data[MAX_MEM];

public:
    void init() {
        for (u32 i = 0; i < MAX_MEM; i++) {
            Data[i] = 0;
        }
    }

    Byte operator[](u32 address) const {
        if (address >= 0x0000 && address < 0x1000) {
            return Data[address];
        } else {
            printf("Memory out of bounds access at address: 0x%04X\n", address);
            exit(1);
        }
    }

    Byte& operator[](u32 address) {
        if (address >= 0x0000 && address < 0x1000) {
            return Data[address];
        } else {
            printf("Memory out of bounds access at address: 0x%04X\n", address);
            exit(1);
        }
    }
};



class Stack {
public:
    static constexpr u32 STACK_SIZE = 16;

    u8 SP = 0;  // Points to the next free position
    Word Stack[STACK_SIZE] = {0};  // Initialize stack to 0

    // Push a value onto the stack
    void push(Word value) {
        if (SP < STACK_SIZE) {
            Stack[SP++] = value;
        } else {
            printf("Stack Overflow: Cannot push value 0x%04X\n", value);
            exit(1);  // Handle overflow gracefully in production
        }
    }

    // Pop a value from the stack
    Word pop() {
        if (SP > 0) {
            return Stack[--SP];
        } else {
            printf("Stack Underflow: Cannot pop from an empty stack\n");
            exit(1);  // Handle underflow gracefully in production
        }
    }

    // Reset the stack
    void init() {
        SP = 0;
        for (u8 i = 0; i < STACK_SIZE; i++) {
            Stack[i] = 0;
        }
    }
};


#include <cstdio>
#include <stdexcept>  // For throwing exceptions
#include <cstring>  // For memset
class gfxMemory {
public:
    static constexpr u32 SCREEN_WIDTH = 64;
    static constexpr u32 SCREEN_HEIGHT = 32;
    static constexpr u32 GFX_MAX_MEM = SCREEN_WIDTH * SCREEN_HEIGHT;
    
    Byte GFXData[GFX_MAX_MEM];

    void init() {
        memset(GFXData, 0, sizeof(GFXData)); // Set all pixels to 0
    }
};


#include <SDL2/SDL.h>
#include <iostream>

// Constants for screen dimensions
const int SCREEN_WIDTH = 640; // 10 pixels per Chip-8 pixel (scaling factor)
const int SCREEN_HEIGHT = 320; // 10 pixels per Chip-8 pixel (scaling factor)

// Create a window and renderer
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// Initialize SDL
bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create window
    window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set the renderer draw color (black)
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    return true;
}

// Clean up SDL resources
void closeSDL() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void renderScreen(gfxMemory& gfxMem) {
    // Clear screen with black color
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(renderer);

    // Set white color for "on" pixels
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            if (gfxMem.GFXData[y * 64 + x] == 1) {
                SDL_Rect pixelRect = { x * 10, y * 10, 10, 10 };
                SDL_RenderFillRect(renderer, &pixelRect);
            }
        }
    }
    SDL_RenderPresent(renderer);
}


#endif // MEMORY_H
