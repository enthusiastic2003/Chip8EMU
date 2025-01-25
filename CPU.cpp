#include<stdio.h>
#include<stdlib.h>
#include "CHIP8Def.hpp"
#include "Memory.hpp"
#include <fstream>
#include <vector>
#include <iostream>
class CPU{
    
    Byte V[16]; // Registers
    Word PC; // Program Counter
    Word I; // Index Register
    Byte delay_timer;
    Byte sound_timer;

    static constexpr Byte chip8_fontset[80] =
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
    
    public:

     void updateTimers() {
        // Decrease the delay timer if it is greater than 0
        if (delay_timer > 0) {
            --delay_timer;
        }

        // Decrease the sound timer if it is greater than 0
        if (sound_timer > 0) {
            --sound_timer;
        }

    }

    bool keypad[16] = {false};  // Keypad array, with 16 keys
    
    static const int NUM_KEYS = 16; // Number of keys in the Chip-8 keypad

    // Map the SDL key events to Chip-8 keys
    int keyMapping[NUM_KEYS];

    // Initialize the key mapping
    void initKeyMapping() {
        keyMapping[0] = SDLK_x;
        keyMapping[1] = SDLK_1;
        keyMapping[2] = SDLK_2;
        keyMapping[3] = SDLK_3;
        keyMapping[4] = SDLK_q;
        keyMapping[5] = SDLK_w;
        keyMapping[6] = SDLK_e;
        keyMapping[7] = SDLK_a;
        keyMapping[8] = SDLK_s;
        keyMapping[9] = SDLK_d;
        keyMapping[10] = SDLK_z;
        keyMapping[11] = SDLK_c;
        keyMapping[12] = SDLK_4;
        keyMapping[13] = SDLK_r;
        keyMapping[14] = SDLK_f;
        keyMapping[15] = SDLK_v;
    }

    
    // Update the keypad state based on SDL events
    void updateKeypad() {
        SDL_PumpEvents();  // Update the state of all SDL events (including key events)

        // Loop through all the keys on the keypad
        for (int i = 0; i < NUM_KEYS; i++) {
            // Check if the key is pressed or released
            if (SDL_GetKeyboardState(NULL)[SDL_GetScancodeFromKey(keyMapping[i])] == 1) {
                keypad[i] = true;  // Key is pressed
            } else {
                keypad[i] = false; // Key is not pressed
            }
        }
    }



    bool isKeyPressed(Byte key) {
        return keypad[key]; // Return whether the key is pressed or not
    }

    Byte waitForKeypress(){
        while (true) {
            updateKeypad(); // Update the keypad state

            // Loop through all the keys on the keypad
            for (int i = 0; i < NUM_KEYS; i++) {
                if (keypad[i]) {
                    return i; // Return the key that is pressed
                }
            }
        }
    }

    Memory mem;
    gfxMemory gfxMem;
    Stack stack;

    bool loadROM(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Failed to open ROM file: " << filepath << std::endl;
            return false;
        }


        // Get file size
        std::streamsize romSize = file.tellg();

        // Disabled check
        // if (romSize >= (MEMORY_SIZE - PROGRAM_START)) {
        //     std::cerr << "ROM size exceeds available memory!" << std::endl;
        //     return false;
        // }

        // Read the ROM into memory starting at 0x200
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(&mem[0x200]), romSize);
        file.close();

        std::cout << "ROM loaded successfully! Size: " << romSize << " bytes" << std::endl;
        return true;
    }

    static constexpr Byte INS_SYS = 0x0;


    void reset(){
        
        PC = 0x200; // Program counter starts at 0x200
        I = 0; // Reset index register

        for (int i = 0; i < 16; i++) // Clear all registers
        {
            V[i] = 0;
        }

        mem.init(); // Clear memory
        gfxMem.init(); // Clear graphics memory
        stack.init(); // Clear stack
        
        initKeyMapping(); // Initialize the key mapping

        // Load fontset
        for (int i = 0; i < 80; i++)
        {
            mem[i] = chip8_fontset[i];
        }

        // Reset timers
        delay_timer = 0;
        sound_timer = 0;


    }



    Word fetchWord(Memory &mem){
        Word opcode = (mem[PC] << 8) | mem[PC + 1];
        PC += 2;
        return opcode;
    }

void drawSprite(Byte VX, Byte VY, Byte N) {
   V[0xF] = 0; // Reset collision flag
   Byte x = V[VX] % 64; // Wrap around horizontally
   Byte y = V[VY] % 32; // Wrap around vertically

   for (Byte row = 0; row < N; row++) {
       if (y + row >= 32) break; // Ignore rows outside the screen
       
       Byte spriteRow = mem[I + row]; // Fetch sprite byte from memory
       for (Byte col = 0; col < 8; col++) {
           if (x + col >= 64) break; // Ignore columns outside the screen
           
           // Calculate pixel position in the graphics memory
           Byte &pixel = gfxMem.GFXData[(y + row) * 64 + (x + col)];
           
           // Check if collision occurs (if the pixel is being erased)
           if ((spriteRow & (0x80 >> col)) != 0) { // Mask for the current bit
               if (pixel == 1) {
                   V[0xF] = 1; // Collision detected
               }
               // XOR the pixel value
               pixel ^= 1;
           }
       }
   }
}

    
    void execute(){
        
        Word opcode = fetchWord(mem);

        Byte InstructionType = (opcode & 0xF000) >> 12;
        Word Instruction = opcode & 0x0FFF;


        switch (InstructionType)
        {
        case 0x0:{
            // System Instruction

            switch (Instruction)
            {
            case 0x00E0:
                // Clear the screen
                gfxMem.init();

                break;
            case 0x00EE:
                // Return from a subroutine
                PC = stack.pop();

                break;
            default:
            {// Call RCA 1802 program at address NNN
                PC=Instruction;
                printf("Executed 0NNN\n");
                //exit(1);
            }break;

            }      
        }break;

        case 0x1:
            // Jump to address NNN
            PC = Instruction;
            break;
        
        case 0x2:
        {
            // Call subroutine at NNN
            stack.push(PC);
            PC = Instruction;
        }break;

        case 0x3:
        {
            Byte Reg = ( Instruction & 0x0F00 ) >> 8;
            Byte Check = (Instruction & 0x00FF);

            if(V[Reg] == Check){
                PC+=2;
            }
        }break;

        case 0x4:
        {
            Byte Reg = ( Instruction & 0x0F00 ) >> 8;
            Byte Check = (Instruction & 0x00FF);

            if(V[Reg] != Check){
                PC+=2;
            }
        }break;

        case 0x5:
        {
            Byte Reg1 = (Instruction & 0x0F00) >> 8;
            Byte Reg2 = (Instruction & 0x00F0) >> 4;
            Byte ChkSum = (Instruction & 0x000F);

            if(ChkSum!=0){
                printf("0x5XY- Instruction not valid");
                exit(1);
            }

            if(V[Reg1] == V[Reg2]){
                PC+=2;
            }
        }break;

        case 0x6: {
            // 6XNN: Store number NN in register VX
            Byte Reg = (Instruction & 0x0F00) >> 8;  // Extract register index
            Byte Value = Instruction & 0x00FF;       // Extract NN (8-bit value)
            V[Reg] = Value;                          // Store NN in VX
        } break;

        case 0x7: {
            // 7XNN: Add the value NN to register VX
            Byte Reg = (Instruction & 0x0F00) >> 8;  // Extract register index
            Byte Value = Instruction & 0x00FF;       // Extract NN (8-bit value)
            V[Reg] += Value;                         // Add NN to VX
        } break;

        case 0x8: {
            Byte RegX = (Instruction & 0x0F00) >> 8; // Extract VX
            Byte RegY = (Instruction & 0x00F0) >> 4; // Extract VY
            Byte ChkSum = Instruction & 0x000F;      // Extract last nibble for operation

            switch (ChkSum) {
                case 0x0:
                    // 8XY0: Store the value of register VY in register VX
                    V[RegX] = V[RegY];
                    break;

                case 0x1:
                    // 8XY1: Set VX to VX OR VY
                    {V[RegX] = V[RegX] | V[RegY];
                    V[0xF] = 0; // Reset VF to 0
                    }break;

                case 0x2:
                    // 8XY2: Set VX to VX AND VY
                    {V[RegX] = V[RegX] & V[RegY];
                    V[0xF] = 0; // Reset VF to 0
                    }break;

                case 0x3:
                    // 8XY3: Set VX to VX XOR VY
                    {V[RegX] = V[RegX] ^ V[RegY];
                    V[0xF] = 0; // Reset VF to 0
                    }break;

                case 0x4:
                    // 8XY4: Add the value of register VY to register VX
                    {
                        u16 result = V[RegX] + V[RegY];
                        V[RegX] = result & 0xFF;  // Store the lower 8 bits of the result in VX
                        V[0xF] = (result > 0xFF) ? 1 : 0;  // Set VF to 1 if there was a carry
                    }
                    break;

                case 0x5:
                    // 8XY5: Subtract the value of register VY from register VX
                    {
                       Byte borrow = V[RegX] < V[RegY] ? 0 : 1;
                       
                       V[RegX] = V[RegX] - V[RegY];  // Subtract VY from VX and store the result in VX
                       V[0xF] = borrow;  // Set VF to 1 if there was no borrow
                    
                    }break;

                case 0x6:
                    {
                        // 8XY6: Store the value of register VY shifted right one bit in register VX
                        Byte temp = V[RegY] & 0b00000001;  // Set VF to the least significant bit of VY
                        V[RegX] = V[RegY] >> 1;  // Shift VY to the right by one bit and store it in VX
                        V[0xF] = temp;  // Set VF to the least significant bit of VY
                    }break;

                case 0x7:
                    // 8XY7: Set register VX to the value of VY minus VX
                    {
                        u16 result = V[RegY] - V[RegX];
                        V[RegX] = result & 0xFF;  // Store the lower 8 bits of the result in VX
                        V[0xF] = (V[RegY] > V[RegX]) ? 1 : 0;  // Set VF to 1 if there was no borrow
                    }
                    break;

                case 0xE:
                {
                    // 8XYE: Store the value of register VY shifted left one bit in register VX
                    Byte temp = (V[RegY] & 0b10000000) >> 7;  // Set VF to the most significant bit of VY
                    V[RegX] = V[RegY] << 1;  // Shift VY to the left by one bit and store it in VX
                    V[0xF] = temp;  // Set VF to the most significant bit of VY
                }break;
                

                default:
                    printf("Invalid 0x8XY- Instruction: 0x%04X\n", opcode);
                    exit(1);
                    break;
                }
            }break;

        case 0x9: 
            {
                // 9XY0 - Skip the following instruction if VX != VY
                Byte Reg1 = (Instruction & 0x0F00) >> 8; // Extract VX
                Byte Reg2 = (Instruction & 0x00F0) >> 4; // Extract VY
                Byte ChkSum = (Instruction & 0x000F);    // Ensure the last nibble is 0

                if (ChkSum != 0) {
                    printf("0x9XY- Instruction not valid");
                    exit(1);
                }

                if (V[Reg1] != V[Reg2]) {
                    PC += 2; // Skip next instruction
                }
            } 
            break;

        case 0xA: 
            {
                // ANNN - Store memory address NNN in register I
                I = Instruction; // Store the 12-bit address in I
            } 
            break;

        case 0xB: 
            {
                // BNNN - Jump to address NNN + V0
                PC = Instruction + V[0]; // NNN is the lower 12 bits
            } 
            break;

        case 0xC: 
            {
                // CXNN - Set VX to a random number with a mask of NN
                Byte Reg = (Instruction & 0x0F00) >> 8; // Extract VX
                Byte Mask = Instruction & 0x00FF;       // Extract NN

                // Random number generation (0-255) & apply mask
                V[Reg] = (rand() % 256) & Mask;
            } 
            break;
        
        case 0xD:
        {
            Byte VX = (Instruction & 0x0F00) >> 8;
            Byte VY = (Instruction & 0x00F0) >> 4;
            Byte N = Instruction & 0x000F;
            drawSprite(VX, VY, N);

        }break;

        case 0xE:
        {
            Byte Reg = (Instruction & 0x0F00) >> 8;  // Extract the register VX
            Byte Op = (Instruction & 0x00FF);

            switch (Op) {
                case 0x9E:
                {
                    // EX9E: Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed
                    Byte key = V[Reg];  // The value in register VX corresponds to the key.
                    
                    // Assuming you have a key state array (0 for unpressed, 1 for pressed)
                    
                    
                    //TODO: Implement Keypad
                    if (keypad[key] == true) { // If the key is pressed
                        PC += 2; // Skip the next instruction
                    }
                    
                } break;

                case 0xA1:
                {
                    // EXA1: Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
                    Byte key = V[Reg];  // The value in register VX corresponds to the key.
                    
                    // Assuming you have a key state array (0 for unpressed, 1 for pressed)

                    
                    //TODO::: Implement KeyPad
                    if (keypad[key] == false) { // If the key is not pressed
                        PC += 2; // Skip the next instruction
                    }
                   
                } break;

                default:
                    printf("Unknown EX instruction\n");
                    exit(1);
            }
        }break;

        case 0xF:
        {
            Byte Reg = (Instruction & 0x0F00) >> 8;  // Extract the register VX
            Byte Op = (Instruction & 0x00FF);
            switch (Op)
            {
                case 0x07:
                {
                    // FX07: Store the current value of the delay timer in register VX
                    V[Reg] = delay_timer; // Store the current delay timer value in Vx
                } break;

                case 0x0A:
                {
                    // FX0A: Wait for a keypress and store the result in register VX
                    // Assume a function `waitForKeypress` that waits for a key press and returns the key value
                    Byte key = waitForKeypress(); // Function should return the key pressed
                    V[Reg] = key;  // Store the key value in register Vx
                    

                } break;

                case 0x15:
                {
                    // FX15: Set the delay timer to the value of register VX
                    delay_timer = V[Reg]; // Set the delay timer to the value of Vx
                } break;

                case 0x18:
                {
                    // FX18: Set the sound timer to the value of register VX
                    sound_timer = V[Reg]; // Set the sound timer to the value of Vx
                } break;

                case 0x1E:
                {
                    // FX1E: Add the value stored in register VX to register I
                    Byte temp;
                    if (I + V[Reg] > 0xFFF) {
                        temp = 0; // Set VF to 1 if there is overflow
                    } else {
                       temp = 1;
                    }

                    I += V[Reg]; // Add the value of Vx to register I
                    // Set VF to 1 if there is overflow
                    V[0xF] = temp;

                } break;

                case 0x29:
                {
                    Byte temp;
                    Byte digit = V[Reg]; // Get the value in register VX (hex digit)

                    if (digit*0x5 > 0xFFF) {
                        temp = 0; // Set VF to 1 if there is overflow
                    } else {
                        temp = 1;
                    }
                    // FX29: Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
                    I = digit * 0x5;  // Each sprite data for digits 0-F is 5 bytes long, so we multiply the digit by 5
                    V[0xF] = temp; // Set VF to 1 if there is overflow
                } break;

                case 0x33:
                {
                    // FX33: Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2
                    Byte value = V[Reg]; // Get the value from register VX
                    mem[I] = value / 100;      // Store hundreds place
                    mem[I + 1] = (value / 10) % 10; // Store tens place
                    mem[I + 2] = value % 10;  // Store ones place
                } break;

                case 0x55:
                {
                    // FX55: Store the values of registers V0 to VX inclusive in memory starting at address I
                    for (u32 i = 0; i <= Reg; i++) {
                        mem[I + i] = V[i]; // Store values from V0 to VX at memory starting from I
                    }
                    I += Reg + 1; // After the operation, I is updated to I + X + 1
                } break;

                case 0x65:
                {
                    // FX65: Fill registers V0 to VX inclusive with the values stored in memory starting at address I
                    for (u32 i = 0; i <= Reg; i++) {
                        V[i] = mem[I + i]; // Load values from memory starting at I into V0 to VX
                    }
                    I += Reg + 1; // After the operation, I is updated to I + X + 1
                } break;

                default:
                    printf("Unknown FX instruction\n");
                    exit(1);
            }
        }
        break;

        default:
        {
            printf("Unimplemented Instruction %x ", (InstructionType << 12 )| (Instruction >> 4));
            exit(1);
        }

        }
    } 

};




#include <iostream>
#include <cstdlib>
#include <cstring>
#include <csignal>

// Global flag to control when to exit the main loop
volatile bool quitFlag = false;

// Signal handler to catch Ctrl+C (SIGINT)
void signalHandler(int signum) {
    std::cout << "Caught signal " << signum << ", quitting..." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
   if (argc != 2) {
       std::cerr << "Usage: " << argv[0] << " <path_to_rom>" << std::endl;
       return 1;
   }

   const char* romPath = argv[1];
   CPU cpu;
   cpu.reset();

   bool ret = initSDL();
   if (!ret) {
       std::cerr << "Failed to initialize display" << std::endl;
       return 1;
   }

   ret = cpu.loadROM(romPath);
   if (!ret) {
       std::cerr << "Failed to load ROM: " << romPath << std::endl;
       return 1;
   }

   std::signal(SIGINT, signalHandler);

   const int CYCLE_DELAY = 2; // Adjust for instruction speed
   const int TIMER_INTERVAL = 16; // ~60Hz
   Uint32 lastTimerUpdate = SDL_GetTicks();

   while (!quitFlag) {
       Uint32 currentTicks = SDL_GetTicks();

       cpu.updateKeypad();
       cpu.execute();

       // Update timers at 60Hz
       if (currentTicks - lastTimerUpdate >= TIMER_INTERVAL) {
           cpu.updateTimers();
           lastTimerUpdate = currentTicks;
           renderScreen(cpu.gfxMem);
       }

       //renderScreen(cpu.gfxMem);
       SDL_Delay(CYCLE_DELAY);
   }

   closeSDL();
   return 0;
}

